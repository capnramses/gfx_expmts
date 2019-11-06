#include "text.h"
#include "stb/stb_image.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>

#define ATLAS_COLS 16
#define ATLAS_ROWS 16

Font load_font(const char* img_fn, const char* meta_fn) {
	Font font;
	{
		FILE* fp = fopen (meta_fn, "r");
		assert (fp);
		char line [128];
		int ascii_code = -1;
		float prop_xMin = 0.0f;
		float prop_width = 0.0f;
		float prop_yMin = 0.0f;
		float prop_height = 0.0f;
		float prop_y_offset = 0.0f;
		// get header line first
		fgets (line, 128, fp);
		// loop through and get each glyph's info
		while(EOF != fscanf (fp, "%i %f %f %f %f %f\n",
			&ascii_code,
			&prop_xMin,
			&prop_width,
			&prop_yMin,
			&prop_height,
			&prop_y_offset)){
			font.glyph_widths[ascii_code] = prop_width;
			font.glyph_y_offsets[ascii_code] = 1.0 - prop_height - prop_y_offset;
		}
		fclose (fp);
	}
	{
		int x = 0, y = 0, n = 0, force_channels = 4;
		unsigned char* image_data = stbi_load(img_fn, &x, &y, &n, force_channels);
		assert(image_data);
		if ((x & (x - 1)) != 0 || (y & (y - 1)) != 0) {
			fprintf (stderr, "WARNING: image %s is not power-of-2 dimensions\n", img_fn);
		}
		int width_in_bytes = x * 4;
		unsigned char *top = NULL;
		unsigned char *bottom = NULL;
		unsigned char temp = 0;
		int half_height = y / 2;
		for (int row = 0; row < half_height; row++) {
			top = image_data + row * width_in_bytes;
			bottom = image_data + (y - row - 1) * width_in_bytes;
			for (int col = 0; col < width_in_bytes; col++) {
				temp = *top;
				*top = *bottom;
				*bottom = temp;
				top++;
				bottom++;
			}
		}
		glActiveTexture(GL_TEXTURE0);
		glGenTextures(1, &font.texture);
		glBindTexture (GL_TEXTURE_2D, font.texture);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, x, y, 0, GL_RGBA, GL_UNSIGNED_BYTE, image_data);
		glGenerateMipmap (GL_TEXTURE_2D);
		glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		GLfloat max_aniso = 0.0f;
		glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &max_aniso);
		glTexParameterf (GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, max_aniso);
		free(image_data);
	}
	{
		/* here i used negative y from the buffer as the z value so that it was on
		the floor but also that the 'front' was on the top side. also note how i
		work out the texture coordinates, st, from the vertex point position */
		const char* vs_str =
		"#version 300 es\n"
		"layout (location = 0) in vec2 vp;"
		"layout (location = 1) in vec2 vt;"
		"out vec2 st;"
		"void main () {"
		"  st = vt;"
		"  gl_Position = vec4 (vp, 0.0, 1.0);"
		"}";
		const char* fs_str =
		"#version 300 es\n"
		"precision mediump float;" // need this
		"in vec2 st;"
		"uniform sampler2D tex;"
		"uniform vec4 text_colour;"
		"out vec4 frag_colour;"
		"void main () {"
		"  frag_colour = texture (tex, st) * text_colour;"
		"}";
		GLuint vs = glCreateShader (GL_VERTEX_SHADER);
		glShaderSource (vs, 1, &vs_str, NULL);
		glCompileShader (vs);
		int params = -1;
		glGetShaderiv (vs, GL_COMPILE_STATUS, &params);
		if (GL_TRUE != params) {
			fprintf (stderr, "ERROR: GL shader index %i did not compile\n", vs);
		}
		GLuint fs = glCreateShader (GL_FRAGMENT_SHADER);
		glShaderSource (fs, 1, &fs_str, NULL);
		glCompileShader (fs);
		glGetShaderiv (fs, GL_COMPILE_STATUS, &params);
		if (GL_TRUE != params) {
			fprintf (stderr, "ERROR: GL shader index %i did not compile\n", fs);
		}
		font.shader_prog = glCreateProgram ();
		glAttachShader (font.shader_prog, vs);
		glAttachShader (font.shader_prog, fs);
		glLinkProgram (font.shader_prog);
		glGetProgramiv (font.shader_prog, GL_LINK_STATUS, &params);
		if (GL_TRUE != params) {
			fprintf (stderr, "ERROR: could not link shader programme GL index %i\n",
				font.shader_prog);
		}
		font.sp_text_colour_loc = glGetUniformLocation (font.shader_prog, "text_colour");
	}
	return font;
}

void text_to_vbo(const char* str, Font font, int vp_width, int vp_height,
	float at_x, float at_y, float scale_px,
	GLuint* points_vbo, GLuint* texcoords_vbo, int* point_count) {
	int len = strlen(str);
	assert(len > 0);
	float* points_tmp = (float*)malloc(sizeof (float) * len * 12);
	assert(points_tmp);
	float* texcoords_tmp = (float*)malloc(sizeof (float) * len * 12);
	assert(texcoords_tmp);
	for (int i = 0; i < len; i++) {
		// get ascii code as integer
		int ascii_code = str[i];
		// work out row and column in atlas
		int atlas_col = (ascii_code - ' ') % ATLAS_COLS;
		int atlas_row = (ascii_code - ' ') / ATLAS_COLS;
		// work out texture coordinates in atlas
		float s = atlas_col * (1.0 / ATLAS_COLS);
		float t = (atlas_row + 1) * (1.0 / ATLAS_ROWS);
		// work out position of glyphtriangle_width
		float x_pos = at_x;
		float y_pos = at_y - scale_px / vp_height * font.glyph_y_offsets[ascii_code];
		// move next glyph along to the end of this one
		if (i + 1 < len) {
			// upper-case letters move twice as far
			at_x += font.glyph_widths[ascii_code] * scale_px / vp_width;
		}
		// add 6 points and texture coordinates to buffers for each glyph
		points_tmp[i * 12] = x_pos;
		points_tmp[i * 12 + 1] = y_pos;
		points_tmp[i * 12 + 2] = x_pos;
		points_tmp[i * 12 + 3] = y_pos - scale_px / vp_height;
		points_tmp[i * 12 + 4] = x_pos + scale_px / vp_width;
		points_tmp[i * 12 + 5] = y_pos - scale_px / vp_height;
		
		points_tmp[i * 12 + 6] = x_pos + scale_px / vp_width;
		points_tmp[i * 12 + 7] = y_pos - scale_px / vp_height;
		points_tmp[i * 12 + 8] = x_pos + scale_px / vp_width;
		points_tmp[i * 12 + 9] = y_pos;
		points_tmp[i * 12 + 10] = x_pos;
		points_tmp[i * 12 + 11] = y_pos;
		
		texcoords_tmp[i * 12] = s;
		texcoords_tmp[i * 12 + 1] = 1.0 - t + 1.0 / ATLAS_ROWS;
		texcoords_tmp[i * 12 + 2] = s;
		texcoords_tmp[i * 12 + 3] = 1.0 - t;
		texcoords_tmp[i * 12 + 4] = s + 1.0 / ATLAS_COLS;
		texcoords_tmp[i * 12 + 5] = 1.0 - t;
		
		texcoords_tmp[i * 12 + 6] = s + 1.0 / ATLAS_COLS;
		texcoords_tmp[i * 12 + 7] = 1.0 - t;
		texcoords_tmp[i * 12 + 8] = s + 1.0 / ATLAS_COLS;
		texcoords_tmp[i * 12 + 9] = 1.0 - t + 1.0 / ATLAS_ROWS;
		texcoords_tmp[i * 12 + 10] = s;
		texcoords_tmp[i * 12 + 11] = 1.0 - t + 1.0 / ATLAS_ROWS;
	}
	glBindBuffer (GL_ARRAY_BUFFER, *points_vbo);
	glBufferData (GL_ARRAY_BUFFER, len * 12 * sizeof (float), points_tmp, GL_DYNAMIC_DRAW);
	glBindBuffer (GL_ARRAY_BUFFER, *texcoords_vbo);
	glBufferData (GL_ARRAY_BUFFER, len * 12 * sizeof (float), texcoords_tmp, GL_DYNAMIC_DRAW);
	free (points_tmp);
	free (texcoords_tmp);
	*point_count = len * 6;
}

