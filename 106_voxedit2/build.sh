 gcc -g\
 ./main.c camera.c gfx.c input.c ../common/glad/src/glad.c apg_ply.c apg_maths.c apg_pixfont.c \
 -I ../common/glad/include/ -I ../common/include/ \
 -L ./ \
 -lglfw -lGL -lm -ldl
