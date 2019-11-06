//
// Simple Wavefront .obj parser in C
// Anton Gerdelan 22 Dec 2014
// antongerdelan.net
//
#ifndef _OBJ_PARSER_H_
#define _OBJ_PARSER_H_

#include <stdbool.h>

bool load_obj_file (
	const char* file_name,
	float** points,
	float** tex_coords,
	float** normals,
	int* point_count
);

#endif