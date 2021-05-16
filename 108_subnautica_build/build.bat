gcc -Wall -Wextra -g ^
main.c gfx.c apg_maths.c apg_ply.c apg_pixfont.c ^
../common/glad/src/glad.c ^
../common/win64_gcc/libglfw3dll.a ^
-I ../common/glad/include/ -I ../common/include/ ^
-L ../common/win64_gcc/ ^
-lm -lOpenGL32

copy ..\common\win64_gcc\glfw3.dll .\
