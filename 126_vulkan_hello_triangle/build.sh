#!/bin/bash

glslc hello.vert -o vert.spv
glslc hello.frag -o frag.spv
gcc -g main.c -lglfw -lvulkan
