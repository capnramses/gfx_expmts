#!/bin/bash
gcc -g main.c apg_pixfont.c fps_view.c gfx.c maths.c minimap.c -I ./ -I glad/include/ glad/src/gl.c -lglfw -lm
