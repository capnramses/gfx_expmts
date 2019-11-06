#pragma once

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#define MAX_ATLAS_GLYPHS 512

typedef struct Font{
	GLuint texture;
	GLuint shader_prog;
	GLint sp_text_colour_loc;
	float glyph_widths[MAX_ATLAS_GLYPHS];
	float glyph_y_offsets[MAX_ATLAS_GLYPHS];
}Font;

Font load_font(const char* img_fn, const char* meta_fn);
void text_to_vbo(const char* str, Font font, int vp_width, int vp_height,
	float at_x, float at_y, float scale_px,
	GLuint* points_vbo, GLuint* texcoords_vbo, int* point_count);
