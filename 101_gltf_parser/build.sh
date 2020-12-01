#!/bin/bash
clang \
-fsanitize=address \
-Wall -Wextra -pedantic \
-g \
main.c apg_maths.c apg_ply.c gfx.c gfx_gltf.c gltf.c input.c cJSON/cJSON.c glad/src/glad.c \
-I glad/include/ \
-lglfw -lGL -lm
