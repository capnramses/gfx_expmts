copy ..\common\win64_gcc\glfw3.dll .\

gcc main.c gfx.c apg_maths.c apg_unicode.c ^
../common/glad/src/glad.c -I ../common/glad/include/ ^
../common/win64_gcc/libglfw3dll.a ^
-I ../common/include/ -lglfw3 -lm
