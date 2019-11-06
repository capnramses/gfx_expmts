#pragma once
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <stdbool.h>

bool start_gl ();
bool parse_file_into_str (const char* file_name, char* shader_str, int max_len);
GLuint create_programme_from_files (const char* vert_file_name,
	const char* frag_file_name);

extern GLuint g_gl_width;
extern GLuint g_gl_height;
extern GLFWwindow* g_window;
