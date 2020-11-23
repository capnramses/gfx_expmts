gcc main.c gfx.c apg_maths.c apg_ply.c glad/src/glad.c -I ./glad/include/ -I ../common/include/ -L ../common/win64_gcc/ ../common/win64_gcc/libglfw3dll.a -lm -lOpenGL32
