// WAD Rend - Copyright 2017 Anton Gerdelan <antonofnote@gmail.com>
// C99
#pragma once
#include <stdbool.h>
#include <stdio.h>

typedef unsigned char byte_t;

bool open_wad( const char *filename, const char *map_name );
// returns nbytes added
int fill_geom( float* geom_buff );
void fill_sectors();
void draw_sectors( int verts, int sectidx );
int sector_count();
bool load_picture_data( const char *lump_name, byte_t* pixel_buff, int width, int height );
bool get_wad_picture_dims( const char *name, int* _width, int* _height );