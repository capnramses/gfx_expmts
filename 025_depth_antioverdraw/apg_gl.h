//
// opengl custom header, C99
// anton gerdelan <gerdel@scss.tcd.ie>
// trinity college dublin
// 20 jan 2016
//

#pragma once
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <stdbool.h>

struct APG_GL_Context {
	int win_width, win_height, fb_width, fb_height;
	GLFWwindow* win;
};
typedef struct APG_GL_Context APG_GL_Context;

struct APG_Mesh {
	GLuint vao, vbo_vps, vbo_vts, vbo_vns, pc;
};
typedef struct APG_Mesh APG_Mesh;

bool start_gl ();
void stop_gl ();

extern APG_GL_Context g_gl;
