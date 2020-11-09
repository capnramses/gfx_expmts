gcc -g main.c apg_gfx.c apg_maths.c glad/src/glad.c -I . -I ./glad/include/ -I ./stb/ -I ../common/include/ -L ../common/win64_gcc/ ../common/win64_gcc/libglfw3dll.a -lglfw3 -lOpenGL32 -lm
