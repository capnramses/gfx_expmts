#pragma once

#include <stdint.h>

extern float platonic_cube_vertices_xyz[24];
extern uint32_t platonic_cube_indices_ccw_triangles[36];

extern float platonic_octahedron_vertices_xyz[6 * 3];
extern uint32_t platonic_octahedron_indices_ccw_triangles[8 * 3];

extern float platonic_tetrahedron_vertices_xyz[4 * 3];
extern uint32_t platonic_tetrahedron_indices_ccw_triangles[4 * 3];

extern float platonic_isocahedron_vertices_xyz[12 * 3];
extern uint32_t platonic_isocahedron_indices_ccw_triangles[20 * 3];
