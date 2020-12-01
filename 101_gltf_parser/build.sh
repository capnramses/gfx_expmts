#!/bin/bash
clang -fsanitize=address -Wall -Wextra -pedantic main.c gfx_gltf.c gltf.c cJSON/cJSON.c -g
