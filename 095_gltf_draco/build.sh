#!/bin/bash
gcc -g main.c apg_gfx.c apg_maths.c glad/src/glad.c -I ./ -I ./glad/include/ -I ./stb/  -ldl -lglfw -lGL -lm

# ffmpeg stuff:
#mp4.c 
#apt install libavcodec-dev (also pulls in libavutils)
#-lavcodec -lavutil
