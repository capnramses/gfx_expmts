// This is just to test the actual coordinate space of gl_FragCoord.xy as used in the fragment shader.
// Window is 800x600
// viewport is half that
// framebuffer may differ on e.g. retina.
// -> it seems to be window rather than viewport dims. It does state window in the docs.

#include <glad/gl.h>
#include <GLFW/glfw3.h>
#include <stdbool.h>
#include <stdio.h>
#define APG_IMPLEMENTATION
#include "apg.h"

void error_callback_glfw( int error, const char* description ) {
 fprintf( stderr, "GLFW ERROR: code %i msg: %s.\n", error, description );
}

int main( void ) {
  // Start OpenGL context and OS window using the GLFW helper library.
  // Register the error callback function that we wrote earlier.
  glfwSetErrorCallback( error_callback_glfw );

  if ( !glfwInit() ) {
    fprintf( stderr, "ERROR: could not start GLFW3.\n" );
    return 1;
  }

  // Request an OpenGL 4.1, core, context from GLFW.
  glfwWindowHint( GLFW_CONTEXT_VERSION_MAJOR, 4 );
  glfwWindowHint( GLFW_CONTEXT_VERSION_MINOR, 1 );
  glfwWindowHint( GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE );
  glfwWindowHint( GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE );

  glfwWindowHint( GLFW_SAMPLES, 4 );

  // Set this to false to go back to windowed mode.
  bool full_screen = false; // NB. include stdbool.h for bool in C.

  GLFWmonitor* mon = NULL;
  int win_w = 800, win_h = 600; // Our window dimensions, in pixels.

  if ( full_screen ) {
    mon = glfwGetPrimaryMonitor();

    const GLFWvidmode* mode = glfwGetVideoMode( mon );

    // Hinting these properties lets us use "borderless" full screen" mode.
    glfwWindowHint( GLFW_RED_BITS, mode->redBits );
    glfwWindowHint( GLFW_GREEN_BITS, mode->greenBits );
    glfwWindowHint( GLFW_BLUE_BITS, mode->blueBits );
    glfwWindowHint( GLFW_REFRESH_RATE, mode->refreshRate );

    win_w = mode->width;  // Use our 'desktop' resolution for window size
    win_h = mode->height; // to get a 'full screen borderless' window.
  }

  GLFWwindow* window = glfwCreateWindow( win_w, win_h, "Extended OpenGL Init", mon, NULL );

  if ( !window ) {
    fprintf( stderr, "ERROR: Could not open window with GLFW3.\n" );
    glfwTerminate();
    return 1;
  }
  glfwMakeContextCurrent( window );

  glfwSwapInterval( 0 );

  // Start Glad, so we can call OpenGL functions.
  int version_glad = gladLoadGL( glfwGetProcAddress );
  if ( version_glad == 0 ) {
    fprintf( stderr, "ERROR: Failed to initialize OpenGL context.\n" );
    return 1;
  }
  printf( "Loaded OpenGL %i.%i\n", GLAD_VERSION_MAJOR( version_glad ), GLAD_VERSION_MINOR( version_glad ) );

  // Try to call some OpenGL functions, and print some more version info.
  printf( "Renderer: %s.\n", glGetString( GL_RENDERER ) );
  printf( "OpenGL version %s.\n", glGetString( GL_VERSION ) );

  float points[] = { 0.0f, 0.5f, 0.0f, 0.5f, -0.5f, 0.0f, -0.5f, -0.5f, 0.0f };
  GLuint vbo     = 0;
  glGenBuffers( 1, &vbo );
  glBindBuffer( GL_ARRAY_BUFFER, vbo );
  glBufferData( GL_ARRAY_BUFFER, 9 * sizeof( float ), points, GL_STATIC_DRAW );
  GLuint vao = 0;
  glGenVertexArrays( 1, &vao );
  glBindVertexArray( vao );
  glEnableVertexAttribArray( 0 );
  glBindBuffer( GL_ARRAY_BUFFER, vbo );
  glVertexAttribPointer( 0, 3, GL_FLOAT, GL_FALSE, 0, NULL );

  int64_t max_shader_chars = APG_MEGABYTES( 4 );
  char* vertex_shader      = malloc( max_shader_chars );
  char* fragment_shader    = malloc( max_shader_chars );
  bool ret                 = apg_file_to_str( "shader.vert", max_shader_chars, vertex_shader );
  assert( ret && "vertex shader load fail" );

  ret = apg_file_to_str( "shader.frag", max_shader_chars, fragment_shader );
  assert( ret && "frag shader load fail" );

  GLuint vs = glCreateShader( GL_VERTEX_SHADER );
  glShaderSource( vs, 1, (const char* const*)&vertex_shader, NULL );
  glCompileShader( vs );

  // After glCompileShader check for errors.
  int params = GL_FALSE;
  glGetShaderiv( vs, GL_COMPILE_STATUS, &params );

  // On error, capture the log and print it.
  if ( GL_TRUE != params ) {
    int max_length = 2048, actual_length = 0;
    char slog[2048];
    glGetShaderInfoLog( vs, max_length, &actual_length, slog );
    fprintf( stderr, "ERROR: Shader index %u did not compile.\n%s\n", vs, slog );
    return 1;
  }

  GLuint fs = glCreateShader( GL_FRAGMENT_SHADER );
  glShaderSource( fs, 1, (const char* const*)&fragment_shader, NULL );
  glCompileShader( fs );

  // After glCompileShader check for errors.
  params = GL_FALSE;
  glGetShaderiv( fs, GL_COMPILE_STATUS, &params );

  // On error, capture the log and print it.
  if ( GL_TRUE != params ) {
    int max_length = 2048, actual_length = 0;
    char slog[2048];
    glGetShaderInfoLog( fs, max_length, &actual_length, slog );
    fprintf( stderr, "ERROR: Shader index %u did not compile.\n%s\n", fs, slog );
    return 1;
  }

  GLuint shader_program = glCreateProgram();
  glAttachShader( shader_program, fs );
  glAttachShader( shader_program, vs );
  glLinkProgram( shader_program );
  glDeleteShader( vs );
  glDeleteShader( fs );
  free( vertex_shader );
  free( fragment_shader );

  // Check for linking errors:
  glGetProgramiv( shader_program, GL_LINK_STATUS, &params );

  // Print the linking log:
  if ( GL_TRUE != params ) {
    int max_length = 2048, actual_length = 0;
    char plog[2048];
    glGetProgramInfoLog( shader_program, max_length, &actual_length, plog );
    fprintf( stderr, "ERROR: Could not link shader program GL index %u.\n%s\n", shader_program, plog );
    return 1;
  }

  double prev_s            = glfwGetTime(); // Set the initial 'previous time'.
  double title_countdown_s = 0.2;
  while ( !glfwWindowShouldClose( window ) ) {
    double curr_s    = glfwGetTime();   // Get the current time.
    double elapsed_s = curr_s - prev_s; // Work out the time elapsed over the last frame.
    prev_s           = curr_s;          // Set the 'previous time' for the next frame to use.

    // Print the FPS, but not every frame, so it doesn't flicker too much.
    title_countdown_s -= elapsed_s;
    if ( title_countdown_s <= 0.0 && elapsed_s > 0.0 ) {
      double fps = 1.0 / elapsed_s;

      // Create a string and put the FPS as the window title.
      char tmp[256];
      sprintf( tmp, "FPS %.2lf", fps );
      glfwSetWindowTitle( window, tmp );
      title_countdown_s = 0.2;
    }
    glfwPollEvents();
    if ( GLFW_PRESS == glfwGetKey( window, GLFW_KEY_ESCAPE ) ) { glfwSetWindowShouldClose( window, 1 ); }

    // Check if the window resized.
    glfwGetWindowSize( window, &win_w, &win_h );
    int fb_w = 0, fb_h = 0;
    glfwGetFramebufferSize( window, &fb_w, &fb_h );
   // printf( "win %i,%i -- fb %i,%i\n", win_w, win_h, fb_w, fb_h );

    glClearColor( 0.6f, 0.6f, 0.8f, 1.0f );
    glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

    glUseProgram( shader_program );
    glProgramUniform2i( shader_program, glGetUniformLocation( shader_program, "win_dims" ), win_w, win_h );
    glProgramUniform2i( shader_program, glGetUniformLocation( shader_program, "fb_dims" ), fb_w, fb_h );
    glProgramUniform1f( shader_program, glGetUniformLocation( shader_program, "time" ), (float)curr_s );
    glBindVertexArray( vao );

    glViewport( 0, 0, win_w, win_h );
    glDrawArrays( GL_TRIANGLES, 0, 3 );

    // Put the stuff we've been drawing onto the visible area.
    glfwSwapBuffers( window );
  }

  glDeleteProgram( shader_program );
  glDeleteVertexArrays( 1, &vao );
  glDeleteBuffers( 1, &vbo );

  // Close OpenGL window, context, and any other GLFW resources.
  glfwTerminate();
  return 0;
}
