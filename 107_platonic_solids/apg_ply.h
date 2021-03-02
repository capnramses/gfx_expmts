#pragma once

/*

History
 3 Jun 2020 - Support 8-component Blender export default.
13 Apr 2020 - Restored apg_ply_read()
12 Apr 2020 - Reduced float precision to 2dp.
xx Apr 2020 - Added edges to output.

Limitations
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
  unsigned char* colours_ptr;
  float* texcoords_ptr;
  float* edges_ptr; // custom attrib for Anton's voxel outlines
  unsigned int n_vertices;
  unsigned int n_positions_comps;
  unsigned int n_normals_comps;
  unsigned int n_colours_comps;
  unsigned int n_texcoords_comps;
  unsigned int n_edges_comps; // custom attrib for Anton's voxel outlines
} apg_ply_t;

/*
RETURNS - 0 on failure, 1 on success */
unsigned int apg_ply_write( const char* filename, apg_ply_t ply );

/* not a generic reader. only handles some vertex properties. assumes mesh has only 2 body sections - vertex list, followed by face list.
RETURNS - 0 on failure, 1 on success */
unsigned int apg_ply_read( const char* filename, apg_ply_t* ply );

void apg_ply_free( apg_ply_t* ply );
