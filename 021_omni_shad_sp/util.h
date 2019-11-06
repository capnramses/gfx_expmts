#pragma once

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>

#define VP_WIDTH 1024
#define VP_HEIGHT 768
#define MAX_SHDR_LEN 4096

typedef unsigned int uint;

GLFWwindow* start_gl ();

uint init_db_panel ();

bool parse_file_into_str (const char* fn, char* str, int max_len);

uint create_sp_from_files (const char* vs_fn, const char* fs_fn);

void create_vao_from_obj (const char* fn, uint* vao, int* pc);