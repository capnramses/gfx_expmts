#!/bin/bash

# I compile all my C code first so C++ compiler can cope without complaint.
gcc -DAPG_NO_BACKTRACES -o apg.o -c apg.c -I ./
gcc -o glad.o -c ../common/glad/src/glad.c -I ../common/glad/include/
gcc -o gfx.o -c gfx.c -I ./ -I ../common/glad/include/ -I ../common/include/
gcc -o apg_maths.o -c apg_maths.c -I ./

# Compile Basis Transcoder, excluding its dependencies.
# Note: BASISD_SUPPORT_KTX2=0 removes zstd and ktx2 dependencies.
#       BASISD_SUPPORT_KTX2_ZSTD=0 can be use to disable zstd but support most ktx2 files (excluding UASTC super-compressed KTX2 files).
g++ -fno-strict-aliasing -DBASISD_SUPPORT_KTX2=0 \
-o basisu_transcoder.o -c third_party/basis_universal/transcoder/basisu_transcoder.cpp -I third_party/basis_universal/transcoder/

# Compile program.
g++ -g \
-I ../common/include/ \
-I ./third_party/ \
-I ./third_party/basis_universal/transcoder/ \
main.c \
gfx.o \
apg.o \
apg_maths.o \
glad.o \
basisu_transcoder.o \
-lm -lglfw
