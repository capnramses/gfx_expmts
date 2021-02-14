#pragma once

#include <glad/glad.h>
//#include <GL/glew.h> // use GLEW or glad, not both
#define GLFW_DLL
#include <GLFW/glfw3.h>

extern GLFWwindow* gfx_window_ptr; /** Pointer to the application's GLFW window. Defined in gfx.c. */
