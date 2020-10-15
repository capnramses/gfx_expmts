#!/bin/bash
gcc main.c apg_gfx.c glad/src/glad.c -I ./ -I ./glad/include/ -ldl -lglfw -lm

# ffmpeg stuff:
#mp4.c 
#apt install libavcodec-dev (also pulls in libavutils)
#-lavcodec -lavutil
