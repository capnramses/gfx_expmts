/*
stencil test play-around

-- stencil test is performed AFTER frag shader
-- fragment's STENCIL VALUE used in test vs current value in STENCIL BUFFER
-- frag culled on test fail

* glEnable (GL_STENCIL_TEST)
* add stencil buffer to fb
* sb is an image using stencil format
* for use fb attach buffer to GL_STENCIL_ATTACHMENT
* specify bit-depth of stencil
* each fragment has a uint stencil value
* usually set fragment values per-draw call in C
* GL_STENCIL_BUFFER_BIT to glClear
* set stencil is 2-sided for triangles front/back/frontandback
* glStencilFuncSeparate to set the test used or glStencilOp

* -- glStencilMask() defines stencil values before drawing

*/

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <stdio.h>
#include <stdbool.h>

#define IMG_FILE "interior_cover.jpg"

int main () {
	printf ("texture compression experiment\n");

	if (!glfwInit ()) {
		fprintf (stderr, "FATAL ERROR: could not start GLFW3\n");
		return 1;
	}

#ifdef APPLE
	glfwWindowHint (GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint (GLFW_CONTEXT_VERSION_MINOR, 2);
	glfwWindowHint (GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
	glfwWindowHint (GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#else
	glfwWindowHint (GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint (GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint (GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
	glfwWindowHint (GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#endif

	GLFWwindow* window = glfwCreateWindow (1000, 1200, "vid modes", NULL, NULL);
	if (!window) {
		fprintf (stderr, "ERROR: could not open window with GLFW3\n");
		glfwTerminate();
		return 1;
	}
	glfwMakeContextCurrent (window);
	
	// start GLEW extension handler
	glewExperimental = GL_TRUE;
	glewInit ();
	
	// get version info
	const GLubyte* renderer = glGetString (GL_RENDERER); // get renderer string
	const GLubyte* version = glGetString (GL_VERSION); // version as a string
	printf ("Renderer: %s\n", renderer);
	printf ("OpenGL version supported %s\n", version);

	// check for extension
	bool compress = true;
	if (GLEW_EXT_texture_compression_s3tc) {
		printf ("EXT_texture_compression_s3tc FOUND!\n");
	} else {
		printf ("EXT_texture_compression_s3tc NOT FOUND!\n");
		compress = false; // fall back!
	}

	// load image normally
	int x, y, n;
	x = y = n = -1;
	unsigned char* data = stbi_load (IMG_FILE, &x, &y, &n, 4);
	printf ("image %s loaded with %ix%i, %i chans\n", IMG_FILE, x, y, n);
	if ((x & (x - 1)) != 0 || (y & (y - 1)) != 0) {
		fprintf (stderr, "WARNING: texture %s is not power-of-2 dimensions\n",
			IMG_FILE);
	}
	// flip
	int width_in_bytes = x * 4;
	unsigned char *top = NULL;
	unsigned char *bottom = NULL;
	unsigned char temp = 0;
	int half_height = y / 2;

	for (int row = 0; row < half_height; row++) {
		top = data + row * width_in_bytes;
		bottom = data + (y - row - 1) * width_in_bytes;
		for (int col = 0; col < width_in_bytes; col++) {
			temp = *top;
			*top = *bottom;
			*bottom = temp;
			top++;
			bottom++;
		}
	}

	// copy into texture with internal format of choice
	// perhaps prepare some alternatives
	GLuint tex = 0, tex_cmpr5 = 0, tex_cmpr3 = 0, tex_cmpr1 = 0;
	{
		glGenTextures (1, &tex);
		glActiveTexture (GL_TEXTURE0);
		glBindTexture (GL_TEXTURE_2D, tex);
		GLint internalFormat = GL_RGBA;
		glTexImage2D (GL_TEXTURE_2D, 0, internalFormat, x, y, 0, GL_RGBA,
			GL_UNSIGNED_BYTE, data);
		glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	}
	if (compress) {
		glGenTextures (1, &tex_cmpr5);
		glActiveTexture (GL_TEXTURE0);
		glBindTexture (GL_TEXTURE_2D, tex_cmpr5);
		GLint internalFormat = GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
		glTexImage2D (GL_TEXTURE_2D, 0, internalFormat, x, y, 0, GL_RGBA,
			GL_UNSIGNED_BYTE, data);
		glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		
		glGenTextures (1, &tex_cmpr3);
		glActiveTexture (GL_TEXTURE0);
		glBindTexture (GL_TEXTURE_2D, tex_cmpr3);
		internalFormat = GL_COMPRESSED_RGBA_S3TC_DXT3_EXT;
		glTexImage2D (GL_TEXTURE_2D, 0, internalFormat, x, y, 0, GL_RGBA,
			GL_UNSIGNED_BYTE, data);
		glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		
		glGenTextures (1, &tex_cmpr1);
		glActiveTexture (GL_TEXTURE0);
		glBindTexture (GL_TEXTURE_2D, tex_cmpr1);
		internalFormat = GL_COMPRESSED_RGBA_S3TC_DXT1_EXT;
		glTexImage2D (GL_TEXTURE_2D, 0, internalFormat, x, y, 0, GL_RGBA,
			GL_UNSIGNED_BYTE, data);
		glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	}
	
	free (data);
	data = NULL;
	
	// geom
	// ----
	float quad[] = {-1.0f, -1.0f, -1.0f, 1.0f, 1.0f, -1.0f, 1.0f, 1.0f};
	GLuint vbo = 0;
	glGenBuffers (1, &vbo);
	glBindBuffer (GL_ARRAY_BUFFER, vbo);
	glBufferData (GL_ARRAY_BUFFER, 8 * sizeof (GLfloat), quad, GL_STATIC_DRAW);
	
	GLuint vao = 0;
	glGenVertexArrays (1, &vao);
	glBindVertexArray (vao);
	glBindBuffer (GL_ARRAY_BUFFER, vbo);
	glVertexAttribPointer (0, 2, GL_FLOAT, GL_FALSE, 0, NULL);
	glEnableVertexAttribArray (0);

	const char* vs_str =
	"#version 430\n"
	"in vec2 vp;"
	"out vec2 st;"
	"void main () {"
	"  st = vp * 0.5 + 0.5;"
	"  gl_Position = vec4 (vp, 0.0, 1.0);"
	"}";
	const char* fs_str =
	"#version 430\n"
	"in vec2 st;"
	"uniform sampler2D tex;"
	"out vec4 frag_colour;"
	"void main () {"
	"  frag_colour = texture (tex, st);"
	"}";
	GLuint vs = glCreateShader (GL_VERTEX_SHADER);
	glShaderSource (vs, 1, &vs_str, NULL);
	glCompileShader (vs);
	GLuint fs = glCreateShader (GL_FRAGMENT_SHADER);
	glShaderSource (fs, 1, &fs_str, NULL);
	glCompileShader (fs);
	GLuint sp = glCreateProgram ();
	glAttachShader (sp, fs);
	glAttachShader (sp, vs);
	glLinkProgram (sp);
	
	bool space_held_down = false;
	int tex_mode = 0;
	glBindTexture (GL_TEXTURE_2D, tex);
	printf ("RGBA\n");
	
	glEnable (GL_STENCIL_TEST);
	
	glStencilFunc (GL_ALWAYS, 1, 0xFF);
	// never, less, lequal, greater, gequal, equal, notequal, always
	//glStencilFuncSeparate (GL_FRONT_AND_BACK, GL_LESS);
	glStencilOp (
		GL_KEEP, // keep means don't modify the framebuffer if stencil test fails
		GL_KEEP, // don't modify buffer if stencil passes but depth test fails
		GL_REPLACE // if stencil and depth test both pass then write the new frag
	);
	glStencilMask (0xFF);
	
	while (!glfwWindowShouldClose (window)) {
		glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT |
			GL_STENCIL_BUFFER_BIT);
		glUseProgram (sp);
		glBindVertexArray (vao);
		glDrawArrays (GL_TRIANGLE_STRIP, 0, 4);
		glfwPollEvents ();
		if (!space_held_down && (GLFW_PRESS == glfwGetKey (window, GLFW_KEY_SPACE))
			) {
			switch (tex_mode) {
				case 0:
					tex_mode = 1;
					glBindTexture (GL_TEXTURE_2D, tex_cmpr1);
					printf ("DXT1\n");
					break;
				case 1:
					tex_mode = 3;
					glBindTexture (GL_TEXTURE_2D, tex_cmpr3);
					printf ("DXT3\n");
					break;
				case 3:
					tex_mode = 5;
					glBindTexture (GL_TEXTURE_2D, tex_cmpr5);
					printf ("DXT5\n");
					break;
				default:
					tex_mode = 0;
					glBindTexture (GL_TEXTURE_2D, tex);
					printf ("RGBA\n");
			}
			space_held_down = true;
		}
		if (GLFW_RELEASE == glfwGetKey (window, GLFW_KEY_SPACE)) {
			space_held_down = false;
		}
		if (GLFW_PRESS == glfwGetKey (window, GLFW_KEY_ESCAPE)) {
			glfwSetWindowShouldClose (window, 1);
		}
		glfwSwapBuffers (window);
	}
	
	printf ("shutdown\n");
	glfwTerminate();
	return 0;
}
