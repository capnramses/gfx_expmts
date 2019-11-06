// == About This Program ==
// Reads a panoramic photo e.g. taken on a smartphone from Google Street app
// and writes out 6 PNGs making faces of a cubemap
//
// == Author / Licence / History == 
// Copyright Dr Anton Gerdelan <antonofnote@gmail.com>
// first version: 28 June 2016. 15 April 2018: additional Makefile for Win64.
// This file and all original code in this repository carries GNU "Stick It To The Man" GPL v3 Licence.
//
// == To Build ==
// Compile main.c and obj_parser.c and link against glfw3, glew, and opengl for your compiler
// Makefiles for Windows mingw-gcc 64-bit and gcc 64-bit Linux are provided
// so eg `mingw32-make -f Makefile.win64` or `make -f Makefile.lin64`
// OSX - should be similar to Linux except `-framework OpenGL` instead of a library
// Visual Studio IDE will never be supported - it will work but you're on your own.
//
// == To Run ==
// `./pano2cube my_source_panorama.jpg`
// expects a spherical panorama image as per Google Street app (email one to yourself from your phone)
//

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"
#include "linmath.h"
#include "obj_parser.h"
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <stdio.h>
#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#define VP_WIDTH 1024
#define VP_HEIGHT 1024

void init_gl();
void free_gl();
void init_shaders();
void compile_shader(GLuint shdr_idx);
void link_program(GLuint prog_idx);
bool screencapture(const char* fn);

GLFWwindow* g_win;
GLint P_loc, V_loc;
GLuint g_shadprog;
GLuint g_tex_pano;
int g_fb_width = VP_WIDTH; // hack
int g_fb_height = VP_HEIGHT;

int main(int argc, char** argv){
	int pcount = 0;
	GLuint g_vao_tri;
	if(argc < 2){
		printf("usage: ./pano2cube MYPANO.JPG\n");
		return 0;
	}
	init_gl();
	{
		printf ("loading panorama `%s`\n", argv[1]);
		int x = 0, y = 0, n = 0;
		unsigned char* image_data = stbi_load(argv[1], &x, &y, &n, 4);
		assert(image_data);
		{
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
		}
		glActiveTexture(GL_TEXTURE0);
  	glGenTextures(1, &g_tex_pano);
  	glBindTexture (GL_TEXTURE_2D, g_tex_pano);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, x, y, 0, GL_RGBA, GL_UNSIGNED_BYTE, image_data);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		GLfloat max_aniso = 0.0f;
		glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &max_aniso);
		// set the maximum!
		glTexParameterf (GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, max_aniso);
		stbi_image_free(image_data);
		printf("loaded %ix%i, %i chans\n", x, y, n);
	}
	init_shaders();
	{ // geometry
		float *vps, *vts, *vns;
		load_obj_file("sphere.obj", &vps, &vts, &vns, &pcount);
		assert (vps);
		assert (vts);
		assert (pcount > 0);
		glGenVertexArrays(1, &g_vao_tri);
		glBindVertexArray (g_vao_tri);
		GLuint vbos[2];
		glGenBuffers(2, vbos);
		glBindBuffer(GL_ARRAY_BUFFER, vbos[0]);
		glBufferData(GL_ARRAY_BUFFER, pcount * 3 * sizeof (float), vps, GL_STATIC_DRAW);
		free(vps);
		vps = NULL;
		glEnableVertexAttribArray (0);
		glVertexAttribPointer (0, 3, GL_FLOAT, GL_FALSE, 0, NULL);
		glBindBuffer(GL_ARRAY_BUFFER, vbos[1]);
		glBufferData(GL_ARRAY_BUFFER, pcount * 2 * sizeof (float), vts, GL_STATIC_DRAW);
		free(vts);
		vts = NULL;
		glEnableVertexAttribArray (1);
		glVertexAttribPointer (1, 2, GL_FLOAT, GL_FALSE, 0, NULL);
	}
	const char* op_fns[6] = {
		"forward.png",
		"right.png",
		"back.png",
		"left.png",
		"up.png",
		"down.png"
	};
	mat4 P = perspective(90.0f, 1.0f, 0.01f, 100.0f);
	mat4 Vs[6];
	{ // put camera on origin, _inside_ sphere, facing 6 directions with 90 view each.
		vec3 cam_pos = (vec3){0.0f, 0.0f, 0.0f};
		Vs[0] = look_at (cam_pos, (vec3){0.0f, 0.0f, -1.0f}, (vec3){0.0f, 1.0f, 0.0f}); // fwd
		Vs[1] = look_at (cam_pos, (vec3){1.0f, 0.0f, 0.0f}, (vec3){0.0f, 1.0f, 0.0f}); // rgt
		Vs[2] = look_at (cam_pos, (vec3){0.0f, 0.0f, 1.0f}, (vec3){0.0f, 1.0f, 0.0f}); // bk
		Vs[3] = look_at (cam_pos, (vec3){-1.0f, 0.0f, 0.0f}, (vec3){0.0f, 1.0f, 0.0f}); // lft
		Vs[4] = look_at (cam_pos, (vec3){0.0f, 1.0f, 0.0f}, (vec3){0.0f, 0.0f, 1.0f}); // up
		Vs[5] = look_at (cam_pos, (vec3){0.0f, -1.0f, 0.0f}, (vec3){0.0f, 0.0f, -1.0f}); // dn
		glUseProgram(g_shadprog);
		glUniformMatrix4fv(P_loc, 1, GL_FALSE, P.m);
	}
	glBindVertexArray(g_vao_tri);
	glActiveTexture (GL_TEXTURE0);
	glBindTexture (GL_TEXTURE_2D, g_tex_pano);
	for(int i = 0; i < 6; i++) {
		glViewport(0,0,VP_WIDTH,VP_HEIGHT);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		
		glUniformMatrix4fv(V_loc, 1, GL_FALSE, Vs[i].m);
		glDrawArrays (GL_TRIANGLES, 0, pcount);
		
		screencapture(op_fns[i]);
		
		glfwSwapBuffers(g_win);
		glfwPollEvents();
		
		printf("writing image `%s`\n", op_fns[i]);
	}
	printf("done\n");
	free_gl();
	return 0;
}

void init_gl(){
	{
		glfwInit();
		glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
		glfwWindowHint(GLFW_SRGB_CAPABLE, GLFW_TRUE);
		glfwWindowHint (GLFW_SAMPLES, 16);
		{ // NOTE OPENGL ES 3 INVOKED HERE
			//glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
			//glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
			//glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
		}
		g_win = glfwCreateWindow(VP_WIDTH, VP_HEIGHT, "pano2cube - Anton Gerdelan", NULL, NULL);
		glfwMakeContextCurrent(g_win);
		glewExperimental = GL_TRUE;
		glewInit();
	}
	{
		const GLubyte* renderer = glGetString (GL_RENDERER); // get renderer string
		const GLubyte* version = glGetString (GL_VERSION); // version as a string
		printf ("Renderer: %s\n", renderer);
		printf ("OpenGL version supported %s\n", version);
	}
	{
		glEnable(GL_DEPTH_TEST);
		glDepthFunc(GL_LESS);
		//glEnable (GL_CULL_FACE);
		glCullFace (GL_BACK);
		glFrontFace (GL_CCW);
		glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
	}
}

void free_gl(){ glfwTerminate(); }

void compile_shader(GLuint shdr_idx){
	printf("compiling shader\n");
	glCompileShader (shdr_idx);
	int params = -1;
	glGetShaderiv (shdr_idx, GL_COMPILE_STATUS, &params);
	if (GL_TRUE != params) {
		fprintf (stderr, "ERROR: GL shader index %i did not compile\n", shdr_idx);
		int max_length = 2048;
		int actual_length = 0;
		char log[2048];
		glGetShaderInfoLog (shdr_idx, max_length, &actual_length, log);
		printf ("shader info log for GL index %u:\n%s\n", shdr_idx, log);
	}
}

void link_program(GLuint prog_idx){
	printf("linking program\n");
	glLinkProgram (prog_idx);
	int params = -1;
	glGetProgramiv (prog_idx, GL_LINK_STATUS, &params);
	if (GL_TRUE != params) {
		fprintf (stderr, "ERROR: could not link shader programme GL index %u\n", prog_idx);
		int max_length = 2048;
		int actual_length = 0;
		char log[2048];
		glGetProgramInfoLog (prog_idx, max_length, &actual_length, log);
		printf ("program info log for GL index %u:\n%s", prog_idx, log);
	}
}

void init_shaders() {
	const char* vertex_shader =
		"#version 100\n"
		"attribute vec3 vp;"
		"attribute vec2 vt;"
		"uniform mat4 P, V;"
		"varying vec2 st;"
		"void main(){"
		"  st = vt;"
		"  gl_Position = P * V * vec4(vp, 1.0);"
		"}";
	const char* fragment_shader =
		"#version 100\n"
		"precision mediump float;" // need this
		"varying vec2 st;"
		"uniform sampler2D pano_texture;"
		"void main(){"
		"  gl_FragColor = texture2D (pano_texture, st);"
		//"  gl_FragColor = vec4 (st, 0.0, 1.0);"
		"}";
	GLuint vs = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vs, 1, &vertex_shader, NULL);
	compile_shader(vs);
	GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource (fs, 1, &fragment_shader, NULL);
	compile_shader(fs);
	g_shadprog = glCreateProgram();
	glAttachShader(g_shadprog, fs);
	glAttachShader(g_shadprog, vs);
	glBindAttribLocation(g_shadprog, 0, "vp");
	glBindAttribLocation(g_shadprog, 1, "vt");
	link_program(g_shadprog);
	P_loc = glGetUniformLocation(g_shadprog, "P");
	V_loc = glGetUniformLocation(g_shadprog, "V");
}

bool screencapture(const char* fn) {
	unsigned char* buffer = (unsigned char*)malloc (g_fb_width * g_fb_height * 3);
	glFinish();
	glFlush();
	glReadPixels (0, 0, g_fb_width, g_fb_height, GL_RGB, GL_UNSIGNED_BYTE, buffer);
	unsigned char* last_row = buffer + (g_fb_width * 3 * (g_fb_height - 1));
	if (!stbi_write_png (fn, g_fb_width, g_fb_height, 3, last_row, -3 * g_fb_width)) {
		fprintf (stderr, "ERROR: could not write screenshot file `%s`\n", fn);
	}
	free (buffer);
	return true;
}
