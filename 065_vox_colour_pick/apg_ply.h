#pragma once

/* Limitations
* Components are optional, but the order is fixed.
  The following is valid:
  x, y, s, t, red, green, blue.
  But the following is not valid:
  s, t, x, y, z
* Edges are ignored.
* Custom material sections are ignored.
* Comments are discarded.
* Only triangular and quad faces are read.
* Quad faces are always converted to triangles.
*/

typedef struct apg_ply_t {
  float* positions_ptr;
  float* normals_ptr;
  float* colours_ptr;
	float* texcoords_ptr;
  int n_vertices;
  int n_positions_comps;
  int n_normals_comps;
  int n_colours_comps;
	int n_texcoords_comps;
  int loaded; // 1 if there were no errors
} apg_ply_t;

unsigned int apg_ply_write( const char* filename, apg_ply_t ply );
