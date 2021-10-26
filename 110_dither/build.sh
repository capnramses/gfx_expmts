#!/bin/bash
# set -v is verbose mode (equiv to echo on)
set -v
gcc main.c gfx.c ../common/glad/src/glad.c -I ../common/glad/include/ -I ../common/include/ -lm -ldl -lglfw
