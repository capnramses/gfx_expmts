//
// OpenGL Debug Draw
// First v. Anton Gerdelan 11 June 2015
// feature complete v. original spec: 16 June 2015
// https://github.com/capnramses/opengl_debug_draw
//

#pragma once
#include <stdbool.h>

//
// memory is reserved to support this many lines at one time
#define MAX_APG_GL_DB_LINES 8192

// reserve memory for drawing stuff
bool init_gl_db();

//
// free memory
void free_gl_db();

//
// add a new debug line to world space coords
// params are arrays of floats e.g. start[3] end[3] colour[4]
// return index corresponding to internal memory offset
int add_gl_db_line( float* start_xyz, float* end_xyz, float* colour_rgba );

//
// a line with a colour that goes from black to coloured in direction of norm
int add_gl_db_normal( float* n_xyz, float* pos_xyz, float scale, float* colour_rgba );

//
// 3 lines in a cross with a colour to show a position
// returns the first of 3 contiguous line ids
int add_gl_db_pos( float* pos_xyz, float scale, float* colour_rgba );

//
// draw box for this aabb
// returns the first of 12 contiguous line ids
int add_gl_db_aabb( float* min_xyz, float* max_xyz, float* colour_rgba );

//
// draw a circle+radius to represent a sphere
// returns first of 39 line ids
int add_gl_db_rad_circle( float* centre_xyz, float radius, float* colour_rgba );

//
// takes 8 xyz corner points for given camera frustum and draws a box
// most camera code already extracts points from matrices so not repeated here
// returns first line's id
int add_gl_db_frustum( float* ftl, float* ftr, float* fbl, float* fbr, float* ntl, float* ntr, float* nbl, float* nbr, float* colour_rgba );

//
// modify or move a line previously added
// returns false if line_id wasn't right
bool mod_gl_db_line( unsigned int line_id, float* start_xyz, float* end_xyz, float* colour_rgba );

//
// update the camera matrix so that line points given are defined as being in
// world coord space
// matrix is a 16 float column-major matrix as 1d array in column order
void update_gl_db_cam_mat( float* PV_mat4 );

//
// draw the lines!
// if x_ray is true then depth testing is disabled
void draw_gl_db( bool x_ray );
