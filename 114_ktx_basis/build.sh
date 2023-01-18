#!/bin/bash

gcc -g -I ../common/include/stb/ -I ../common/include/ -I ../common/glad/include/ main.c gfx.c apg_maths.c ../common/glad/src/glad.c -lm -lglfw
