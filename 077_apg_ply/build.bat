gcc -g -Wfatal-errors -DGLEW_STATIC ^
main.c apg_ply.c apg_pixfont.c gl_utils.c input.c camera.c ^
-I ..\common\include\ -L ..\common\win64_gcc\ ^
..\common\src\GL\glew.c ..\common\win64_gcc\libglfw3dll.a ^
-lm -lOpenGL32
copy ..\common\win64_gcc\glfw3.dll .\