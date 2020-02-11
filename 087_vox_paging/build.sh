#!/bin/bash
clang -fsanitize=address -Wall -Wextra -Wfatal-errors -pedantic -g \
main.c voxels.c apg_ply.c apg_pixfont.c camera.c input.c gl_utils.c diamond_square.c \
../common/src/GL/glew.c -I../common/include/ -I ../common/include/stb/ -lm -lglfw -lGL
