#!/bin/bash
gcc -Wall -Wextra \
main.c gfx.c apg_maths.c apg_ply.c apg_pixfont.c \
../common/glad/src/glad.c \
-I ../common/glad/include/ -I ../common/include/ \
-lm -ldl -lglfw -lGL
