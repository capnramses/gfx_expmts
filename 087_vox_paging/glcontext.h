// DWARF 3-D Source Code
// Copyright Anton Gerdelan <antongdl@protonmail.com>. 2018
#pragma once

// this header is for internal sharing of glfw window and GL headers between gfx.c and input.c

#include <GL/glew.h>
#define GLFW_DLL
#include <GLFW/glfw3.h>

extern GLFWwindow* g_window;
