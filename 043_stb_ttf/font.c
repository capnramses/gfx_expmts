// Font atlas and string geometry generator
// Based on Sean Barret's stb_truetype.h library
// Anton Gerdelan - 3 May 2018 <antonofnote@gmail.com>
// C99
// Licence: GNU GPL v3

#include "font.h"
#define STB_RECT_PACK_IMPLEMENTATION
#include "stb_rect_pack.h"
#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_CODEPOINT 65519 + 1

// set to false to exclude from atlas
bool ascii = true;
bool latin1 = true;
bool latinextab = true;
bool greekcoptic = true;
bool cyrillic = true;
bool thai = true;
bool cjk_punct = true;
bool hirigana = true;
bool katakana = true;
bool bopomofo = true;
bool hangul = true;
bool romaji = true;
bool custom_cjk = true;

// a list of Chinese-Japanese-Korean set glyphs actually used in strings in the app. there are otherwise too many glyphs to fit
// in a baked font image this array can be built by eg. parsing a utf-8 "strings.txt" file and pulling out the codepoint values
int cjk_codepoints[] = { 31616, 20307, 27721, 35821, 32321, 39636, 28450, 35486, 26085, 26412, 12398, 26032, 28216, 25103,
												 36938, 25138, 12375, 12356, 12466, 12540, 12512, 20572, 27490, 32080, 26463, 32066, 20102, 36873,
												 39033, 36984, 38917, 12458, 12503, 12471, 12519, 12531, 20027, 26426, 27231, 12467, 12477, 12523,
												 21152, 20837, 21443, 21462, 28040, 12461, 12515, 12475, 24320, 22987, 38283, 12417, 12414, 12377,
												 22320, 22336, 12450, 12489, 12524, 12473, 36830, 25509, 20013, 37832, 32154 };

bool load_font( const char *file_name, float scale_px, int atlas_w, int atlas_h, int oversampling, font_t *font ) {
	printf( "loading font `%s`:\n", file_name );
	assert( file_name );
	assert( scale_px > 0.0f );
	assert( font );

	memset( font, 0, sizeof( font_t ) );

	FILE *fptr = fopen( file_name, "rb" );
	if ( !fptr ) {
		fprintf( stderr, "ERROR: could not open file `%s`\n", file_name );
		return false;
	}
	{ // read entire file into buffer
		fseek( fptr, 0, SEEK_END );
		size_t sz = ftell( fptr );
		rewind( fptr );
		font->file_buffer = (unsigned char *)malloc( sz );
		assert( font->file_buffer );
		printf( " * allocated %i\n", (int)sz );
		fread( font->file_buffer, sz, 1, fptr );
	}
	fclose( fptr );

	font->char_data = (stbtt_packedchar *)calloc( sizeof( stbtt_packedchar ), MAX_CODEPOINT );

	int nfonts = stbtt_GetNumberOfFonts( font->file_buffer );
	printf( " * %i font(s) in file\n", nfonts );
	int font_offset = stbtt_GetFontOffsetForIndex( font->file_buffer, 0 );
	printf( " * offset for font 0 is %i \n", font_offset );
	int ret = stbtt_InitFont( &font->font_info_stb, font->file_buffer, font_offset );
	if ( !ret ) {
		fprintf( stderr, "ERROR: failed to init font\n" );
		return false;
	}
	printf( " * %i glyphs\n", font->font_info_stb.numGlyphs );
	font->scale_px = scale_px;
	font->scale_stb = stbtt_ScaleForPixelHeight( &font->font_info_stb, font->scale_px );
	printf( " * px scale=%.2f, stb scale=%.2f\n", font->scale_px, font->scale_stb );

	{
		size_t sz = atlas_w * atlas_h;
		font->atlas_image = (unsigned char *)calloc( sz, 1 );
		assert( font->atlas_image );
		font->atlas_w = atlas_w;
		font->atlas_h = atlas_h;
		printf( " * allocated %i for %ix%i atlas\n", (int)sz, atlas_w, atlas_h );
		int stride_in_bytes = atlas_w; // 0 means tightly packed
		int padding = 1;							 // 1 recommended for bilinear filtering
		// NOTE(Anton) no idea what last param does. NULL in an stb test example
		int ret = stbtt_PackBegin( &font->pack_context, font->atlas_image, atlas_w, atlas_h, stride_in_bytes, padding, NULL );
		if ( !ret ) {
			fprintf( stderr, "ERROR: failed to begin packing font\n" );
			return false;
		}
		stbtt_PackSetOversampling( &font->pack_context, oversampling, oversampling );
		{
			stbtt_pack_range ranges[6];
			int nranges = 0;
			if ( ascii ) {
				ranges[nranges].first_unicode_codepoint_in_range = 0x0020;
				ranges[nranges].num_chars = 0x007F - 0x0020;
				nranges++;
			}
			if ( latin1 ) {
				ranges[nranges].first_unicode_codepoint_in_range = 0x00A0;
				ranges[nranges].num_chars = 0x00FF - 0x00A0;
				nranges++;
			}
			if ( latinextab ) {
				ranges[nranges].first_unicode_codepoint_in_range = 0x0100;
				ranges[nranges].num_chars = 256;
				nranges++;
			}
			if ( greekcoptic ) {
				ranges[nranges].first_unicode_codepoint_in_range = 0x0370;
				ranges[nranges].num_chars = 0x03FF - 0x0370;
				nranges++;
			}
			if ( cyrillic ) {
				ranges[nranges].first_unicode_codepoint_in_range = 0x0400;
				ranges[nranges].num_chars = 0x04FF - 0x0400;
				nranges++;
			}
			for ( int i = 0; i < nranges; i++ ) {
				ranges[i].font_size = font->scale_px;
				ranges[i].array_of_unicode_codepoints = NULL;
				ranges[i].chardata_for_range = font->char_data + ranges[i].first_unicode_codepoint_in_range;
			}

			stbtt_PackFontRanges( &font->pack_context, font->file_buffer, 0, ranges, nranges );
		}
	}

	return true;
}

bool pack_thai( const char *file_name, font_t *font ) {
	printf( "loading thai font `%s`:\n", file_name );
	assert( file_name );
	assert( font );
	assert( font->atlas_image );

	FILE *fptr = fopen( file_name, "rb" );
	if ( !fptr ) {
		fprintf( stderr, "ERROR: could not open file `%s`\n", file_name );
		return false;
	}
	{ // read entire file into buffer
		fseek( fptr, 0, SEEK_END );
		size_t sz = ftell( fptr );
		rewind( fptr );
		font->file_buffer_thai = (unsigned char *)malloc( sz );
		assert( font->file_buffer_thai );
		printf( " * allocated %i thai\n", (int)sz );
		fread( font->file_buffer_thai, sz, 1, fptr );
	}
	fclose( fptr );

	int nfonts = stbtt_GetNumberOfFonts( font->file_buffer_thai );
	printf( " * %i font(s) in file\n", nfonts );
	int font_offset = stbtt_GetFontOffsetForIndex( font->file_buffer_thai, 0 );
	printf( " * offset for font 0 is %i \n", font_offset );
	int ret = stbtt_InitFont( &font->font_info_stb_thai, font->file_buffer_thai, font_offset );
	if ( !ret ) {
		fprintf( stderr, "ERROR: failed to init font\n" );
		return false;
	}
	printf( " * %i glyphs\n", font->font_info_stb_thai.numGlyphs );
	font->scale_stb_thai = stbtt_ScaleForPixelHeight( &font->font_info_stb_thai, font->scale_px );
	printf( " * px scale=%.2f, stb scale=%.2f\n", font->scale_px, font->scale_stb );

	stbtt_pack_range ranges[1];
	memset( ranges, 0, sizeof( stbtt_pack_range ) * 1 );
	int nranges = 0;
	if ( thai ) {
		ranges[nranges].first_unicode_codepoint_in_range = 0x0E00;
		ranges[nranges].num_chars = 0x0E7F - 0x0E00;
		ranges[nranges].font_size = font->scale_px;
		ranges[nranges].chardata_for_range = font->char_data + ranges[nranges].first_unicode_codepoint_in_range;
		nranges++;
	}
	stbtt_PackFontRanges( &font->pack_context, font->file_buffer_thai, 0, ranges, nranges );

	return true;
}

bool pack_cjk( const char *file_name, font_t *font ) {
	printf( "loading cjk font `%s`:\n", file_name );
	assert( file_name );
	assert( font );
	assert( font->atlas_image );

	FILE *fptr = fopen( file_name, "rb" );
	if ( !fptr ) {
		fprintf( stderr, "ERROR: could not open file `%s`\n", file_name );
		return false;
	}
	{ // read entire file into buffer
		fseek( fptr, 0, SEEK_END );
		size_t sz = ftell( fptr );
		rewind( fptr );
		font->file_buffer_cjk = (unsigned char *)malloc( sz );
		assert( font->file_buffer_cjk );
		printf( " * allocated %i cjk\n", (int)sz );
		fread( font->file_buffer_cjk, sz, 1, fptr );
	}
	fclose( fptr );

	int nfonts = stbtt_GetNumberOfFonts( font->file_buffer_cjk );
	printf( " * %i font(s) in file\n", nfonts );
	int font_offset = stbtt_GetFontOffsetForIndex( font->file_buffer_cjk, 0 );
	printf( " * offset for font 0 is %i \n", font_offset );
	int ret = stbtt_InitFont( &font->font_info_stb_cjk, font->file_buffer_cjk, font_offset );
	if ( !ret ) {
		fprintf( stderr, "ERROR: failed to init font\n" );
		return false;
	}
	printf( " * %i glyphs\n", font->font_info_stb_cjk.numGlyphs );
	font->scale_stb_cjk = stbtt_ScaleForPixelHeight( &font->font_info_stb_cjk, font->scale_px );
	printf( " * px scale=%.2f, stb scale=%.2f\n", font->scale_px, font->scale_stb );

	int ncjk_actually_used_codepoints = sizeof( cjk_codepoints ) / sizeof( int );
	printf( " * ncjk_actually_used_codepoints = %i\n", ncjk_actually_used_codepoints );

	{
		stbtt_pack_range ranges[7 + ncjk_actually_used_codepoints];
		memset( ranges, 0, sizeof( stbtt_pack_range ) * 6 + ncjk_actually_used_codepoints );
		int nranges = 0;
		int max_codepoint = 0xffef;

		if ( cjk_punct ) {
			ranges[nranges].first_unicode_codepoint_in_range = 0x3000;
			ranges[nranges].num_chars = 0x303f - 0x3000;
			nranges++;
		}
		if ( hirigana ) {
			ranges[nranges].first_unicode_codepoint_in_range = 0x3040;
			ranges[nranges].num_chars = 0x309F - 0x3040;
			nranges++;
		}
		if ( katakana ) {
			ranges[nranges].first_unicode_codepoint_in_range = 0x30A0;
			ranges[nranges].num_chars = 0x30FF - 0x30A0;
			nranges++;
		}
		if ( bopomofo ) {
			ranges[nranges].first_unicode_codepoint_in_range = 0x3100;
			ranges[nranges].num_chars = 0x312F - 0x3100;
			nranges++;
		}
		if ( hangul ) {
			ranges[nranges].first_unicode_codepoint_in_range = 0x3130;
			ranges[nranges].num_chars = 0x318F - 0x3130;
			nranges++;
		}
		if ( romaji ) {
			ranges[nranges].first_unicode_codepoint_in_range = 0xff00;
			ranges[nranges].num_chars = 0xffef - 0xff00;
			nranges++;
		}

		for ( int i = 0; i < nranges; i++ ) {
			ranges[i].font_size = font->scale_px;
			ranges[i].chardata_for_range = font->char_data + ranges[i].first_unicode_codepoint_in_range;
		}

		if ( custom_cjk ) {
			int mincjk = 999999999;
			// cjk_used_in_app -- putting each glyph in its own range so it plays well with later char data lookup
			for ( int i = 0; i < ncjk_actually_used_codepoints; i++ ) {
				ranges[nranges].font_size = font->scale_px;
				ranges[nranges].chardata_for_range = font->char_data + cjk_codepoints[i];
				ranges[nranges].array_of_unicode_codepoints = &cjk_codepoints[i];
				ranges[nranges].num_chars = 1;
				if ( cjk_codepoints[i] > max_codepoint ) {
					max_codepoint = cjk_codepoints[i];
				}
				if ( cjk_codepoints[i] < mincjk ) {
					mincjk = cjk_codepoints[i];
				}
				nranges++;
			}
			printf( " * custom cjk min/max codepoints = %i/%i\n", mincjk, max_codepoint );
			assert( max_codepoint < MAX_CODEPOINT ); // otherwise realloc
		}

		stbtt_PackFontRanges( &font->pack_context, font->file_buffer_cjk, 0, ranges, nranges );
	}

	return true;
}

void finalise_font( font_t *font ) {
	assert( font );

	stbtt_PackEnd( &font->pack_context );
}

void create_opengl_font_atlas( font_t *font ) {
	assert( font );
	assert( font->atlas_image );

	if ( !font->atlas_texture ) {
		glGenTextures( 1, &font->atlas_texture );
	}

	glBindTexture( GL_TEXTURE_2D, font->atlas_texture );
	{
		glTexImage2D( GL_TEXTURE_2D, 0, GL_RED, font->atlas_w, font->atlas_h, 0, GL_RED, GL_UNSIGNED_BYTE, font->atlas_image );
		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );
	}
	glBindTexture( GL_TEXTURE_2D, 0 );

	free( font->atlas_image );
	font->atlas_image = NULL;
}

void free_font( font_t *font ) {
	assert( font );
	assert( font->file_buffer );
	assert( font->char_data );

	free( font->char_data );
	free( font->file_buffer );
	if ( font->file_buffer_cjk ) {
		free( font->file_buffer_cjk );
	}
	if ( font->file_buffer_thai ) {
		free( font->file_buffer_thai );
	}
	if ( font->atlas_image ) {
		free( font->atlas_image );
	}
	if ( font->atlas_texture ) {
		glDeleteTextures( 1, &font->atlas_texture );
	}

	memset( font, 0, sizeof( font_t ) );
}

int utf8_decode( const char *buf, int *nbytes ) {
	// 1
	unsigned char u0 = (unsigned char)buf[0];
	*nbytes = 1;
	if ( buf[0] >= 0 && u0 <= 127 ) {
		return u0;
	}

	// 2
	unsigned char u1 = (unsigned char)buf[1];
	*nbytes = 2;
	if ( u0 >= 192 && u0 <= 223 ) {
		return ( u0 - 192 ) * 64 + ( u1 - 128 );
	}

	// WARNING(Anton) comparison is always false due to limited range of data type [-Wtype-limits]  if ( buf[0] == 0xed && (
	// buf[1] & 0xa0 ) == 0xa0 ) {
	// if ( buf[0] == 0xed && ( buf[1] & 0xa0 ) == 0xa0 ) {
	//	return -1; // code points, 0xd800 to 0xdfff
	//}

	// 3
	unsigned char u2 = (unsigned char)buf[2];
	*nbytes = 3;
	if ( u0 >= 224 && u0 <= 239 ) {
		return ( u0 - 224 ) * 4096 + ( u1 - 128 ) * 64 + ( u2 - 128 );
	}

	// 4
	unsigned char u3 = (unsigned char)buf[3];
	*nbytes = 4;
	if ( u0 >= 240 && u0 <= 247 ) {
		return ( u0 - 240 ) * 262144 + ( u1 - 128 ) * 4096 + ( u2 - 128 ) * 64 + ( u3 - 128 );
	}

	return -1;
}

// returns number of codepoints scanned from string
int update_string_geom( const font_t *font, float x, float y, const char *text, unsigned int vbo, float scale ) {
	assert( text );
	assert( font );
	assert( vbo > 0 );

	float initial_x = x;

	int glyphs_added = 0;
	int comps_added = 0;
	int str_sz = strlen( text ); // TODO unicode decode

	float *geom_data = (float *)calloc( sizeof( float ), str_sz * 6 * 4 );
	assert( geom_data );
	{
		int i = 0;
		while ( i < str_sz ) {

			// get ascii code as integer
			int nbytes = 0;
			uint32_t ascii_code = utf8_decode( &text[i], &nbytes );
			if ( 0 == nbytes ) {
				printf( "WARNING: utf8 char invalid\n" );
				return glyphs_added;
			}
			i += nbytes;

			if ( '\n' == ascii_code ) {
				x = initial_x;
				y += font->scale_px;
			}

			{
				stbtt_aligned_quad q;
				// TODO - detect if cjk code points first
				stbtt_GetPackedQuad( font->char_data, font->atlas_w, font->atlas_h, ascii_code, &x, &y, &q, 0 ); // align to integer ??

				geom_data[comps_added++] = q.x0 * scale; // tl
				geom_data[comps_added++] = -q.y1 * scale;
				geom_data[comps_added++] = q.s0;
				geom_data[comps_added++] = q.t1;
				geom_data[comps_added++] = q.x1 * scale; // tr
				geom_data[comps_added++] = -q.y1 * scale;
				geom_data[comps_added++] = q.s1;
				geom_data[comps_added++] = q.t1;
				geom_data[comps_added++] = q.x1 * scale; // br
				geom_data[comps_added++] = -q.y0 * scale;
				geom_data[comps_added++] = q.s1;
				geom_data[comps_added++] = q.t0;
				geom_data[comps_added++] = q.x1 * scale; // br
				geom_data[comps_added++] = -q.y0 * scale;
				geom_data[comps_added++] = q.s1;
				geom_data[comps_added++] = q.t0;
				geom_data[comps_added++] = q.x0 * scale; // bl
				geom_data[comps_added++] = -q.y0 * scale;
				geom_data[comps_added++] = q.s0;
				geom_data[comps_added++] = q.t0;
				geom_data[comps_added++] = q.x0 * scale; // tl
				geom_data[comps_added++] = -q.y1 * scale;
				geom_data[comps_added++] = q.s0;
				geom_data[comps_added++] = q.t1;

				// kerning
				if ( i < str_sz ) {
					uint32_t next_code = utf8_decode( &text[i], &nbytes );
					int kern = stbtt_GetCodepointKernAdvance( &font->font_info_stb, ascii_code, next_code );
					if ( kern ) {
						printf( "kerning %c,%c by %i\n", ascii_code, next_code, kern );
					}

					x += kern * font->scale_stb;
				}

				glyphs_added++;
			}
		} // endwhiles

		glBindBuffer( GL_ARRAY_BUFFER, vbo );
		glBufferData( GL_ARRAY_BUFFER, sizeof( float ) * comps_added, geom_data, GL_STATIC_DRAW );
		glBindBuffer( GL_ARRAY_BUFFER, 0 );
		printf( "added %i comps\n", comps_added );
	} // endblock geom data
	free( geom_data );

	return glyphs_added;
}