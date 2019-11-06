gcc -g -o wadrend main.c wad.c gl_utils.c -std=c99 -I ../common/include/ \
-Wfatal-errors ../common/lin32/libGLEW.a ../common/lin32/libglfw3.a \
-lGL -lX11 -lXxf86vm -lXrandr -lpthread -lXi -ldl -lm -lXcursor -lXinerama
