#!/bin/bash

set -x # echo on

glslc hello.vert -o vert.spv
glslc hello.frag -o frag.spv

# allow skipping validation layers with e.g.
# NDEBUG=1 ./build.sh
if [[ -v NDEBUG ]]; then
  DEFS=-DNDEBUG
fi

gcc -O2 main.c apg_bmp.c apg_maths.c -lglfw -lvulkan $DEFS -g -lm