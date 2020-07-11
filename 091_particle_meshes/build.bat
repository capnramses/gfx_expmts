
echo "Compiling glad"
REM Comple glad with less restrictive warnings
gcc -o glad/src/glad.o -c glad/src/glad.c -I glad/include/ 

echo "Compiling main program"
REM Compile main program with strict warnings
gcc -o a.exe ^
-Wall -Wextra -Wfatal-errors -pedantic -g -std=c99 ^
-D_CRT_SECURE_NO_WARNINGS ^
main.c apg/apg_gfx.c ^
-I apg/ -I glad/include/ -I stb/ -I . ^
../common/win64_gcc/libglfw3dll.a glad/src/glad.o ^
-L ../common/win64_gcc/ ^
-lOpenGL32 

copy ..\common\win64_gcc\glfw3.dll .\
