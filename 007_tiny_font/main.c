//
// Basic Spinning Cube in OpenGL 2.1
// Anton Gerdelan
// 23 Nov 2014
//
#include "maths_funcs.h"
#include "obj_parser.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h> // memset
#define _USE_MATH_DEFINES
#include <math.h>
#ifndef M_PI
	#define M_PI 3.14159265358979323846
#endif
#define HALF_PI M_PI / 2
#include <wchar.h>
typedef wchar_t wchar;
#include <locale.h>
#include <stdbool.h>

//
// dimensions of the window drawing surface
int gl_width = 800;
int gl_height = 800;

#define RES 512
#define XOFF 12

// rgb working image
unsigned char img[RES * RES * 3];

// rgb font image
unsigned char* font_img;
int font_w;

//
// draws a red pixel in the image buffer
//
void plot (int x, int y, int r, int g, int b) {
	if (x < 0 || y < 0 || x >= RES || y >= RES) {
		return;
	}
	int index = y * RES + x;
	int red = index * 3;
	img[red] = r;
	img[red + 1] = g;
	img[red + 2] = b;
}

int glyph_widths[] = {
	1, 3, 5, 3, 4, 4, 1, 2, // !"#$%&'(
	2, 3, 3, 2, 3, 1, 4, // )*+,-./
	4, 4, 4, 4, 4, 4, 4, 4, 4, 4, // 0-9
	2, 2, 3, 4, 3, 4, 4, // :;<=>?@
	4, 4, 4, 4, // ABCD
	4, 4, 4, 4, // EFGH
	3, 3, 4, 4, // IJKL
	5, 4, 4, 4, // MNOP
	4, 4, 4, 3, // QRST
	4, 5, 5, 5, // UVWX
	5, 4, // YZ
	2, 4, 2, 3, 4, 2, // [\]^_`
	3, 3, 3, 3, // abcd
	3, 3, 3, 3, // efgh
	1, 2, 3, 1, // ijkl
	4, 3, 3, 3, // mnop
	3, 4, 3, 3, // qrst
	3, 3, 4, 3, // uvwx
	3, 3, // yz
	3, 1, 3, 4, // {|}~
	4, 5, 4, 5, // Gamma, Delta, Theta, Lamda
	4, 4, 4, 5, // Xi, Pi, Sigma, Upsilon
	5, 5, 5, // Phi, Psi, Omega
	4, 3, 3, 3, // alpha, beta, gamma, delta,
	2, 3, 4, 3, // epsilon, digamma, zeta, eta,
	3, 2, 3, 5, // theta, iota, kappa, lamda
	3, 3, 3, 5, // mu, nu, xi, pi
	4, 4, 4, 3, // rho, sigma, tau, upsilon
	5, 4, 5, 5 // phi, chi, psi, omega
};

//
// x,y is the bottom-left pixel of the first glyph
// code is the ascii code
//
void plot_glyph (int x, int y, int code, int r, int g, int b, int size_fac,
	bool alt) {
	int code_index = code - 33;
	assert (code_index > 0 && code_index < font_w / 5);
	int w = glyph_widths[code_index];
	for (int yi = 0; yi < 5 * size_fac; yi++) {
		for (int xi = 0; xi < w * size_fac; xi++) {
			unsigned char val_red = font_img[(yi / size_fac * font_w +
				xi / size_fac + code_index * 5)];
			if (val_red) {
				if (alt) {
					plot (x + yi - 5 * size_fac + 1, y - xi, r, g, b);
				} else {
					plot (x + xi, y + yi - 5 * size_fac + 1, r, g, b);
				}
			}
		} // endfor
	} // endfor
}

void plot_str (int x, int y, const wchar* str, int r, int g, int b,
	int size_fac, bool alt) {
	assert (str);
	int len = wcslen (str);
	int curr_x = x;
	int curr_y = y;
	for (int i = 0; i < len; i++) {
		if (str[i] == ' ') {
			if (alt) {
				curr_y -= 4 * size_fac;
			} else {
				curr_x += 4 * size_fac;
			}
		} else if (str[i] == '\t') {
			if (alt) {
				curr_y -= 8 * size_fac;
			} else {
				curr_x += 8 * size_fac;
			}
		} else if (str[i] == '\n') {
			if (alt) {
				curr_x += 6 * size_fac;
				curr_y = y;
			} else {
				curr_y += 6 * size_fac;
				curr_x = x;
			}
		} else if (str[i] >= 33 && str[i] <= 126) {
			plot_glyph (curr_x, curr_y, str[i], r, g, b, size_fac, alt);
			int w = glyph_widths[str[i] - 33];
			if (alt) {
				curr_y -= (w * size_fac + 1);
			} else {
				curr_x += (w * size_fac + 1);
			}
		// lower-case greek
		} else {
			int code = 0;
			switch (str[i]) {
				case 0x0393: code = '~' + 1; break; // Gamma
				case 0x0394: code = '~' + 2; break; // Delta
				case 0x0398: code = '~' + 3; break; // Theta
				case 0x039B: code = '~' + 4; break; // Lambda
				case 0x039E: code = '~' + 5; break; // Xi
				case 0x03A0: code = '~' + 6; break; // Pi
				case 0x03A3: code = '~' + 7; break; // Sigma
				case 0x03A5: code = '~' + 8; break; // Upsilon
				case 0x03A6: code = '~' + 9; break; // Phi
				case 0x03A8: code = '~' + 10; break; // Psi
				case 0x03A9: code = '~' + 11; break; // Omega
				case 0x03B1: code = '~' + 12; break; // alpha
				case 0x03B2: code = '~' + 13; break; // beta
				case 0x03B3: code = '~' + 14; break; // gamma
				case 0x03B4: code = '~' + 15; break; // delta
				case 0x03B5: code = '~' + 16; break; // epsilon
				case 0x03DD: code = '~' + 17; break; // digamma
				case 0x03B6: code = '~' + 18; break; // zeta
				case 0x03B7: code = '~' + 19; break; // eta
				case 0x03B8: code = '~' + 20; break; // theta
				case 0x03B9: code = '~' + 21; break; // iota
				case 0x03BA: code = '~' + 22; break; // kappa
				case 0x03BB: code = '~' + 23; break; // lamda
				case 0x03BC: code = '~' + 24; break; // mu
				case 0x03BD: code = '~' + 25; break; // nu
				case 0x03BE: code = '~' + 26; break; // xi
				case 0x03C0: code = '~' + 27; break; // pi
				case 0x03C1: code = '~' + 28; break; // rho
				case 0x03C3: code = '~' + 29; break; // sigma
				case 0x03C4: code = '~' + 30; break; // tau
				case 0x03C5: code = '~' + 31; break; // upsilon
				case 0x03C6: code = '~' + 32; break; // phi
				case 0x03C7: code = '~' + 33; break; // chi
				case 0x03C8: code = '~' + 34; break; // psi
				case 0x03C9: code = '~' + 35; break; // omega
				default:
					printf ("unhandled char, code dec %i hex %#x\n", str[i], str[i]);
					return;
			} // endswitch
			plot_glyph (curr_x, curr_y, code, r, g, b, size_fac, alt);
			int w = glyph_widths[code - 33];
			if (alt) {
				curr_y -= (w * size_fac + 1);
			} else {
				curr_x += (w * size_fac + 1);
			}
		}
	}
}

//
// copy a shader from a plain text file into a character array
bool parse_file_into_str (
	const char* file_name, char* shader_str, int max_len
) {
	FILE* file = fopen (file_name , "r");
	int current_len = 0;
	char line[2048];

	shader_str[0] = '\0'; /* reset string */
	if (!file) {
		fprintf (stderr, "ERROR: opening file for reading: %s\n", file_name);
		return false;
	}
	strcpy (line, ""); /* remember to clean up before using for first time! */
	while (!feof (file)) {
		if (NULL != fgets (line, 2048, file)) {
			current_len += strlen (line); /* +1 for \n at end */
			if (current_len >= max_len) {
				fprintf (stderr, 
					"ERROR: shader length is longer than string buffer length %i\n",
					max_len
				);
			}
			strcat (shader_str, line);
		}
	}
	if (EOF == fclose (file)) { /* probably unnecesssary validation */
		fprintf (stderr, "ERROR: closing file from reading %s\n", file_name);
		return false;
	}
	return true;
}

int main () {
	GLFWwindow* window = NULL;
	const GLubyte* renderer;
	const GLubyte* version;
	GLuint shader_programme;
	GLuint vao;

	setlocale (LC_ALL, "");

	//
	// Start OpenGL using helper libraries
	// --------------------------------------------------------------------------
	if (!glfwInit ()) {
		fprintf (stderr, "ERROR: could not start GLFW3\n");
		return 1;
	} 

	/* change to 3.2 if on Apple OS X
	glfwWindowHint (GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint (GLFW_CONTEXT_VERSION_MINOR, 0);
	glfwWindowHint (GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
	glfwWindowHint (GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE); */

	window = glfwCreateWindow (gl_width, gl_height, "Spinning Cube", NULL, NULL);
	if (!window) {
		fprintf (stderr, "ERROR: opening OS window\n");
		return 1;
	}
	glfwMakeContextCurrent (window);

	glewExperimental = GL_TRUE;
	glewInit ();

	/* get version info */
	renderer = glGetString (GL_RENDERER); /* get renderer string */
	version = glGetString (GL_VERSION); /* version as a string */
	printf ("Renderer: %s\n", renderer);
	printf ("OpenGL version supported %s\n", version);

	int point_count = 0;
	//
	// Set up vertex buffers and vertex array object
	// --------------------------------------------------------------------------
	{
		GLfloat* vp = NULL; // array of vertex points
		GLfloat* vn = NULL; // array of vertex normals (we haven't used these yet)
		GLfloat* vt = NULL; // array of texture coordinates (or these)
		assert (load_obj_file ("cube.obj", vp, vt, vn, point_count));
		
	
		GLuint points_vbo, texcoord_vbo;
		glGenBuffers (1, &points_vbo);
		glBindBuffer (GL_ARRAY_BUFFER, points_vbo);
		// copy our points from the header file into our VBO on graphics hardware
		glBufferData (GL_ARRAY_BUFFER, sizeof (float) * 3 * point_count,
			vp, GL_STATIC_DRAW);
		// and grab the normals
		glGenBuffers (1, &texcoord_vbo);
		glBindBuffer (GL_ARRAY_BUFFER, texcoord_vbo);
		glBufferData (GL_ARRAY_BUFFER, sizeof (float) * 2 * point_count,
			vt, GL_STATIC_DRAW);
	
		glGenVertexArrays (1, &vao);
		glBindVertexArray (vao);
		glEnableVertexAttribArray (0);
		glBindBuffer (GL_ARRAY_BUFFER, points_vbo);
		glVertexAttribPointer (0, 3, GL_FLOAT, GL_FALSE, 0, NULL);
		glEnableVertexAttribArray (1);
		glBindBuffer (GL_ARRAY_BUFFER, texcoord_vbo);
		glVertexAttribPointer (1, 2, GL_FLOAT, GL_FALSE, 0, NULL);
		
		free (vp);
		free (vn);
		free (vt);
	}
	//
	// Load shaders from files
	// --------------------------------------------------------------------------
	{
		char* vertex_shader_str;
		char* fragment_shader_str;
		
		// allocate some memory to store shader strings
		vertex_shader_str = (char*)malloc (81920);
		fragment_shader_str = (char*)malloc (81920);
		// load shader strings from text files
		assert (parse_file_into_str ("teapot.vert", vertex_shader_str, 81920));
		assert (parse_file_into_str ("teapot.frag", fragment_shader_str, 81920));
		GLuint vs, fs;
		vs = glCreateShader (GL_VERTEX_SHADER);
		fs = glCreateShader (GL_FRAGMENT_SHADER);
		glShaderSource (vs, 1, (const char**)&vertex_shader_str, NULL);
		glShaderSource (fs, 1, (const char**)&fragment_shader_str, NULL);
		// free memory
		free (vertex_shader_str);
		free (fragment_shader_str);
		glCompileShader (vs);
		glCompileShader (fs);
		shader_programme = glCreateProgram ();
		glAttachShader (shader_programme, fs);
		glAttachShader (shader_programme, vs);
		glLinkProgram (shader_programme);
		/* TODO NOTE: you should check for errors and print logs after compiling and also linking shaders */
	}
	
	//
	// Create some matrices
	// --------------------------------------------------------------------------
		
	mat4 M, V, P;
	M = identity_mat4 ();//scale (identity_mat4 (), vec3 (0.05, 0.05, 0.05));
	vec3 cam_pos (-1.0, 0.0, 2.0);
	vec3 targ_pos (-1.0, 0.0, 0.0);
	vec3 up (0.0, 1.0, 0.0);
	V = look_at (cam_pos, targ_pos, up);
	P = perspective (67.0f, (float)gl_width / (float)gl_height, 0.1, 1000.0);
	
	int M_loc = glGetUniformLocation (shader_programme, "M");
	int V_loc = glGetUniformLocation (shader_programme, "V");
	int P_loc = glGetUniformLocation (shader_programme, "P");
	// send matrix values to shader immediately
	glUseProgram (shader_programme);
	glUniformMatrix4fv (M_loc, 1, GL_FALSE, M.m);
	glUniformMatrix4fv (V_loc, 1, GL_FALSE, V.m);
	glUniformMatrix4fv (P_loc, 1, GL_FALSE, P.m);
	
	// load font
	int in, font_h;
	font_img = stbi_load ("font.png", &font_w, &font_h, &in, 1);
	
	// grey
	memset (img, 200, RES * RES * 3);
	plot_str (10, 100, L"As I was going to St. Ives\nI met a man with seven wives.\n"
	"Each wife had seven sacks\nEach sack had seven cats\nEach cat had seven kits\n"
	"Kits, cats, sacks, and wives\nHow many were going to\nSt. Ives?\n\n"
	"Δ δ Π π Θ θ Σ σ", 0,0,0,1, false);
	
	GLuint tex;
	glGenTextures (1, &tex);
	glActiveTexture (GL_TEXTURE0);
	glBindTexture (GL_TEXTURE_2D, tex);
	glTexImage2D (
		GL_TEXTURE_2D,
		0,
		GL_RGB,
		RES,
		RES,
		0,
		GL_RGB,
		GL_UNSIGNED_BYTE,
		img
	);
	glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	
	//
	// Start rendering
	// --------------------------------------------------------------------------
	// tell GL to only draw onto a pixel if the fragment is closer to the viewer
	glEnable (GL_DEPTH_TEST); // enable depth-testing
	glDepthFunc (GL_LESS); // depth-testing interprets a smaller value as "closer"
	glClearColor (0.01, 0.01, 0.25, 1.0);

	float a = 0.0f;
	double prev = glfwGetTime ();
	while (!glfwWindowShouldClose (window)) {
		glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		// just the default viewport, covering the whole render area
		glViewport (0, 0, gl_width, gl_height);
		
		double curr = glfwGetTime ();
		double elapsed = curr - prev;
		prev = curr;
		
		glUseProgram (shader_programme);
		glBindVertexArray (vao);
		
		//a += sinf (elapsed * 50.0f);
		a = 0.0f;
		M = rotate_y_deg (identity_mat4 (), a);
		glUniformMatrix4fv (M_loc, 1, GL_FALSE, M.m);
		
		glDrawArrays (GL_TRIANGLES, 0, point_count);

		/* this just updates window events and keyboard input events (not used yet) */
		glfwPollEvents ();
		glfwSwapBuffers (window);
	}

	return 0;
}
