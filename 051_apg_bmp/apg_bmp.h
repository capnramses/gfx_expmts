// BMP File Reader interface
// Copyright Dr Anton Gerdelan <antongdl@protonmail.com> 2019
// C89 ( impl is C99 )

#ifndef _APG_BMP_H_
#define _APG_BMP_H_
#include <stddef.h>

/* TODO
- allow user to provide their own malloc / not do any allocation internally
*/

// reads a Microsoft BMP from filename, and allocates and returns an RGB-ordered buffer, or NULL on error.
unsigned char* apg_read_bmp( const char* filename, int* w, int* h, int* n );

// returns 0 on error
int apg_write_bmp( const char* filename, const unsigned char* pixel_data, int w, int h, int n_chans );

#endif
