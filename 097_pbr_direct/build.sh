#!/bin/bash
gcc -g main.c gfx.c apg_maths.c glad/src/glad.c -I ./ -I ./glad/include/ -I ./stb/ -I ../common/include/ -lglfw -lGL -lm -ldl
