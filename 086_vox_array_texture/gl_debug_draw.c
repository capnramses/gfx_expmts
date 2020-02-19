//
// OpenGL Debug Draw
// First v. Anton Gerdelan 11 June 2015
// feature complete v. original spec: 16 June 2015
// https://github.com/capnramses/opengl_debug_draw
//

#include "gl_debug_draw.h"
#include "glcontext.h"
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <stdint.h>
#include <string.h> // memset
#include <math.h>
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// current num of debug lines in buffer
unsigned int g_count_gl_db_lines;
// vao for drawing properties of lines
GLuint g_gl_db_lines_vao;
// handle to GPU buffer
GLuint g_gl_db_lines_vbo;
// shader prog for lines
GLuint g_gl_db_lines_sp;
// camera matrix uniform id
GLint g_gl_db_PV_loc = -1;

void _print_shader_info_log( unsigned int shader_index ) {
  int max_length    = 2048;
  int actual_length = 0;
  char log[2048];
  glGetShaderInfoLog( shader_index, max_length, &actual_length, log );
  printf( "shader info log for GL index %u\n%s\n", shader_index, log );
}

void _print_programme_info_log( unsigned int m_programme_idx ) {
  int max_length    = 2048;
  int actual_length = 0;
  char log[2048];
  glGetProgramInfoLog( m_programme_idx, max_length, &actual_length, log );
  printf( "program info log for GL index %u\n%s\n", (int)m_programme_idx, log );
}

const char* gl_db_lines_vs_str =
  "#version 120\n"
  "attribute vec3 vp;\n"
  "attribute vec4 vc;\n"
  "uniform mat4 PV;\n"
  "varying vec4 fc;\n"
  "void main () {\n"
  "  gl_Position = PV * vec4 (vp, 1.0);\n"
  "  fc = vc;\n"
  "}";

const char* gl_db_lines_fs_str =
  "#version 120\n"
  "varying vec4 fc;\n"
  "void main () {\n"
  "  gl_FragColor = fc;\n"
  "}";

//
// reserve memory for drawing stuff
bool init_gl_db() {
  // vao for drawing properties of lines
  glGenVertexArrays( 1, &g_gl_db_lines_vao );
  glBindVertexArray( g_gl_db_lines_vao );

  // create GPU-side buffer
  // size is 32-bits for GLfloat * num lines * 7 comps per vert * 2 per lines
  glGenBuffers( 1, &g_gl_db_lines_vbo );
  glBindBuffer( GL_ARRAY_BUFFER, g_gl_db_lines_vbo );
  glBufferData( GL_ARRAY_BUFFER, 4 * MAX_APG_GL_DB_LINES * 14, NULL, GL_DYNAMIC_DRAW );

  GLsizei stride = 4 * 7;
  GLintptr offs  = 4 * 3;
  glVertexAttribPointer( 0, 3, GL_FLOAT, GL_FALSE, stride, NULL );          // point
  glVertexAttribPointer( 1, 4, GL_FLOAT, GL_FALSE, stride, (GLvoid*)offs ); // colour
  glEnableVertexAttribArray( 0 );                                           // point
  glEnableVertexAttribArray( 1 );                                           // colour

  GLuint vs = glCreateShader( GL_VERTEX_SHADER );
  GLuint fs = glCreateShader( GL_FRAGMENT_SHADER );
  glShaderSource( vs, 1, (const char**)&gl_db_lines_vs_str, NULL );
  glCompileShader( vs );
  _print_shader_info_log( vs );
  glShaderSource( fs, 1, (const char**)&gl_db_lines_fs_str, NULL );
  glCompileShader( fs );
  _print_shader_info_log( fs );
  g_gl_db_lines_sp = glCreateProgram();
  glAttachShader( g_gl_db_lines_sp, fs );
  glAttachShader( g_gl_db_lines_sp, vs );
  glBindAttribLocation( g_gl_db_lines_sp, 0, "vp" );
  glBindAttribLocation( g_gl_db_lines_sp, 1, "vc" );
  glLinkProgram( g_gl_db_lines_sp );
  _print_programme_info_log( g_gl_db_lines_sp );
  g_gl_db_PV_loc = glGetUniformLocation( g_gl_db_lines_sp, "PV" );
  assert( g_gl_db_PV_loc >= -1 );
  // flag that shaders can be deleted whenever the program is deleted
  glDeleteShader( fs );
  glDeleteShader( vs );
  float PV[16];
  memset( PV, 0, 16 * sizeof( float ) );
  PV[0] = PV[5] = PV[10] = PV[15] = 1.0f;
  glUseProgram( g_gl_db_lines_sp );
  glUniformMatrix4fv( g_gl_db_PV_loc, 1, GL_FALSE, PV );

  return true;
}

//
// free memory
void free_gl_db() {
  glDeleteBuffers( 1, &g_gl_db_lines_vbo );
  glDeleteVertexArrays( 1, &g_gl_db_lines_vao );
  // attached shaders have prev been flagged to delete so will also be deleted
  glDeleteProgram( g_gl_db_lines_sp );
}

//
// add a new debug line to world space coords
// params are 2 arrays of 3 floats
// return index corresponding to internal memory offset
int add_gl_db_line( float* start_xyz, float* end_xyz, float* colour_rgba ) {
  if ( g_count_gl_db_lines >= MAX_APG_GL_DB_LINES ) {
    fprintf( stderr, "ERROR: too many gl db lines\n" );
    return -1;
  }

  float sd[14];
  sd[0]  = start_xyz[0];
  sd[1]  = start_xyz[1];
  sd[2]  = start_xyz[2];
  sd[3]  = colour_rgba[0];
  sd[4]  = colour_rgba[1];
  sd[5]  = colour_rgba[2];
  sd[6]  = colour_rgba[3];
  sd[7]  = end_xyz[0];
  sd[8]  = end_xyz[1];
  sd[9]  = end_xyz[2];
  sd[10] = colour_rgba[0];
  sd[11] = colour_rgba[1];
  sd[12] = colour_rgba[2];
  sd[13] = colour_rgba[3];

  glBindBuffer( GL_ARRAY_BUFFER, g_gl_db_lines_vbo );
  GLintptr os = sizeof( sd ) * g_count_gl_db_lines;
  GLsizei sz  = sizeof( sd );
  glBufferSubData( GL_ARRAY_BUFFER, os, sz, sd );

  return g_count_gl_db_lines++;
}

//
// a line with a colour that goes from black to coloured in direction of norm
int add_gl_db_normal( float* n_xyz, float* pos_xyz, float scale, float* colour_rgba ) {
  if ( g_count_gl_db_lines >= MAX_APG_GL_DB_LINES ) {
    fprintf( stderr, "ERROR: too many gl db lines\n" );
    return -1;
  }
  float end[3];
  end[0] = pos_xyz[0] + n_xyz[0] * scale;
  end[1] = pos_xyz[1] + n_xyz[1] * scale;
  end[2] = pos_xyz[2] + n_xyz[2] * scale;

  float sd[14];
  sd[0]  = pos_xyz[0];
  sd[1]  = pos_xyz[1];
  sd[2]  = pos_xyz[2];
  sd[3]  = 0.0f;
  sd[4]  = 0.0f;
  sd[5]  = 0.0f;
  sd[6]  = 1.0f;
  sd[7]  = end[0];
  sd[8]  = end[1];
  sd[9]  = end[2];
  sd[10] = colour_rgba[0];
  sd[11] = colour_rgba[1];
  sd[12] = colour_rgba[2];
  sd[13] = colour_rgba[3];

  glBindBuffer( GL_ARRAY_BUFFER, g_gl_db_lines_vbo );
  GLintptr os = sizeof( sd ) * g_count_gl_db_lines;
  GLsizei sz  = sizeof( sd );
  glBufferSubData( GL_ARRAY_BUFFER, os, sz, sd );

  return g_count_gl_db_lines++;
}

//
// 3 lines in a cross with a colour to show a position
// returns the first of 3 contiguous line ids
int add_gl_db_pos( float* pos_xyz, float scale, float* colour_rgba ) {
  int rid = -1;
  float start[3], end[3];

  start[0] = pos_xyz[0] - scale;
  start[1] = pos_xyz[1];
  start[2] = pos_xyz[2];
  end[0]   = pos_xyz[0] + scale;
  end[1]   = pos_xyz[1];
  end[2]   = pos_xyz[2];
  rid      = add_gl_db_line( start, end, colour_rgba );
  start[0] = pos_xyz[0];
  start[1] = pos_xyz[1] - scale;
  start[2] = pos_xyz[2];
  end[0]   = pos_xyz[0];
  end[1]   = pos_xyz[1] + scale;
  end[2]   = pos_xyz[2];
  add_gl_db_line( start, end, colour_rgba );
  start[0] = pos_xyz[0];
  start[1] = pos_xyz[1];
  start[2] = pos_xyz[2] - scale;
  end[0]   = pos_xyz[0];
  end[1]   = pos_xyz[1];
  end[2]   = pos_xyz[2] + scale;
  add_gl_db_line( start, end, colour_rgba );

  return rid;
}

//
// draw box for this aabb
// returns the first of 12 contiguous line ids
int add_gl_db_aabb( float* min_xyz, float* max_xyz, float* colour_rgba ) {
  int rid = -1;
  float start[3], end[3];

  // bottom ring
  // rear
  start[0] = min_xyz[0];          // L
  end[0]   = max_xyz[0];          // R
  start[1] = end[1] = min_xyz[1]; // B
  start[2] = end[2] = min_xyz[2]; // P
  rid               = add_gl_db_line( start, end, colour_rgba );
  // right
  start[0] = max_xyz[0]; // R
  end[2]   = max_xyz[2]; // A
  add_gl_db_line( start, end, colour_rgba );
  // front
  end[0]   = min_xyz[0]; // L
  start[2] = max_xyz[2]; // A
  add_gl_db_line( start, end, colour_rgba );
  // left
  start[0] = min_xyz[0]; // L
  end[2]   = min_xyz[2]; // P
  add_gl_db_line( start, end, colour_rgba );

  // top ring
  start[0] = min_xyz[0];          // L
  end[0]   = max_xyz[0];          // R
  start[1] = end[1] = max_xyz[1]; // T
  start[2] = end[2] = min_xyz[2]; // P
  add_gl_db_line( start, end, colour_rgba );
  // right
  start[0] = max_xyz[0]; // R
  end[2]   = max_xyz[2]; // A
  add_gl_db_line( start, end, colour_rgba );
  // front
  end[0]   = min_xyz[0]; // L
  start[2] = max_xyz[2]; // A
  add_gl_db_line( start, end, colour_rgba );
  // left
  start[0] = min_xyz[0]; // L
  end[2]   = min_xyz[2]; // P
  add_gl_db_line( start, end, colour_rgba );

  // 4 side edges
  start[0] = end[0] = min_xyz[0]; // L
  start[1]          = min_xyz[1];
  end[1]            = max_xyz[1];
  start[2] = end[2] = min_xyz[2]; // P
  add_gl_db_line( start, end, colour_rgba );
  start[0] = end[0] = max_xyz[0]; // R
  add_gl_db_line( start, end, colour_rgba );
  start[2] = end[2] = max_xyz[2]; // A
  add_gl_db_line( start, end, colour_rgba );
  start[0] = end[0] = min_xyz[0]; // L
  add_gl_db_line( start, end, colour_rgba );

  return rid;
}

//
// draw a circle+radius to represent a sphere
// returns first of 39 line ids
int add_gl_db_rad_circle( float* centre_xyz, float radius, float* colour_rgba ) {
  int rid = -1;
  float start[3], end[3];
  // 3 radius lines in a cross first
  start[0] = centre_xyz[0];
  start[1] = centre_xyz[1] - radius;
  start[2] = centre_xyz[2];
  end[0]   = centre_xyz[0];
  end[1]   = centre_xyz[1] + radius;
  end[2]   = centre_xyz[2];
  rid      = add_gl_db_line( start, end, colour_rgba );
  start[0] = centre_xyz[0] - radius;
  start[1] = centre_xyz[1];
  start[2] = centre_xyz[2];
  end[0]   = centre_xyz[0] + radius;
  end[1]   = centre_xyz[1];
  end[2]   = centre_xyz[2];
  add_gl_db_line( start, end, colour_rgba );
  start[0] = centre_xyz[0];
  start[1] = centre_xyz[1];
  start[2] = centre_xyz[2] - radius;
  end[0]   = centre_xyz[0];
  end[1]   = centre_xyz[1];
  end[2]   = centre_xyz[2] + radius;
  add_gl_db_line( start, end, colour_rgba );
  // circles of 12 segments
  int segs = 12;
  // x,y around z loop
  for ( int i = 0; i < segs; i++ ) {
    start[0] = centre_xyz[0] + radius * cos( 2.0f * M_PI * (float)i / (float)segs );
    start[1] = centre_xyz[1] + radius * sin( 2.0f * M_PI * (float)i / (float)segs );
    start[2] = centre_xyz[2];
    end[0]   = centre_xyz[0] + radius * cos( 2.0f * M_PI * (float)( i + 1 ) / (float)segs );
    end[1]   = centre_xyz[1] + radius * sin( 2.0f * M_PI * (float)( i + 1 ) / (float)segs );
    end[2]   = centre_xyz[2];
    add_gl_db_line( start, end, colour_rgba );
  }
  // x,z around y loop
  for ( int i = 0; i < segs; i++ ) {
    start[0] = centre_xyz[0] + radius * cos( 2.0f * M_PI * (float)i / (float)segs );
    start[1] = centre_xyz[1];
    start[2] = centre_xyz[2] + radius * sin( 2.0f * M_PI * (float)i / (float)segs );
    end[0]   = centre_xyz[0] + radius * cos( 2.0f * M_PI * (float)( i + 1 ) / (float)segs );
    end[1]   = centre_xyz[1];
    end[2]   = centre_xyz[2] + radius * sin( 2.0f * M_PI * (float)( i + 1 ) / (float)segs );
    add_gl_db_line( start, end, colour_rgba );
  }
  // y,z around xloop
  for ( int i = 0; i < segs; i++ ) {
    start[0] = centre_xyz[0];
    start[1] = centre_xyz[1] + radius * cos( 2.0f * M_PI * (float)i / (float)segs );
    start[2] = centre_xyz[2] + radius * sin( 2.0f * M_PI * (float)i / (float)segs );
    end[0]   = centre_xyz[0];
    end[1]   = centre_xyz[1] + radius * cos( 2.0f * M_PI * (float)( i + 1 ) / (float)segs );
    end[2]   = centre_xyz[2] + radius * sin( 2.0f * M_PI * (float)( i + 1 ) / (float)segs );
    add_gl_db_line( start, end, colour_rgba );
  }

  return rid;
}

//
// takes 8 xyz corner points for given camera frustum and draws a box
// most camera code already extracts points from matrices so not repeated here
// returns first line's id
int add_gl_db_frustum( float* ftl, float* ftr, float* fbl, float* fbr, float* ntl, float* ntr, float* nbl, float* nbr, float* colour_rgba ) {
  int rid = -1;
  float start[3], end[3];
  start[0] = ftl[0];
  start[1] = ftl[1];
  start[2] = ftl[2];
  end[0]   = ftr[0];
  end[1]   = ftr[1];
  end[2]   = ftr[2];
  rid      = add_gl_db_line( start, end, colour_rgba );
  start[0] = ntl[0];
  start[1] = ntl[1];
  start[2] = ntl[2];
  end[0]   = ntr[0];
  end[1]   = ntr[1];
  end[2]   = ntr[2];
  add_gl_db_line( start, end, colour_rgba );
  start[0] = fbl[0];
  start[1] = fbl[1];
  start[2] = fbl[2];
  end[0]   = fbr[0];
  end[1]   = fbr[1];
  end[2]   = fbr[2];
  add_gl_db_line( start, end, colour_rgba );
  start[0] = nbl[0];
  start[1] = nbl[1];
  start[2] = nbl[2];
  end[0]   = nbr[0];
  end[1]   = nbr[1];
  end[2]   = nbr[2];
  add_gl_db_line( start, end, colour_rgba );
  // sides of top/bottom panels
  start[0] = ftl[0];
  start[1] = ftl[1];
  start[2] = ftl[2];
  end[0]   = ntl[0];
  end[1]   = ntl[1];
  end[2]   = ntl[2];
  add_gl_db_line( start, end, colour_rgba );
  start[0] = ftr[0];
  start[1] = ftr[1];
  start[2] = ftr[2];
  end[0]   = ntr[0];
  end[1]   = ntr[1];
  end[2]   = ntr[2];
  add_gl_db_line( start, end, colour_rgba );
  start[0] = fbl[0];
  start[1] = fbl[1];
  start[2] = fbl[2];
  end[0]   = nbl[0];
  end[1]   = nbl[1];
  end[2]   = nbl[2];
  add_gl_db_line( start, end, colour_rgba );
  start[0] = fbr[0];
  start[1] = fbr[1];
  start[2] = fbr[2];
  end[0]   = nbr[0];
  end[1]   = nbr[1];
  end[2]   = nbr[2];
  add_gl_db_line( start, end, colour_rgba );
  // edges for left/right panels
  start[0] = ftl[0];
  start[1] = ftl[1];
  start[2] = ftl[2];
  end[0]   = fbl[0];
  end[1]   = fbl[1];
  end[2]   = fbl[2];
  add_gl_db_line( start, end, colour_rgba );
  start[0] = ntl[0];
  start[1] = ntl[1];
  start[2] = ntl[2];
  end[0]   = nbl[0];
  end[1]   = nbl[1];
  end[2]   = nbl[2];
  add_gl_db_line( start, end, colour_rgba );
  start[0] = ftr[0];
  start[1] = ftr[1];
  start[2] = ftr[2];
  end[0]   = fbr[0];
  end[1]   = fbr[1];
  end[2]   = fbr[2];
  add_gl_db_line( start, end, colour_rgba );
  start[0] = ntr[0];
  start[1] = ntr[1];
  start[2] = ntr[2];
  end[0]   = nbr[0];
  end[1]   = nbr[1];
  end[2]   = nbr[2];
  add_gl_db_line( start, end, colour_rgba );
  return rid;
}

//
// modify or move a line previously added
// returns false if line_id wasn't right
bool mod_gl_db_line( unsigned int line_id, float* start_xyz, float* end_xyz, float* colour_rgba ) {
  if ( line_id >= g_count_gl_db_lines ) {
    fprintf( stderr, "ERROR: modifying gl db line - bad ID\n" );
    return false;
  }

  float sd[14];
  sd[0]  = start_xyz[0];
  sd[1]  = start_xyz[1];
  sd[2]  = start_xyz[2];
  sd[3]  = colour_rgba[0];
  sd[4]  = colour_rgba[1];
  sd[5]  = colour_rgba[2];
  sd[6]  = colour_rgba[3];
  sd[7]  = end_xyz[0];
  sd[8]  = end_xyz[1];
  sd[9]  = end_xyz[2];
  sd[10] = colour_rgba[0];
  sd[11] = colour_rgba[1];
  sd[12] = colour_rgba[2];
  sd[13] = colour_rgba[3];

  glBindBuffer( GL_ARRAY_BUFFER, g_gl_db_lines_vbo );
  GLintptr os = sizeof( sd ) * g_count_gl_db_lines;
  GLsizei sz  = sizeof( sd );
  glBufferSubData( GL_ARRAY_BUFFER, os, sz, sd );

  return true;
}

//
// update the camera matrix so that line points given are defined as being in
// world coord space
// matrix is a 16 float column-major matrix as 1d array in column order
void update_gl_db_cam_mat( float* PV_mat4 ) {
  glUseProgram( g_gl_db_lines_sp );
  glUniformMatrix4fv( g_gl_db_PV_loc, 1, GL_FALSE, PV_mat4 );
}

void draw_gl_db( bool x_ray ) {
  GLboolean dwe = false;
  glGetBooleanv( GL_DEPTH_TEST, &dwe );
  if ( dwe && x_ray ) {
    glDisable( GL_DEPTH_TEST );
  } else if ( !dwe && !x_ray ) {
    glEnable( GL_DEPTH_TEST );
  }

  glUseProgram( g_gl_db_lines_sp );
  glBindVertexArray( g_gl_db_lines_vao );
  glDrawArrays( GL_LINES, 0, 2 * g_count_gl_db_lines );

  if ( dwe && x_ray ) {
    glEnable( GL_DEPTH_TEST );
  } else if ( !dwe && !x_ray ) {
    glDisable( GL_DEPTH_TEST );
  }
}
