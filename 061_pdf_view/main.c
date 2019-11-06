/* hardware-accelerated PDF rendering

compile:
gcc main.c apg_pdf.c glew.c -lm -lglfw -lGL

run:
./a.out sample_PDF.pdf

may be handy
- NV path rendering OpenGL extension, Mark Kilgard and Jeff Bolz
- Google Skia
- stb_truetype, slug
*/

#define _CRT_SECURE_NO_WARNINGS

#include "apg_pdf.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <GL/glew.h>
#define GLFW_DLL
#include <GLFW/glfw3.h>

typedef struct file_record_t {
  uint8_t* data;
  size_t sz;
} file_record_t;

static file_record_t _read_entire_file( const char* filename ) {
  file_record_t record;
  memset( &record, 0, sizeof( file_record_t ) );

  FILE* fp = fopen( filename, "rb" );
  if ( !fp ) { return record; }
  {
    fseek( fp, 0L, SEEK_END );
    record.sz   = (size_t)ftell( fp );
    record.data = malloc( record.sz );
    rewind( fp );
    size_t nr = fread( record.data, record.sz, 1, fp );
    if ( 1 != nr ) {
      free( record.data );
      record.data = NULL;
    }
  }
  fclose( fp );
  return record;
}

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

    g_window = glfwCreateWindow( g_win_width, g_win_height, "PDF in OpenGL Test", NULL, NULL );
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
  int w, h, ncomps;
};

struct texture_t texture_from_jpeg( const uint8_t* jpeg_data, size_t jpeg_sz ) {
  struct texture_t texture;
  memset( &texture, 0, sizeof( struct texture_t ) );

  uint8_t* raw_data = stbi_load_from_memory( jpeg_data, jpeg_sz, &texture.w, &texture.h, &texture.ncomps, 0 );
  assert( raw_data );

  printf( "loaded image for texture - %ix%i n=%i\n", texture.w, texture.h, texture.ncomps );

  glGenTextures( 1, &texture.handle_gl );
  {
    glPixelStorei( GL_UNPACK_ALIGNMENT, 1 ); // for irregular display sizes in RGB
    glBindTexture( GL_TEXTURE_2D, texture.handle_gl );
    if ( 4 == texture.ncomps ) {
      glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA, texture.w, texture.h, 0, GL_RGBA, GL_UNSIGNED_BYTE, raw_data );
    } else if ( 3 == texture.ncomps ) {
      glTexImage2D( GL_TEXTURE_2D, 0, GL_RGB, texture.w, texture.h, 0, GL_RGB, GL_UNSIGNED_BYTE, raw_data );
    } else if ( 1 == texture.ncomps ) {
      glTexImage2D( GL_TEXTURE_2D, 0, GL_RED, texture.w, texture.h, 0, GL_RED, GL_UNSIGNED_BYTE, raw_data );
    }
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
    glBindTexture( GL_TEXTURE_2D, 0 );
    glPixelStorei( GL_UNPACK_ALIGNMENT, 4 );
  }
  texture.loaded = true;

  free( raw_data );

  return texture;
}

int main( int argc, char** argv ) {
  apg_pdf_t pdf;
  file_record_t record;

  if ( argc < 2 ) {
    printf( "Usage: %s MY_FILE.PDF\n", argv[0] );
    return 0;
  }

  { // Read file and parse PDF
    printf( "reading `%s`...\n", argv[1] );
    record = _read_entire_file( argv[1] );
    if ( !record.data ) {
      fprintf( stderr, "ERROR: opening file `%s`\n", argv[1] );
      return 1;
    }
    pdf = apg_pdf_read_mem( record.data, record.sz );
    if ( !pdf.loaded ) {
      fprintf( stderr, "ERROR reading PDF from `%s`\n", argv[1] );
      return 1;
    }
  }

  _start_gl();
  struct texture_t texture;

  { // decode and load images into graphics memory
    for ( int i = 0; i < pdf.obj_list_n; i++ ) {
      if ( pdf.obj_list[i].subtype_image ) {
// write JPEG out to a file to test
#ifdef WRITE_IMAGES
        {
          FILE* fptr = fopen( "out.jpg", "wb" );
          if ( !fptr ) { return false; }
          {
            size_t offs = pdf.obj_list[i].stream_data_offset;
            size_t n    = fwrite( &record.data[offs], pdf.obj_list[i].stream_data_length, 1, fptr );
            if ( n != 1 ) {
              fclose( fptr );
              return false;
            }
          }
          fclose( fptr );
        }
#endif

        size_t offs = pdf.obj_list[i].stream_data_offset;
        texture     = texture_from_jpeg( &record.data[offs], pdf.obj_list[i].stream_data_length );

      } // endif subtype_image
    }   // endfor images
  }     // endblock decode images

	glfwSetWindowSize( g_window, texture.w, texture.h );

  { // OpenGL rendering
    const char* vertex_shader =
      "#version 410\n"
      "in vec2 a_vp;"
      "out vec2 v_st;"
      "void main () {"
      "  v_st = a_vp * 0.5 + 0.5;"
      "  v_st.t = 1.0 - v_st.t;"
      "  gl_Position = vec4( a_vp.x, a_vp.y, 0.0, 1.0 );"
      "}";
    const char* fragment_shader =
      "#version 410\n"
      "in vec2 v_st;"
      "uniform sampler2D u_tex;"
      "out vec4 o_frag_colour;"
      "void main () {"
      "  vec4 texel = texture( u_tex, v_st );"
      "  o_frag_colour = vec4( texel.rgb, 1.0 );"
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
      glfwWaitEvents(); // be kind to the CPU when not needing to update
      int width, height;
      glfwGetFramebufferSize( g_window, &width, &height );
      glViewport( 0, 0, width, height );
      glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

      glUseProgram( program );
      glActiveTexture( GL_TEXTURE0 );
      glBindVertexArray( vertex_array_gl );

      glBindTexture( GL_TEXTURE_2D, texture.handle_gl );
      glDrawArrays( GL_TRIANGLES, 0, 6 );

      glBindVertexArray( 0 );
      glUseProgram( 0 );

      glfwSwapBuffers( g_window );
    }

    glfwTerminate();
  }

  free( record.data );
  printf( "program HALT\n" );
  return 0;
}
