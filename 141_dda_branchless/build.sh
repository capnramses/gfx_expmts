#!/bin/bash
gcc -g main.c apg_bmp.c gfx.c apg_maths.c ray.c vox_fmt.c glad/src/gl.c -I glad/include/ -lglfw -lm