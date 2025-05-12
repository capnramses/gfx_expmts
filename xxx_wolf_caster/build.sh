#!/bin/bash
gcc -g main.c gfx.c maths.c -I ./ -I glad/include/ glad/src/gl.c -lglfw -lm