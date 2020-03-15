REM Compile GLEW with fewer warnings
gcc -Wno-attributes -g -o glew.o -c ..\common\src\GL\glew.c -I ..\common\include\ -DGLEW_STATIC

REM Compile main program with strict warnings
gcc -g -Wfatal-errors -Wall -Wextra -pedantic -DGLEW_STATIC ^
main.c voxels.c apg_ply.c apg_pixfont.c gl_utils.c input.c camera.c diamond_square.c ^
-I ..\common\include\ -I ..\common\include\stb\ -L ..\common\win64_gcc\ ^
glew.o ..\common\win64_gcc\libglfw3dll.a ^
-lm -lOpenGL32
copy ..\common\win64_gcc\glfw3.dll .\