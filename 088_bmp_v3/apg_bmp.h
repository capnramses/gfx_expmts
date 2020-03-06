/* apg_bmp.h V3
Anton Gerdelan
C
*/

#ifndef APG_BMP_H_
#define APG_BMP_H_

#include <stdio.h>

/*
Reads a bitmap from a file, allocates memory for the raw image data, and returns it.
PARAMS
w,h,     - Retrieves the width and height of the BMP in pixels.
n_chans  - Retrieves the number of channels in the BMP.
RETURNS
- Tightly-packed pixel memory containing in RGBA order. The caller must call free() on the memory.
- NULL on any error.
*/
unsigned char* apg_bmp_read( const char* filename, int* w, int* h, int* n_chans );

#endif
