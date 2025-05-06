#!/bin/bash

glslc hello.vert -o vert.spv
glslc hello.frag -o frag.spv
gcc -O2 main.c -lglfw -lvulkan
