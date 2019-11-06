// 2019-05-23 MSAA for FBOs. Copyright Anton Gerdelan <antonofnote@gmail.com>
// C99
//
// windows:
// gcc .\main.c ..\common\src\GL\glew.c -I ..\common\include\ -DGLEW_STATIC -lOpenGL32 ..\common\win64_gcc\libglfw3.a -lgdi32 -lws2_32
// linux:
// gcc main.c ../common/src/GL/glew.c -I ../common/include/ -lglfw -lm  -lpthread -lGL


#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <stdio.h>
#include <assert.h>
#include <stdbool.h>

#define NIMAGES 16

static int g_win_width = 1280, g_win_height = 1024;
static int g_fb_width = 1280, g_fb_height = 1024;
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

int main() {
  _start_gl();

  const char* vertex_shader =
    "#version 410\n"
    "in vec2 a_vp;"
    "uniform float u_idx;"
    "out vec2 v_st;"
    "void main () {"
    "  v_st = a_vp * 0.5 + 0.5;"
    "  float x = mod( u_idx, 7.0 ) * 0.275;"
    "  float y = float( int(u_idx) / 7 ) * 0.275;"
    "  gl_Position = vec4( a_vp.x * 0.25 - 0.75 + x, a_vp.y * 0.25 + 0.75 - y , 0.0, 1.0 );"
    "}";
  const char* fragment_shader =
    "#version 410\n"
    "in vec2 v_st;"
    "out vec4 o_frag_colour;"
    "void main () {"
    "  o_frag_colour = vec4( v_st.s, v_st.t, 0.0, 1.0 );"
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

  float triangle[] = { -1, 1, -1, -1, 1, -1 };
  GLuint vertex_buffer_gl, vertex_array_gl;
  glGenVertexArrays( 1, &vertex_array_gl );
  glGenBuffers( 1, &vertex_buffer_gl );

  glBindVertexArray( vertex_array_gl );
  glBindBuffer( GL_ARRAY_BUFFER, vertex_buffer_gl );
  {
    glBufferData( GL_ARRAY_BUFFER, sizeof( float ) * 6, triangle, GL_STATIC_DRAW );
    glEnableVertexAttribArray( 0 );
    glBindBuffer( GL_ARRAY_BUFFER, vertex_buffer_gl );
    glVertexAttribPointer( 0, 2, GL_FLOAT, GL_FALSE, 0, NULL );
  }
  glBindBuffer( GL_ARRAY_BUFFER, 0 );
  glBindVertexArray( 0 );

  glfwGetFramebufferSize( g_window, &g_fb_width, &g_fb_height );

  GLuint fb, c_tex, d_tex;
  {
    // fb
    int nsamples = 8;
    glGenFramebuffers( 1, &fb );
    glGenTextures( 1, &c_tex );
    glGenTextures( 1, &d_tex );
    glBindFramebuffer( GL_FRAMEBUFFER, fb );
    glActiveTexture( GL_TEXTURE0 );
    glBindTexture( GL_TEXTURE_2D_MULTISAMPLE, c_tex );
    glTexImage2DMultisample( GL_TEXTURE_2D_MULTISAMPLE, nsamples, GL_RGB, g_fb_width, g_fb_height, GL_TRUE );
    glFramebufferTexture2D( GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D_MULTISAMPLE, c_tex, 0 );
    glBindTexture( GL_TEXTURE_2D_MULTISAMPLE, d_tex );
    glTexImage2DMultisample( GL_TEXTURE_2D_MULTISAMPLE, nsamples, GL_DEPTH_COMPONENT, g_fb_width, g_fb_height, GL_TRUE );
    glFramebufferTexture2D( GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D_MULTISAMPLE, d_tex, 0 );
    GLenum draw_bufs[] = { GL_COLOR_ATTACHMENT0 };
    glDrawBuffers( 1, draw_bufs );
    // glReadBuffer( GL_NONE );

    {
      GLenum status = glCheckFramebufferStatus( GL_FRAMEBUFFER );
      if ( GL_FRAMEBUFFER_COMPLETE != status ) {
        printf( "ERROR: incomplete framebuffer\n" );
        if ( GL_FRAMEBUFFER_UNDEFINED == status ) {
          printf( "GL_FRAMEBUFFER_UNDEFINED\n" );
        } else if ( GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT == status ) {
          printf( "GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT\n" );
        } else if ( GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT == status ) {
          printf( "GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT\n" );
        } else if ( GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER == status ) {
          printf( "GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER\n" );
        } else if ( GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER == status ) {
          printf( "GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER\n" );
        } else if ( GL_FRAMEBUFFER_UNSUPPORTED == status ) {
          printf( "GL_FRAMEBUFFER_UNSUPPORTED\n" );
        } else if ( GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE == status ) {
          printf( "GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE\n" );
        } else if ( GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS == status ) {
          printf( "GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS\n" );
        } else {
          printf( "glCheckFramebufferStatus unspecified error\n" );
        }
        // return false;
      }
    }
    glBindFramebuffer( GL_FRAMEBUFFER, 0 );
  }

  glClearColor( 0.2, 0.2, 0.2, 1.0 );
  while ( !glfwWindowShouldClose( g_window ) ) {
    glfwPollEvents();

    glBindFramebuffer( GL_FRAMEBUFFER, fb );
    glViewport( 0, 0, g_fb_width, g_fb_height );
    glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

    glUseProgram( program );
    glBindVertexArray( vertex_array_gl );
    for ( int i = 0; i < NIMAGES; i++ ) {
      glUniform1f( u_idx, (float)i );
      glDrawArrays( GL_TRIANGLES, 0, 3 );
    }
    glBindVertexArray( 0 );
    glUseProgram( 0 );
    glBindFramebuffer( GL_FRAMEBUFFER, 0 );
    glViewport( 0, 0, g_fb_width, g_fb_height );
    glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

    { // blit fb
      glBindFramebuffer( GL_READ_FRAMEBUFFER, fb );
      glBindFramebuffer( GL_DRAW_FRAMEBUFFER, 0 );
      glBlitFramebuffer( 0, 0, g_fb_width, g_fb_height, 0, 0, g_fb_width, g_fb_height, GL_COLOR_BUFFER_BIT, GL_NEAREST );
      glBindFramebuffer( GL_FRAMEBUFFER, 0 );
    }

    glfwSwapBuffers( g_window );
  }

  glDeleteProgram( program );
  glDeleteBuffers( 1, &vertex_buffer_gl );
  glDeleteVertexArrays( 1, &vertex_array_gl );
  glfwTerminate();
  printf( "program HALT\n" );

  return 0;
}
