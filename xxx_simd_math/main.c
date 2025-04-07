/*
SIMD for x64
Note: There is also Neon for ARM devices).

Example - maybe setting r,g,b,a in an image.
Colour-fill a rectangle.
Alignment of memory may overlap edges.
Tiling may be a good idea.
e.g. instead of
ABCD
do
AB
CD (less likely to go off the edges).
for AVX512 you'd rather have a 4x4 block than a 1x16 sliver.

SoA vs AoS
ideally we want RRRR GGGG BBBB AAAA so we can do R'=R*G on 4 Rs at once.

Qs
1. What to #include on Linux for GCC and Clang.
2. Which avx/sse to use. sse older.
  mmx integer-only 64-bit register (from Pentium MMX).
  sse/neon 4-wide 128-bit register.
  avx      8-wide (from Sandy Bridge and Haswell (avx2)
  avx512  16-wide (from Skylake-X)
AVX: introduces three-operand instructions (c = a + b vs. a = a + b)
AVX2: Introduces fused multiply-add (FMA: c = c + a*b
Sandy Bridge and later has (at least) AVX
https://acl.inf.ethz.ch/teaching/fastcode/2021/slides/07-simd-avx.pdf

3. Can I do some AVX512?
4. How to test device caps - at build or run time?
  cat /proc/cpuinfo
5. How much SIMD is compiler already doing with my maths code for me?
   (I think I can check with asm inspection as in Compiler Explorer)
6. How much faster does it go in a test app?
*/

#ifdef __AVX__
#include <immintrin.h> // GCC AVX header. Pass the `-mavx` flag.
#endif
#include <stdio.h>

int main() {
#ifdef __AVX__
  printf( "Hello, AVX!\n" );
#else
  print( "No AVX support!\n" );
#endif

  __m256 f; // 256-bit avx register with 8 floats in it.
//  __m256d d; // 256-bit avx register with 4 doubles in it.  
//  __m256i i; // 256-bit "  with either 32x 8-bits, 16x 16-bits, 8x 32-bits, or 4x64-bit integers.

  float a[8] ={1,2,3,4,5,6,7,8};
  f = _mm256_loadu_ps(a); // u is unaligned memory in
  __m256 ff = _mm256_setzero_ps();
  ff = _mm256_add_ps( f, f );

  float b[8] = { 0 };
  _mm256_storeu_ps( b, ff );
  printf("b={%f,%f,%f,%f,%f,%f,%f,%f}\n",b[0],b[1],b[2],b[3],b[4],b[5],b[6],b[7]);

  // pick a for-loop that goes i++
  // and change it to x+=4
  // and do groups of 4 items (xyzw or rgba) at a time.
  // most simd instructions are e.g. add (and also a datatype)
  // and then they e.g. throw 4 floats into a 128bit register.

  // for packed RGBA RGBA RGBA RGBA
  // SSE doesn't support a stride so you cant really do 'all the Rs'
  // .: SoAs is important on x64/intel intrinsics
  // Neon can do a stride.
  
  return 0;
}
