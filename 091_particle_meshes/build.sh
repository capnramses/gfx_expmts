
echo "Compiling glad"
# Comple glad with less restrictive warnings
gcc -o glad/src/glad.o -c glad/src/glad.c -I glad/include/ 

echo "Compiling main program"
# Compile main program with strict warnings
clang \
-Wall -Wextra -Wfatal-errors -pedantic -g -std=c99 \
-fsanitize=address -fsanitize=undefined \
main.c apg/apg_gfx.c apg/apg_ply.c apg/apg_rand.c \
-I apg/ -I glad/include/ -I stb/ -I . \
glad/src/glad.o \
-lglfw -lGL -ldl -lm
