#ifndef _OBJ_PARSER_H_
#define _OBJ_PARSER_H_

/*************************************\
* Anton's lazy OBJ parser             *
* Anton Gerdelan 7 Nov 2013           *
*-------------------------------------*
* Notes:                              *
* I ignore MTL files                  *
* Mesh MUST be triangulated - quads   *
* not accepted                        *
* Mesh MUST contain vertex points,    *
* normals, and texture coordinates    *
* faces MUST come after all other     *
* data in the .obj file               *
\*************************************/

bool load_obj_file (
	const char* file_name,
	float*& points,
	float*& tex_coords,
	float*& normals,
	int& point_count
);

#endif
