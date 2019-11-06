// threaded - took ~3 seconds to display all, but also started right away
// serial   - took >6 seconds to display all, and no rendering until then

// windows:
// gcc .\main.c ..\common\src\GL\glew.c .\threads.c -lpthread -I ..\common\include\  -DGLEW_STATIC -lOpenGL32  ..\common\win64_gcc\libglfw3.a -lgdi32 -lws2_32
// linux:
// gcc main.c threads.c ../common/src/GL/glew.c -I ../common/include/ -lglfw -lm -lpthread -lGL

#include "threads.h"

#define STBI_ONLY_PNG
#define STB_IMAGE_IMPLEMENTATION
#include "stb/stb_image.h"
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <stdio.h>
#include <assert.h>
#include <unistd.h> // not portable TODO

#define TEST_IMAGE_FN "main_menu_bkrnd.png"

// -- FLIP THIS VALUE TO TEST SERIAL V THREADED
bool threaded = true;

static int g_win_width = 1920, g_win_height = 1080;
static GLFWwindow* g_window;

static bool _start_gl() {
  {
    if ( !glfwInit() ) {
      fprintf( stderr, "ERROR: could not start GLFW3\n" );
      return false;
    }
    glfwWindowHint( GLFW_CONTEXT_VERSION_MAJOR, 4 );
    glfwWindowHint( GLFW_CONTEXT_VERSION_MINOR, 1 );
    glfwWindowHint( GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE );
    glfwWindowHint( GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE );

    g_window = glfwCreateWindow( g_win_width, g_win_height, "Voxel Test", NULL, NULL );
    if ( !g_window ) {
      fprintf( stderr, "ERROR: could not open window with GLFW3\n" );
      glfwTerminate();
      return false;
    }
    glfwMakeContextCurrent( g_window );
  }
  glewExperimental = GL_TRUE;
  glewInit();

  const GLubyte* renderer = glGetString( GL_RENDERER );
  const GLubyte* version  = glGetString( GL_VERSION );
  printf( "Renderer: %s\n", renderer );
  printf( "OpenGL version supported %s\n", version );

  return true;
}

struct texture_t {
  char filename[256];
  GLuint handle_gl;

  bool loaded;
  // don't touch if loaded is false! - thread will set then set loaded when safe
  int w, h, ncomps;
  unsigned char* pixels_ptr; // only used whilst loading
};

struct texture_t load_texture( const char* filename ) {
  struct texture_t texture = { 0 };

  glGenTextures( 1, &texture.handle_gl );

  texture.pixels_ptr = stbi_load( filename, &texture.w, &texture.h, &texture.ncomps, 4 );
  stbi__vertical_flip( texture.pixels_ptr, texture.w, texture.h, texture.ncomps );
  assert( texture.pixels_ptr );
  strncat( texture.filename, filename, 255 );
  {
    glBindTexture( GL_TEXTURE_2D, texture.handle_gl );
    glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA, texture.w, texture.h, 0, GL_RGBA, GL_UNSIGNED_BYTE, texture.pixels_ptr );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
    glBindTexture( GL_TEXTURE_2D, 0 );
  }
  stbi_image_free( texture.pixels_ptr );
  texture.loaded = true;

  return texture;
}

void _load_texture_sr( int worker_idx, void* args ) {
  struct texture_t* texture_ptr = (struct texture_t*)args;
  printf( "loading `%s` on worker %i\n", texture_ptr->filename, worker_idx );
  texture_ptr->pixels_ptr = stbi_load( texture_ptr->filename, &texture_ptr->w, &texture_ptr->h, &texture_ptr->ncomps, 4 );
  stbi__vertical_flip( texture_ptr->pixels_ptr, texture_ptr->w, texture_ptr->h, texture_ptr->ncomps );
  assert( texture_ptr->pixels_ptr );
}

void _load_texture_finished_cb( const char* name, void* args ) {
  struct texture_t* texture_ptr = (struct texture_t*)args;

  // can only upload to OpenGL on main thread
  glBindTexture( GL_TEXTURE_2D, texture_ptr->handle_gl );
  glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA, texture_ptr->w, texture_ptr->h, 0, GL_RGBA, GL_UNSIGNED_BYTE, texture_ptr->pixels_ptr );
  glBindTexture( GL_TEXTURE_2D, 0 );

  stbi_image_free( texture_ptr->pixels_ptr );

  texture_ptr->loaded = true;

  printf( "job finished %s for handlegl %i\n", name, texture_ptr->handle_gl );
}

void load_image_file_to_texture_threaded( const char* filename, struct texture_t* texture ) {
  glGenTextures( 1, &texture->handle_gl );
  glBindTexture( GL_TEXTURE_2D, texture->handle_gl );
  glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA, 0, 0, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL );
  glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
  glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );
  glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
  glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
  glBindTexture( GL_TEXTURE_2D, 0 );
  strncat( texture->filename, filename, 255 );

  job_description_t job = { 0 };
  job.job_function_ptr  = _load_texture_sr;
  job.job_function_args = texture;
  job.on_finished_cb    = _load_texture_finished_cb;
  strncat( job.name, filename, 15 );
  printf( "starting job - texture %s\n", texture->filename );
  worker_pool_push_job( job );
}

void delete_texture( struct texture_t* texture ) {
  glDeleteTextures( 1, &texture->handle_gl );
  memset( texture, 0, sizeof( struct texture_t ) );
}

#define NIMAGES 40

// tests on my laptop loading 40 images into textures (8 worker threads)
// serial:   ~8000ms
// threaded:  ~600ms
int main() {
  struct texture_t textures[NIMAGES] = { 0 };

  _start_gl();
  worker_pool_init();

  if ( threaded ) {
    for ( int i = 0; i < NIMAGES; i++ ) { load_image_file_to_texture_threaded( TEST_IMAGE_FN, &textures[i] ); }
  } else {
    for ( int i = 0; i < NIMAGES; i++ ) {
      textures[i] = load_texture( TEST_IMAGE_FN );
      printf( "loaded texture %s\n", textures[i].filename );
    }
  }

#if 0 // used for timing tests
  bool all_loaded = false;
  while ( !all_loaded ) {
    worker_pool_update();
    all_loaded = true;
    for ( int i = 0; i < NIMAGES; i++ ) {
      if ( !textures[i].loaded ) { all_loaded = false; }
      break;
    }
    if ( !all_loaded ) { usleep( 1000 ); }
  }
  return 0;
#endif

  const char* vertex_shader =
    "#version 410\n"
    "in vec2 a_vp;"
    "uniform float u_idx;"
    "out vec2 v_st;"
    "void main () {"
    "  v_st = a_vp * 0.5 + 0.5;"
    "  float x = mod( u_idx, 7.0 ) * 0.275;"
    "  float y = float( int(u_idx) / 7 ) * 0.275;"
    "  gl_Position = vec4( a_vp.x * 0.125 - 0.85 + x, a_vp.y * 0.125 + 0.85 - y , 0.0, 1.0 );"
    "}";
  const char* fragment_shader =
    "#version 410\n"
    "in vec2 v_st;"
    "uniform sampler2D u_tex;"
    "out vec4 o_frag_colour;"
    "void main () {"
    "  o_frag_colour = texture( u_tex, v_st );"
    "}";
  GLuint vs = glCreateShader( GL_VERTEX_SHADER );
  glShaderSource( vs, 1, &vertex_shader, NULL );
  glCompileShader( vs );
  GLuint fs = glCreateShader( GL_FRAGMENT_SHADER );
  glShaderSource( fs, 1, &fragment_shader, NULL );
  glCompileShader( fs );
  GLuint program = glCreateProgram();
  glAttachShader( program, fs );
  glAttachShader( program, vs );
  glLinkProgram( program );
  glDeleteShader( vs );
  glDeleteShader( fs );
  GLint u_idx = glGetUniformLocation( program, "u_idx" );

  float square[] = { -1, 1, -1, -1, 1, -1, 1, -1, 1, 1, -1, 1 };
  GLuint vertex_buffer_gl, vertex_array_gl;
  glGenVertexArrays( 1, &vertex_array_gl );
  glGenBuffers( 1, &vertex_buffer_gl );

  glBindVertexArray( vertex_array_gl );
  glBindBuffer( GL_ARRAY_BUFFER, vertex_buffer_gl );
  {
    glBufferData( GL_ARRAY_BUFFER, sizeof( float ) * 12, square, GL_STATIC_DRAW );
    glEnableVertexAttribArray( 0 );
    glBindBuffer( GL_ARRAY_BUFFER, vertex_buffer_gl );
    glVertexAttribPointer( 0, 2, GL_FLOAT, GL_FALSE, 0, NULL );
  }
  glBindBuffer( GL_ARRAY_BUFFER, 0 );
  glBindVertexArray( 0 );

  glClearColor( 0.2, 0.2, 0.2, 1.0 );
  while ( !glfwWindowShouldClose( g_window ) ) {
    glfwPollEvents();
    worker_pool_update(); // fire callbacks for any finished threads, and free up workers
    // this means jobs can never be processed in less than this frequency...hmm...
    // could also queue 'done' jobs and keep going

    glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

    glUseProgram( program );
    glActiveTexture( GL_TEXTURE0 );
    glBindVertexArray( vertex_array_gl );
    for ( int i = 0; i < NIMAGES; i++ ) {
      glBindTexture( GL_TEXTURE_2D, textures[i].handle_gl );
      glUniform1f( u_idx, (float)i );
      glDrawArrays( GL_TRIANGLES, 0, 6 );
    }
    glBindVertexArray( 0 );
    glUseProgram( 0 );

    glfwSwapBuffers( g_window );
  }

  worker_pool_free();
  glDeleteProgram( program );
  glDeleteBuffers( 1, &vertex_buffer_gl );
  glDeleteVertexArrays( 1, &vertex_array_gl );
  for ( int i = 0; i < NIMAGES; i++ ) { delete_texture( &textures[i] ); }
  glfwTerminate();
  printf( "program HALT\n" );
  return 0;
}
