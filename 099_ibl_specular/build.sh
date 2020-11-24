#!/bin/bash
gcc -Wall -g \
main.c \
gfx.c \
apg_maths.c \
apg_ply.c \
input.c \
glad/src/glad.c \
-I ./glad/include/ -I ../common/include/ \
-lm -lGL -lglfw -ldl
