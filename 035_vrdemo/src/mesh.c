#include "mesh.h"
#include <stdio.h>
#include <assert.h>
#include <string.h>

Mesh load_ascii_stl(const char* fn){
	assert(fn);
	Mesh mesh;
	printf("loading STL mesh %s\n", fn);
	FILE* fp = fopen(fn, "r");
	assert(fp);
	char line[1024];
	while(fgets(line, 1024, fp)){
		if (strncmp(line, "vertex", 6) == 0){
			assert ((mesh.vcount * 3 + 3) < MAX_MESH_COMPS);
			sscanf(line, "vertex %f %f %f",
				&mesh.vps[mesh.vcount * 3],
				&mesh.vps[mesh.vcount * 3 + 1],
				&mesh.vps[mesh.vcount * 3 + 2]);
			mesh.vcount ++;
		}
	}
	fclose(fp);
	return mesh;
}
