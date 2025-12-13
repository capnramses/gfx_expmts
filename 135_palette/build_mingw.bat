gcc -g main.c gfx.c apg_bmp.c apg_maths.c glad/src/gl.c -I glad/include/ -I ../common/include/ ..\common\win64_gcc\libglfw3dll.a -lm
copy ..\common\win64_gcc\glfw3.dll .\
