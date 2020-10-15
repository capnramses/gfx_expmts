#!/bin/bash
gcc main.c apg_gfx.c glad/src/glad.c -I ./ -I ./glad/include/ -ldl -lglfw
