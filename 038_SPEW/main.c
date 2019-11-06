
// note: glfw has functions for proc hookup and extension check across different
// platforms

#include "spew.h"
#include "GLFW/glfw3.h"
#include <stdio.h>
#include <stdbool.h>

// sets up function pointers for opengl 4.1 core
GLFWwindow* window;

int main() {
  printf( "hello world!\n" );
  { // start GLFW and create a context
    if ( !glfwInit() ) {
      printf( "ERROR: starting GLFW\n" );
      return 1;
    }
    glfwWindowHint( GLFW_CONTEXT_VERSION_MAJOR, 4 );
    glfwWindowHint( GLFW_CONTEXT_VERSION_MINOR, 1 );
    glfwWindowHint( GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE );
    glfwWindowHint( GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE );
    window = glfwCreateWindow( 640, 480, "Hello Triangle", NULL, NULL );
    if ( !window ) {
      fprintf( stderr, "ERROR: could not open window with GLFW3\n" );
      glfwTerminate();
      return 1;
    }
    glfwMakeContextCurrent( window );
  }

  printf( "SPEWing...\n" );
  // set up reqd function pointers
  if ( !spew_fptrs( glfwGetProcAddress, glfwExtensionSupported ) ) {
    printf( "SPEW FAILED\n" );
    return false;
  }

  {
    const GLubyte* renderer = glGetString( GL_RENDERER );
    const GLubyte* version  = glGetString( GL_VERSION );
    printf( "Renderer: %s\n", renderer );
    printf( "OpenGL version supported %s\n", version );
  }

  { // geom
    GLfloat points[] = { 0.0f, 0.5f, 0.0f, 0.5f, -0.5f, 0.0f, -0.5f, -0.5f, 0.0f };
    GLuint vbo;
    glGenBuffers( 1, &vbo );
    glBindBuffer( GL_ARRAY_BUFFER, vbo );
    glBufferData( GL_ARRAY_BUFFER, 9 * sizeof( GLfloat ), points, GL_STATIC_DRAW );
    GLuint vao;
    glGenVertexArrays( 1, &vao );
    glBindVertexArray( vao );
    glEnableVertexAttribArray( 0 );
    glBindBuffer( GL_ARRAY_BUFFER, vbo );
    glVertexAttribPointer( 0, 3, GL_FLOAT, GL_FALSE, 0, NULL );
  }
  { // shaders
    const char* vertex_shader =
      "#version 410\n"
      "in vec3 vp;"
      "void main() {"
      "	gl_Position = vec4( vp, 1.0 );"
      "}";
    const char* fragment_shader =
      "#version 410\n"
      "out vec4 frag_colour;"
      "void main() {"
      "	frag_colour = vec4( 0.5, 0.0, 0.5, 1.0 );"
      "}";
    GLuint vert_shader = glCreateShader( GL_VERTEX_SHADER );
    glShaderSource( vert_shader, 1, &vertex_shader, NULL );
    glCompileShader( vert_shader );
    GLuint frag_shader = glCreateShader( GL_FRAGMENT_SHADER );
    glShaderSource( frag_shader, 1, &fragment_shader, NULL );
    glCompileShader( frag_shader );
    GLuint shader_programme = glCreateProgram();
    glAttachShader( shader_programme, frag_shader );
    glAttachShader( shader_programme, vert_shader );
    glLinkProgram( shader_programme );
    glUseProgram( shader_programme );
  }
  glClearColor( 0.4, 0.2, 0.2, 1.0 );
  while ( true ) { // drawing loop
    glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
    glDrawArrays( GL_TRIANGLES, 0, 3 );
    glfwPollEvents();
    glfwSwapBuffers( window );
  }
  glfwTerminate();
  return 0;
}
