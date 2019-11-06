#!/bin/bash
clang -fsanitize=address -Wall -Wextra -Wfatal-errors -Werror -pedantic -g \
main.c apg_ply.c apg_pixfont.c camera.c input.c gl_utils.c \
../common/src/GL/glew.c -I../common/include/ -lm -lglfw -lGL
