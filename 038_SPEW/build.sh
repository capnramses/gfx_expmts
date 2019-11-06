clang -std=c99 -g -o demo main.c spew.c lib/linux64/libglfw3.a \
-lGL -lX11 -lXxf86vm -lXrandr -lpthread -lXi -lXinerama -lXcursor -ldl -lrt -lm
