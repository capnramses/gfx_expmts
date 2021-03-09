#include "platonic.h"

//
// The D6
//

float platonic_cube_vertices_xyz[8 * 3] = {
  1.0f, 1.0f, 1.0f,   // 0
  1.0f, 1.0f, -1.0f,  // 1
  1.0f, -1.0f, 1.0f,  // 2
  1.0f, -1.0f, -1.0f, // 3
  -1.0f, 1.0f, 1.0f,  // 4
  -1.0f, 1.0f, -1.0f, // 5
  -1.0f, -1.0f, 1.0f, // 6
  -1.0f, -1.0f, -1.0f // 7
};

// Facets are square but divided into 2 triangles each here.
uint32_t platonic_cube_indices_ccw_triangles[6 * 2 * 3] = {
  1, 0, 2, 2, 3, 1, //
  4, 5, 7, 7, 6, 4, //
  0, 1, 5, 5, 4, 0, //
  3, 2, 6, 6, 7, 3, //
  2, 0, 4, 4, 6, 2, //
  1, 3, 7, 7, 5, 1  //
};

//
// The D8
//

float platonic_octahedron_vertices_xyz[6 * 3] = {
  1.0f, 0.0f, 0.0f,  // 0
  -1.0f, 0.0f, 0.0f, // 1
  0.0f, 1.0f, 0.0f,  // 2
  0.0f, -1.0f, 0.0f, // 3
  0.0f, 0.0f, 1.0f,  // 4
  0.0f, 0.0f, -1.0f  // 5
};

// Facets are already triangle-shaped.
uint32_t platonic_octahedron_indices_ccw_triangles[8 * 3] = {
  0, 2, 4, //
  2, 0, 5, //
  3, 0, 4, //
  0, 3, 5, //
  2, 1, 4, //
  1, 2, 5, //
  1, 3, 4, //
  3, 1, 5  //
};

//
// D4 - Tetrahedron
//

//
// D12 - Dodecahedron
//

//
// D20 - Isocahedron
//

// D10 - not a platonic solid because the edges of each face are not the same size.
