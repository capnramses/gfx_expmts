/**
 *
 * Based on "A Trip Down the Graphics Pipeline", Ch. 4 "Platonic Solids", Jim Blinn.
 *
 * TODO (maybe)
 * function to generate a list of face normals and face colours for each solid
 * and unroll the indexed into unique triangles
 * and UVs maybe
 */

#include "platonic.h"

//
// D4 - Tetrahedron
//
float platonic_tetrahedron_vertices_xyz[4 * 3] = {
  1.0f, 1.0f, 1.0f,   // 0
  1.0f, -1.0f, -1.0f, // 1
  -1.0f, 1.0f, -1.0f, // 2
  -1.0f, -1.0f, 1.0f  // 3
};

uint32_t platonic_tetrahedron_indices_ccw_triangles[4 * 3] = {
  3, 2, 1, //
  2, 3, 0, //
  1, 0, 3, //
  0, 1, 2  //
};

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
// NB the octohedron is the 'dual' shape of the cube -> each vertex lies on a cube face.
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
// D12 - Dodecahedron
// Note: the dodecahedron has the property of having a cube embedded within it.

#define PLA_PHI 1.618034f
#define PLA_INV_PHI 0.618034f
float platonic_dodecahedron_vertices_xyz[20 * 3] = {
  1, 1, 1,                      // 0 "1"
  1, 1, -1,                     // 1 "2"
  1, -1, 1,                     // 2 "3"
  1, -1, -1,                    // 3 "4"
  -1, 1, 1,                     // 4 "5"
  -1, 1, -1,                    // 5 "6"
  -1, -1, 1,                    // 6 "7"
  -1, -1, -1,                   // 7 "8"
  PLA_INV_PHI, PLA_PHI, 0.0f,   // 8 "11"
  -PLA_INV_PHI, PLA_PHI, 0.0f,  // 9 "12"
  PLA_INV_PHI, -PLA_PHI, 0.0f,  // 10 "13"
  -PLA_INV_PHI, -PLA_PHI, 0.0f, // 11 "14"
  PLA_PHI, 0.0f, PLA_INV_PHI,   // 12 "21"
  PLA_PHI, 0.0f, -PLA_INV_PHI,  // 13 "22"
  -PLA_PHI, 0.0f, PLA_INV_PHI,  // 14 "23"
  -PLA_PHI, 0.0f, -PLA_INV_PHI, // 15 "24"
  0.0f, PLA_INV_PHI, PLA_PHI,   // 16 "31"
  0.0f, -PLA_INV_PHI, PLA_PHI,  // 17 "32"
  0.0f, PLA_INV_PHI, -PLA_PHI,  // 18 "33"
  0.0f, -PLA_INV_PHI, -PLA_PHI, // 19 "34"
};

uint32_t platonic_dodecahedron_indices_ccw_triangles[36 * 3] = {
  1, 8, 0, 0, 12, 13, 13, 1, 0,  // "11"
  4, 9, 5, 5, 15, 14, 14, 4, 5,  // "12"
  2, 10, 3, 3, 13, 12, 12, 2, 3, // "13"
  7, 11, 6, 6, 14, 15, 15, 7, 6, // "14"
  2, 12, 0, 0, 16, 17, 17, 2, 0, // "21"
  1, 13, 3, 3, 19, 18, 18, 1, 3, // "22"
  4, 14, 6, 6, 17, 16, 16, 4, 6, // "23"
  7, 15, 5, 5, 18, 19, 19, 7, 5, // "24"
  4, 16, 0, 0, 8, 9, 9, 4, 0,    // "31"
  2, 17, 6, 6, 11, 10, 10, 2, 6, // "32"
  1, 18, 5, 5, 9, 8, 8, 1, 5,    // "33"
  7, 19, 3, 3, 10, 11, 11, 7, 3  // "34"
};

//
// D20 - Isocahedron
//
float platonic_isocahedron_vertices_xyz[12 * 3] = {
  // first 'golden rectangle'
  PLA_PHI, 1.0f, 0.0f,   // 0 "11"
  -PLA_PHI, 1.0f, 0.0f,  // 1 "12"
  PLA_PHI, -1.0f, 0.0f,  // 2 "13"
  -PLA_PHI, -1.0f, 0.0f, // 3 "14"
  // second
  1.0f, 0.0f, PLA_PHI,   // 4 "21"
  1.0f, 0.0f, -PLA_PHI,  // 5 "22"
  -1.0f, 0.0f, PLA_PHI,  // 6 "23"
  -1.0f, 0.0f, -PLA_PHI, // 7 "24"
  // third
  0.0f, PLA_PHI, 1.0f,  // 8 "31"
  0.0f, -PLA_PHI, 1.0f, // 9 "32"
  0.0f, PLA_PHI, -1.0f, // 10 "33"
  0.0f, -PLA_PHI, -1.0f // 11 "34"
};

uint32_t platonic_isocahedron_indices_ccw_triangles[20 * 3] = {
  0, 8, 4,  // 0
  0, 5, 10, // 1
  2, 4, 9,  // 2
  2, 11, 5, // 3
  1, 6, 8,  // 4
  1, 10, 7, // 5
  3, 9, 6,  // 6
  3, 7, 11, // 7
  //
  0, 10, 8, // 8
  1, 8, 10, // 9
  2, 9, 11, // 10
  3, 11, 9, // 11
            //
  4, 2, 0,  // 12
  5, 0, 2,  // 13
  6, 1, 3,  // 14
  7, 3, 1,  // 15
  //
  8, 6, 4,  // 16
  9, 4, 6,  // 17
  10, 5, 7, // 18
  11, 7, 5, // 19
};

// D10 - not a platonic solid because the edges of each face are not the same size.
