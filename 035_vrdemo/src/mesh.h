#pragma once

#include <stdbool.h>
#define MAX_MESH_COMPS 10000

typedef struct Mesh{
	float vps[MAX_MESH_COMPS];
	int vcount;
} Mesh;

Mesh load_ascii_stl(const char* fn);
