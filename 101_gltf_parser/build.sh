#!/bin/bash
clang -fsanitize=address -Wall -Wextra -pedantic main.c gltf.c cJSON/cJSON.c -g
