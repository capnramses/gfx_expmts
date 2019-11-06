// Font atlas and string geometry generator
// Based on Sean Barret's stb_truetype.h library
// Anton Gerdelan - 3 May 2018 <antonofnote@gmail.com>
// C99
// Licence: GNU GPL v3
//
// TODO
// * draw_text() function, load_font_shaders() ?
// * position setter as relative to window size/space or absolute
// * remove debugging printfs and image writes
// * allow caller to provide malloc function
// * a texts thing to manage and draw a list of texts - has model matrices etc

#pragma once
#include "stb_truetype.h"
#include <stdbool.h>

// baked (atlas) font loaded from one or more font filess
typedef struct font_t {
	float scale_px;
	// computed from scale_px - for use by stb functions
	float scale_stb;

	// has to stay loaded until done with font
	unsigned char *file_buffer;
	// uses file_buffer, does not alloc mem
	stbtt_fontinfo font_info_stb;

	// 1-channel image
	unsigned char *atlas_image;
	int atlas_h, atlas_w;

	stbtt_pack_context pack_context;

	// optional CJK font data
	unsigned char *file_buffer_cjk;
	stbtt_fontinfo font_info_stb_cjk;
	float scale_stb_cjk;

	// optional Thai font data
	unsigned char *file_buffer_thai;
	stbtt_fontinfo font_info_stb_thai;
	float scale_stb_thai;

	// layout in atlas of each code point (index by codepoint)
	stbtt_packedchar *char_data;

	// OpenGL texture handle
	unsigned int atlas_texture;

} font_t;

// loads default font, starts packing
bool load_font( const char *file_name, float scale_px, int atlas_w, int atlas_h, int oversampling, font_t *font );

// optionally add in a font supporting thai glyphs
bool pack_thai( const char *file_name, font_t *font );

// continues packaing a loaded font by adding CJK glyphs from another file
bool pack_cjk( const char *file_name, font_t *font );

// ends font packing
void finalise_font( font_t *font );

// copy internal image memory into an OpenGL texture. also frees that buffer
void create_opengl_font_atlas( font_t *font );

// frees a loaded font
void free_font( font_t *font );

///////////////////////////

// utility to pull the next unicode codepoint from a string, and indicate how many bytes long it is
int utf8_decode( const char *buf, int *nbytes );

// update an opengl vertex buffer object with quads rendering a string of unicode text
int update_string_geom( const font_t *font, float x, float y, const char *text, unsigned int vbo, float scale );

// TODO -- API for text strings - create, move, draw list of them
typedef struct text_t {
	float r, g, b, a;
} text_t;