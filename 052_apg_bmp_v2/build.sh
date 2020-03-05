#!/bin/bash
clang -fsanitize=address -fsanitize=undefined -ggdb -Wall -pedantic -Werror -g -o demo main_opengl.c apg_bmp.c ../common/src/GL/glew.c -I ../common/include/ -lglfw -lpthread -lGL
clang -fsanitize=address -fsanitize=undefined -ggdb -Wall -pedantic -Werror -g -o readwritetest main_test.c apg_bmp.c
