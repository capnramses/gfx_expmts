//
// terrain demo
// first version: anton gerdelan <gerdela@scss.tcd.ie> 13 december 2015
// trinity college dublin, ireland
//
// TODO
// choose terrain res independently of image res
// interpolation with NN
// or bi-linear
//
// TODO -- normals

#include "apg_maths.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <stdio.h>
#include <assert.h>

typedef unsigned char byte;
typedef unsigned int uint;

GLFWwindow* window;
GLuint terr_vao;
GLuint terr_pc;
GLuint terr_sp;
GLint terr_sp_P_loc = -1, terr_sp_V_loc = -1;
mat4 V, P;

int main () {
	printf ("terrain demo\n");
	{ // start glfw
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
		glfwWindowHint (GLFW_CONTEXT_VERSION_MINOR, 5);
		glfwWindowHint (GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
		glfwWindowHint (GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#endif
		window = glfwCreateWindow (1024, 767, "terrain demo", NULL, NULL);
		if (!window) {
			fprintf (stderr, "ERROR: could not open window with GLFW3\n");
			glfwTerminate ();
			return 1;
		}
		glfwMakeContextCurrent (window);
	}
	{ // start glew
		glewExperimental = GL_TRUE;
		glewInit ();
		const GLubyte* renderer = glGetString (GL_RENDERER);
		const GLubyte* version = glGetString (GL_VERSION);
		printf ("Renderer: %s\n", renderer);
		printf ("OpenGL version supported %s\n", version);
	}
	{ // load hm image and ~terrain colour image
		int x = 0, y = 0, n = 0;
		byte* img = stbi_load ("hm2.png", &x, &y, &n, 0);
		if (!img) {
			fprintf (stderr, "ERROR: could not hm2.png\n");
			glfwTerminate ();
			return 1;
		}
		printf ("hm2.png loaded %ix%i %ichans\n", x, y, n);
		//size_t sz = sizeof (float) * x * y * 3;
		// 2 triangles per point except last row and col
		size_t sz = sizeof (float) * (x - 1) * (y - 1) * 3 * 3 * 2;
		//sz /= 4; // every 4 points
		//sz *= 6; // makes 2 triangles
		float* terrain_vp_buff = (float*)malloc (sz);
		if (!terrain_vp_buff) {
			fprintf (stderr, "ERROR: OOM allocating for terrain\n");
			glfwTerminate ();
			return 1;
		}
		printf ("terrain memory allocated %lu bytes\n", sz);
		uint tbc = 0;
		for (uint y_i = 0; y_i < y - 1; y_i++) {
			for (uint x_i = 0; x_i < x - 1; x_i++) {
				{
					byte val = img[y_i * x + x_i];
					float height = (float)val / 255; // 0.0 black to 1.0 white
					height = height * 2.0f - 1.0f; // -1.0 to 1.0
					float across =  (float)x_i / (float)x; // 0.0 to 1.0
					across = across * 2.0f - 1.0f; // -1.0 to 1.0
					float down =  (float)y_i / (float)y; // 0.0 to 1.0
					down = down * 2.0f - 1.0f; // -1.0 to 1.0
					terrain_vp_buff[tbc] = across;
					terrain_vp_buff[tbc + 1] = height;
					terrain_vp_buff[tbc + 2] = down;
					tbc += 3;
				}
				{
					byte val = img[(y_i + 1) * x + x_i];
					float height = (float)val / 255; // 0.0 black to 1.0 white
					height = height * 2.0f - 1.0f; // -1.0 to 1.0
					float across =  (float)x_i / (float)x; // 0.0 to 1.0
					across = across * 2.0f - 1.0f; // -1.0 to 1.0
					float down =  (float)(y_i + 1) / (float)y; // 0.0 to 1.0
					down = down * 2.0f - 1.0f; // -1.0 to 1.0
					terrain_vp_buff[tbc] = across;
					terrain_vp_buff[tbc + 1] = height;
					terrain_vp_buff[tbc + 2] = down;
					tbc += 3;
				}
				{
					byte val = img[(y_i + 1) * x + (x_i + 1)];
					float height = (float)val / 255; // 0.0 black to 1.0 white
					height = height * 2.0f - 1.0f; // -1.0 to 1.0
					float across =  (float)(x_i + 1) / (float)x; // 0.0 to 1.0
					across = across * 2.0f - 1.0f; // -1.0 to 1.0
					float down =  (float)(y_i + 1) / (float)y; // 0.0 to 1.0
					down = down * 2.0f - 1.0f; // -1.0 to 1.0
					terrain_vp_buff[tbc] = across;
					terrain_vp_buff[tbc + 1] = height;
					terrain_vp_buff[tbc + 2] = down;
					tbc += 3;
				}
				// 2nd tri
				{
					byte val = img[(y_i + 1) * x + (x_i + 1)];
					float height = (float)val / 255; // 0.0 black to 1.0 white
					height = height * 2.0f - 1.0f; // -1.0 to 1.0
					float across =  (float)(x_i + 1) / (float)x; // 0.0 to 1.0
					across = across * 2.0f - 1.0f; // -1.0 to 1.0
					float down =  (float)(y_i + 1) / (float)y; // 0.0 to 1.0
					down = down * 2.0f - 1.0f; // -1.0 to 1.0
					terrain_vp_buff[tbc] = across;
					terrain_vp_buff[tbc + 1] = height;
					terrain_vp_buff[tbc + 2] = down;
					tbc += 3;
				}
				{
					byte val = img[y_i * x + (x_i + 1)];
					float height = (float)val / 255; // 0.0 black to 1.0 white
					height = height * 2.0f - 1.0f; // -1.0 to 1.0
					float across =  (float)(x_i + 1) / (float)x; // 0.0 to 1.0
					across = across * 2.0f - 1.0f; // -1.0 to 1.0
					float down =  (float)y_i / (float)y; // 0.0 to 1.0
					down = down * 2.0f - 1.0f; // -1.0 to 1.0
					terrain_vp_buff[tbc] = across;
					terrain_vp_buff[tbc + 1] = height;
					terrain_vp_buff[tbc + 2] = down;
					tbc += 3;
				}
				{
					byte val = img[y_i * x + x_i];
					float height = (float)val / 255; // 0.0 black to 1.0 white
					height = height * 2.0f - 1.0f; // -1.0 to 1.0
					float across =  (float)x_i / (float)x; // 0.0 to 1.0
					across = across * 2.0f - 1.0f; // -1.0 to 1.0
					float down =  (float)y_i / (float)y; // 0.0 to 1.0
					down = down * 2.0f - 1.0f; // -1.0 to 1.0
					terrain_vp_buff[tbc] = across;
					terrain_vp_buff[tbc + 1] = height;
					terrain_vp_buff[tbc + 2] = down;
					tbc += 3;
				}
			}
		}
		terr_pc = tbc;
		glGenVertexArrays (1, &terr_vao);
		glBindVertexArray (terr_vao);
		uint vbo = 0;
		glGenBuffers (1, &vbo);
		glBindBuffer (GL_ARRAY_BUFFER, vbo);
		glBufferData (GL_ARRAY_BUFFER, tbc * sizeof (float), terrain_vp_buff,
			GL_STATIC_DRAW);
		glEnableVertexAttribArray (0);
		glVertexAttribPointer (0, 3, GL_FLOAT, GL_FALSE, 0, NULL);
		free (terrain_vp_buff);
		free (img);
		printf ("terrain generated - %u points\n", terr_pc);
	}
	{ // camera
		V = look_at (
			vec3_from_3f (0.0f, 1.5, 1.5f),
			vec3_from_3f (0.0f, 0.0f, 0.0f),
			normalise_vec3 (vec3_from_3f (0.0f, 1.0f, -1.0f))
		);
		float a = (float)1024.0f / (float)768.0f;
		P = perspective (67.0f, a, 0.1f, 4.0f);
	}
	{ // load shaders
		const char* vs_str = "#version 450\n"
			"in vec3 vp;\n"
			"uniform mat4 P, V;\n"
			"out float h;\n"
			"void main () {\n"
			"  gl_Position = P * V * vec4 (vp, 1.0);\n"
			"  h = vp.y;\n"
			"}\n";
		const char* fs_str = "#version 450\n"
			"in float h;\n"
			"out vec4 fc;\n"
			"void main () {\n"
			"  vec3 snow = vec3 (0.9, 0.9, 0.9);\n"
			"  vec3 dirt = vec3 (0.1, 0.6, 0.05);\n"
			"  vec3 water = vec3 (0.05, 0.2, 0.6);\n"
			"  vec3 col = vec3 (0.0, 0.0, 0.0);\n"
			"  if (h > 0.0) {\n"
			"    col = mix (dirt, snow, h);\n"
			"  } else {\n"
			"    col = mix (dirt, water, -h);\n"
			"  }\n"
			"  fc = vec4 (col, 1.0);\n"
			"}\n";
		GLuint vs = glCreateShader (GL_VERTEX_SHADER);
		glShaderSource (vs, 1, &vs_str, NULL);
		glCompileShader (vs);
		GLuint fs = glCreateShader (GL_FRAGMENT_SHADER);
		glShaderSource (fs, 1, &fs_str, NULL);
		glCompileShader (fs);
		terr_sp = glCreateProgram ();
		glAttachShader (terr_sp, vs);
		glAttachShader (terr_sp, fs);
		glLinkProgram (terr_sp);
		terr_sp_P_loc = glGetUniformLocation (terr_sp, "P");
		terr_sp_V_loc = glGetUniformLocation (terr_sp, "V");
	}
	{ // initial GL states
		glClearColor (0.2, 0.2, 0.2, 1.0);
		glDisable (GL_BLEND);
		glDepthFunc (GL_LESS);
		glEnable (GL_DEPTH_TEST);
		glEnable (GL_CULL_FACE);
		glCullFace (GL_BACK);
		glFrontFace (GL_CCW);
	}
	double prev_s = glfwGetTime ();
	while (!glfwWindowShouldClose (window)) {
		double curr_s = glfwGetTime ();
		double elapsed_s = curr_s - prev_s;
		prev_s = curr_s;
		glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		{
			glUseProgram (terr_sp);
			glUniformMatrix4fv (terr_sp_P_loc, 1, GL_FALSE, P.m);
			glUniformMatrix4fv (terr_sp_V_loc, 1, GL_FALSE, V.m);
			glBindVertexArray (terr_vao);
			glDrawArrays (GL_TRIANGLES, 0, terr_pc);
		}
		glfwPollEvents ();
		glfwSwapBuffers (window);
	}
	glfwTerminate ();
	return 0;
}
