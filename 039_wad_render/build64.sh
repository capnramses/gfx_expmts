gcc -g -o wadrend main.c wad.c gl_utils.c sky.c -std=c99 -I ../common/include/ \
-Wfatal-errors ../common/lin64/libGLEW.a ../common/lin64/libglfw3.a \
-lGL -lX11 -lXxf86vm -lXrandr -lpthread -lXi -ldl -lm -lXcursor -lXinerama
