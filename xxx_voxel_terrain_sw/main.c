// An implementation following:
// 
// Voxel Space (Comanche Terrain Rendering) https://www.youtube.com/watch?v=bQBY9BM9g_Y


#include <assert.h>
#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <glad/gl.h>
#include <GLFW/glfw3.h>
#define APG_IMPLEMENTATION
#include "apg.h"


#define SCREEN_WIDTH 320 
#define SCREEN_HEIGHT 200
#define MAP_N 1024
#define SCALE_FACTOR 120.0
ver: The Voxel Space engine. This technique was used to render the terrain in the Comanche: Maximum Overkill game.

  float x, y, height, zfar, attitude, heading;
} cam_t;

void ppm_write( const char* fn, img_t img ) {
  FILE* f = fopen( fn, "wb" );
  fprintf( f, "P6\n%i %i\n255\n", img.w, img.h );
  for ( int i = 0; i < img.w * img.h; i++ ) {
    int pi         = img.c_ptr[i];
    uint8_t rgb[3] = { img.pal[pi * 3 + 0], img.pal[pi * 3 + 1], img.pal[pi * 3 + 2] };
    fwrite( rgb, 3, 1, f );
  }
  fclose( f );

  f = fopen( "out_pal.ppm", "wb" );
  fprintf( f, "P6\n16 16\n255\n" );
  fwrite( img.pal, 256 * 3, 1, f );
  fclose( f );

  if ( img.h_ptr ) {
    f = fopen( "out_hm.ppm", "wb" );
    fprintf( f, "P5\n%i %i\n255\n", img.w, img.h );
    fwrite( img.h_ptr, img.w * img.h, 1, f );
    fclose( f );
  }
}

img_t read_cmap( const char* fn_c, const char* fn_p, const char* fn_h ) {
  img_t img = (img_t){ .w = MAP_N, .h = MAP_N };
  FILE* fc  = fopen( fn_c, "rb" );
  assert( fc );
  img.c_ptr = malloc( MAP_N * MAP_N );
  fread( img.c_ptr, MAP_N * MAP_N, 1, fc );
  fclose( fc );
  FILE* fp = fopen( fn_p, "rb" );
  assert( fp );
  fread( img.pal, 768, 1, fp );
  fclose( fp );
  FILE* fh = fopen( fn_h, "rb" );
  assert( fh );
  img.h_ptr = malloc( MAP_N * MAP_N );
  fread( img.h_ptr, MAP_N * MAP_N, 1, fh );
  fclose( fh );
  return img;
}

GLFWwindow* window;
GLuint shader_program, vao, col_texture_handle, pal_texture_handle;
int win_w = 320 * 4, win_h = 200 * 4;

void start_gl( void ) {
  bool gret = glfwInit();
  assert( gret );
  glfwWindowHint( GLFW_CONTEXT_VERSION_MAJOR, 4 );
  glfwWindowHint( GLFW_CONTEXT_VERSION_MINOR, 1 );
  glfwWindowHint( GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE );
  glfwWindowHint( GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE );
  GLFWmonitor* mon = NULL;
  window           = glfwCreateWindow( win_w, win_h, "Voxel Terrain", mon, NULL );
  assert( window );
  glfwMakeContextCurrent( window );
  glfwSwapInterval( 0 );
  //
  int version_glad = gladLoadGL( glfwGetProcAddress );
  assert( version_glad );
  //
  float points[] = { 0.0f, 0.5f, 0.0f, 0.5f, -0.5f, 0.0f, -0.5f, -0.5f, 0.0f };
  GLuint vbo     = 0;
  glGenBuffers( 1, &vbo );
  glBindBuffer( GL_ARRAY_BUFFER, vbo );
  glBufferData( GL_ARRAY_BUFFER, 9 * sizeof( float ), points, GL_STATIC_DRAW );
  glGenVertexArrays( 1, &vao );
  glBindVertexArray( vao );
  glEnableVertexAttribArray( 0 );
  glBindBuffer( GL_ARRAY_BUFFER, vbo );
  glVertexAttribPointer( 0, 3, GL_FLOAT, GL_FALSE, 0, NULL );
  //
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
  int params = -1;
  glGetShaderiv( vs, GL_COMPILE_STATUS, &params );
  if ( GL_TRUE != params ) {
    int max_length = 2048, actual_length = 0;
    char slog[2048];
    glGetShaderInfoLog( vs, max_length, &actual_length, slog );
    fprintf( stderr, "ERROR: Shader index %u did not compile.\n%s\n", vs, slog );
    assert( false );
  }
  GLuint fs = glCreateShader( GL_FRAGMENT_SHADER );
  glShaderSource( fs, 1, (const char* const*)&fragment_shader, NULL );
  glCompileShader( fs );
  params = -1;
  glGetShaderiv( vs, GL_COMPILE_STATUS, &params );
  if ( GL_TRUE != params ) {
    int max_length = 2048, actual_length = 0;
    char slog[2048];
    glGetShaderInfoLog( fs, max_length, &actual_length, slog );
    assert( false );
  }
  shader_program = glCreateProgram();
  glAttachShader( shader_program, fs );
  glAttachShader( shader_program, vs );
  glLinkProgram( shader_program );
  glDeleteShader( vs );
  glDeleteShader( fs );
  free( vertex_shader );
  free( fragment_shader );
  glGetProgramiv( shader_program, GL_LINK_STATUS, &params );
  if ( GL_TRUE != params ) {
    int max_length = 2048, actual_length = 0;
    char plog[2048];
    glGetProgramInfoLog( shader_program, max_length, &actual_length, plog );
    fprintf( stderr, "ERROR: Could not link shader program GL index %u.\n%s\n", shader_program, plog );
    assert( false );
  }
  //
  glGenTextures( 1, &col_texture_handle );
  glGenTextures( 1, &pal_texture_handle );
  glActiveTexture( GL_TEXTURE0 );
  glBindTexture( GL_TEXTURE_2D, col_texture_handle );
  glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
  glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );
  glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
  glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
  glBindTexture( GL_TEXTURE_2D, pal_texture_handle );
  glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
  glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );
  glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
  glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
}


int main() {
  start_gl();

  img_t maps = read_cmap( "map0.color", "map0.palette", "map0.height" );
  ppm_write( "out_cm.ppm", maps ); // Debug output of maps.
  img_t fb = (img_t){ .w = SCREEN_WIDTH, .h = SCREEN_HEIGHT };
  fb.c_ptr = calloc( fb.w * fb.h, 1 );
  memcpy( fb.pal, maps.pal, 256 * 3 );

  glActiveTexture( GL_TEXTURE0 );
  glBindTexture( GL_TEXTURE_2D, pal_texture_handle );
  glPixelStorei( GL_UNPACK_ALIGNMENT, 1 );
  glTexImage2D( GL_TEXTURE_2D, 0, GL_RGB, 256, 1, 0, GL_RGB, GL_UNSIGNED_BYTE, fb.pal );
  glBindTexture( GL_TEXTURE_2D, 0 );

  /** do a background sky gradient */

  cam_t cam             = (cam_t){ .x = 512, .y = 512, .height = 70.0f, .zfar = 1600.0f, .attitude = 60.0f, .heading = 1.5 * 3.141592 }; // (= 270 deg) };
  float terrain_h_scale = SCALE_FACTOR;

  double prev_s            = glfwGetTime();
  double title_countdown_s = 0.2;
  while ( !glfwWindowShouldClose( window ) ) {
    double curr_s    = glfwGetTime();
    double elapsed_s = curr_s - prev_s;
    prev_s           = curr_s;
    title_countdown_s -= elapsed_s;
    if ( title_countdown_s <= 0.0 && elapsed_s > 0.0 ) {
      double fps = 1.0 / elapsed_s;
      char tmp[256];
      sprintf( tmp, "FPS %.2lf", fps );
      glfwSetWindowTitle( window, tmp );
      title_countdown_s = 0.2;
    }
    glfwPollEvents();
    if ( GLFW_PRESS == glfwGetKey( window, GLFW_KEY_ESCAPE ) ) { glfwSetWindowShouldClose( window, 1 ); }
    glfwGetWindowSize( window, &win_w, &win_h );
    int fb_w = 0, fb_h = 0;
    glfwGetFramebufferSize( window, &fb_w, &fb_h );

    if ( glfwGetKey( window, GLFW_KEY_W ) ) {
      cam.x += cos( cam.heading ) * elapsed_s * 100.0;
      cam.y += sin( cam.heading ) * elapsed_s * 100.0;
    }
    if ( glfwGetKey( window, GLFW_KEY_S ) ) {
      cam.x -= cos( cam.heading ) * elapsed_s * 100.0;
      cam.y -= sin( cam.heading ) * elapsed_s * 100.0;
    }
    if ( glfwGetKey( window, GLFW_KEY_A ) ) {
      cam.x -= cos( cam.heading + 1.57 ) * elapsed_s * 100.0;
      cam.y -= sin( cam.heading  + 1.57) * elapsed_s * 100.0;
    }
    if ( glfwGetKey( window, GLFW_KEY_D ) ) {
      cam.x += cos( cam.heading  + 1.57) * elapsed_s * 100.0;
      cam.y += sin( cam.heading  + 1.57) * elapsed_s * 100.0;
    }
    if ( glfwGetKey( window, GLFW_KEY_LEFT ) ) { cam.heading -= 1.0 * elapsed_s; }
    if ( glfwGetKey( window, GLFW_KEY_RIGHT ) ) { cam.heading += 1.0 * elapsed_s; }
    if ( glfwGetKey( window, GLFW_KEY_E ) ) { cam.height += 100.0f * elapsed_s; }
    if ( glfwGetKey( window, GLFW_KEY_Q ) ) { cam.height -= 100.0f * elapsed_s; }
    if ( glfwGetKey( window, GLFW_KEY_UP ) ) { cam.attitude += 150.0 * elapsed_s; }
    if ( glfwGetKey( window, GLFW_KEY_DOWN ) ) { cam.attitude -= 150.0 * elapsed_s; }


////////////////////////////////////////////////////////////
    memset( fb.c_ptr, 0, SCREEN_WIDTH * SCREEN_HEIGHT );

    float sinangle = sin(cam.heading);
    float cosangle = cos(cam.heading);
    // Left-most point of the FOV
    float plx = cosangle * cam.zfar + sinangle * cam.zfar;
    float ply = sinangle * cam.zfar - cosangle * cam.zfar;

    // Right-most point of the FOV
    float prx = cosangle * cam.zfar - sinangle * cam.zfar;
    float pry = sinangle * cam.zfar + cosangle * cam.zfar;

    for (int i = 0; i < SCREEN_WIDTH; i++) {
      float delta_x   = ( plx + ( prx - plx ) / SCREEN_WIDTH * i ) / cam.zfar;
      float delta_y   = ( ply + ( pry - ply ) / SCREEN_WIDTH * i ) / cam.zfar;
      float ray_x     = cam.x;
      float ray_y     = cam.y;
      float tallest_h = SCREEN_HEIGHT;

      for ( int z = 1; z < cam.zfar; z++ ) {
        ray_x += delta_x;
        ray_y += delta_y; // -= to face the other way around (probably what we want)

        // ensure ray_x and ray_y are < map bounds by AND with 1023.
        int map_idx = ( ( maps.w * ( (int)( ray_y ) & ( maps.w - 1 ) ) ) + ( (int)( ray_x ) & ( maps.w - 1 ) ) );

 
        // AARRGHHH my error was /cam.zfar here instead of /z --->>>>> CONFUSING VARIABLE NAMES!!!!!!!!
        int projheight = (int)((cam.height - maps.h_ptr[map_idx]) / z * SCALE_FACTOR + cam.attitude);

        // Only draw pixels if the new projected height is taller than the previous tallest height
        if (projheight < tallest_h) {
          // Draw pixels from previous max-height until the new projected height
          for (int y = projheight; y < tallest_h; y++) {
            if (y >= 0) { fb.c_ptr[(SCREEN_WIDTH * y) + i] = maps.c_ptr[map_idx]; }
          }
          tallest_h = projheight;
        }

        // debug cone of view
        // int fb_idx = (fb.w * (int)( ray_y/ 6 ))  + (int)( ray_x / 6 );
        // printf( "fb_idx %i\n", fb_idx );
        // assert( fb_idx < fb.w * fb.h );
        //  fb.c_ptr[fb_idx] = (uint8_t)0x19;
      }
    }

    glClearColor( 0.6f, 0.6f, 0.8f, 1.0f );
    glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

    glViewport( 0, 0, win_w, win_h );
    glUseProgram( shader_program );
    glProgramUniform2i( shader_program, glGetUniformLocation( shader_program, "win_dims" ), win_w, win_h );
    glProgramUniform2i( shader_program, glGetUniformLocation( shader_program, "fb_dims" ), fb_w, fb_h );
    glProgramUniform1i( shader_program, glGetUniformLocation( shader_program, "u_col_tex" ), 0 );
    glProgramUniform1i( shader_program, glGetUniformLocation( shader_program, "u_pal_tex" ), 1 );

    glBindVertexArray( vao );
    glActiveTexture( GL_TEXTURE0 );
    glBindTexture( GL_TEXTURE_2D, col_texture_handle );
    glPixelStorei( GL_UNPACK_ALIGNMENT, 1 );
    glTexImage2D( GL_TEXTURE_2D, 0, GL_RED, fb.w, fb.h, 0, GL_RED, GL_UNSIGNED_BYTE, fb.c_ptr );
    glActiveTexture( GL_TEXTURE1 );
    glBindTexture( GL_TEXTURE_2D, pal_texture_handle );

    glDrawArrays( GL_TRIANGLES, 0, 3 );

    glfwSwapBuffers( window );
  } // endwhile

  ppm_write( "output.ppm", fb );
  free( fb.c_ptr );
  free( maps.c_ptr );
  free( maps.h_ptr );

  glfwTerminate();
  return 0;
}
