//
// generic shader interface prototype
// C99
// Anton Gerdelan
// 20 Nov 2015
//

#pragma once

#include "maths_funcs.h"
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <stdbool.h>

#define MAX_SHADERS 128

struct Shader {
	GLuint sp, vs, fs;
	bool all_compiled;
	bool linked;
	int M_loc, V_loc, P_loc;
	int vp_loc, vt_loc, vn_loc;
};

struct Shaders {
	Shader shaders[128];
	int shader_count;
	int current_sp_i;
};

// returns false on error
bool init_shaders ();

// returns false on error
bool _init_fallback_shaders ();

// returns false on error
bool create_sp_from_files (const char* vs_fn, const char* fs_fn, int* sp_i);

// create shader with appropritate type first
// returns false on error and prints log
bool compile_s_from_file (GLuint s, const char* fn);

// does not do any malloc - fills existing buffer up to length max_len
// returns false on error
bool apg_file_to_str (const char* file_name, size_t max_len, char* str);

void _print_s_info_log (GLuint s);

// create program and attach shaders first
// returns false on error and prints log
bool link_sp (GLuint sp);

void _print_sp_info_log (GLuint sp);

// returns false on error
bool use_sp (int sp_i);

bool uni_M (int sp_i, mat4 M);

bool uni_V (int sp_i, mat4 M);

bool uni_P (int sp_i, mat4 M);

/*
bool uniform_M (int spi, mat4 M);
bool uniform_V (int spi, mat4 M);
bool uniform_P (int spi, mat4 M);
bool uniform_cam_pos (int spi, vec3 v);
*/