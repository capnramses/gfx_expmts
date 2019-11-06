#!/bin/bash
gcc -Wfatal-errors -g main.c voxels.c apg_ply.c apg_pixfont.c gl_utils.c ../common/src/GL/glew.c -I../common/include/ -lm -lglfw -lGL
