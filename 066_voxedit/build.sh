#!/bin/bash
gcc -Wall -Wextra -Wfatal-errors -pedantic -g \
main.c voxels.c apg_ply.c apg_pixfont.c camera.c input.c gl_utils.c \
../common/src/GL/glew.c -I../common/include/ -lm -lglfw -lGL
