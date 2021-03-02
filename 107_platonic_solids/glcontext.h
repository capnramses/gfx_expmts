/*
"New Voxel Project"
This header is for internal sharing of glfw window and GL headers between gfx.c and input.c.
Copyright: Anton Gerdelan <antonofnote@gmail.com>
C99
*/

#pragma once

#include <glad/glad.h>
//#include <GL/glew.h> // use GLEW or glad, not both
#define GLFW_DLL
#include <GLFW/glfw3.h>

extern GLFWwindow* gfx_window_ptr; /** Pointer to the application's GLFW window. Defined in apg_gfx.c. */
