#!/bin/bash
gcc main.c gfx.c apg_maths.c ../common/glad/src/glad.c -I ../common/glad/include/ -I ../common/include/ -lglfw -lm
