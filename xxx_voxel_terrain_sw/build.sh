#!/bin/bash
gcc main.c -g -fsanitize=address -lm -I glad/include/ glad/src/gl.c -lGL -lglfw
