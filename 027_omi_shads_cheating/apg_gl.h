//
// opengl custom header, C99
// anton gerdelan <gerdel@scss.tcd.ie>
// trinity college dublin
// 20 jan 2016
//

//
// TODO:
// * should I just make some VAO/VBOs for platonic solids on init?
//

#pragma once
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <stdbool.h>

//===============================data structures===============================
struct APG_GL_Context {
	int win_width, win_height, fb_width, fb_height;
	GLFWwindow* win;
};
typedef struct APG_GL_Context APG_GL_Context;

struct APG_Mesh {
	GLuint vao, vbo_vps, vbo_vts, vbo_vns, vbo_indexed, pc;
};
typedef struct APG_Mesh APG_Mesh;

struct APG_Shader {
	GLuint sp, vs, fs;
	GLint tex_loc, sca_loc, pos_loc;
};
typedef struct APG_Shader APG_Shader;

//==================================interface==================================
bool start_gl ();
void stop_gl ();

extern APG_GL_Context g_gl;

//===============================platonic solids===============================
extern float apg_cube_points[24];
extern unsigned int apg_cube_ccw_tri_indices[36];
#define APG_CUBE_POINT_COUNT 36
#define APG_CUBE_POINTS_SZ 8 * 3 * sizeof(float)
#define APG_CUBE_INDICES_SZ 12 * 3 * sizeof(unsigned int)
extern GLuint ss_quad_tristrip_vao;
extern APG_Shader g_ss_quad_shader;
