//
// rendering everything to depth buffer only in a first pass to reduce overdraw
// of fragment shaders. idea from Doom 3 source code
// anton gerdelan 20 jan 2016 <gerdela@scss.tcd.ie>
// trinity college dublin, ireland
//
// press F2 to toggle mode
// notes - i get a speed-up only with a lower number of meshes e.g. 99 draws
// with higher numbers e.g. 999 or 9999 the cpu side stuff seems to nail
// the frame rate
//

#include "apg_maths.h"
#include "obj_parser.h"
#include "apg_gl.h"
#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#define MESH_FILE "../common/mesh/suzanne.obj"
#define NUM_MONKEYS 99
float monkey_zs[NUM_MONKEYS];
bool do_pre_pass = true;
APG_Mesh mesh;
GLuint shader_programme, dshader_programme;
GLint sp_PVM_loc = -1, dsp_PVM_loc = -1;
mat4 P, V, PV;

static void init () {
	assert (start_gl ());
	// create FBs
	// create textures
	{ // load meshes
		float *vps = NULL, *vts = NULL, *vns = NULL;
		int pc = 0;
		assert (load_obj_file (MESH_FILE, &vps, &vts, &vns, &pc));
		mesh.pc = (GLuint)pc;
		glGenVertexArrays (1, &mesh.vao);
		glBindVertexArray (mesh.vao);
		glGenBuffers (1, &mesh.vbo_vps);
		glBindBuffer (GL_ARRAY_BUFFER, mesh.vbo_vps);
		glBufferData (GL_ARRAY_BUFFER, mesh.pc * 3 * sizeof (float), vps,
			GL_STATIC_DRAW);
		glEnableVertexAttribArray (0);
		glVertexAttribPointer (0, 3, GL_FLOAT, GL_FALSE, 0, NULL);
		glGenBuffers (1, &mesh.vbo_vts);
		glBindBuffer (GL_ARRAY_BUFFER, mesh.vbo_vts);
		glBufferData (GL_ARRAY_BUFFER, mesh.pc * 3 * sizeof (float), vts,
			GL_STATIC_DRAW);
		glEnableVertexAttribArray (1);
		glVertexAttribPointer (1, 2, GL_FLOAT, GL_FALSE, 0, NULL);
		glGenBuffers (1, &mesh.vbo_vns);
		glBindBuffer (GL_ARRAY_BUFFER, mesh.vbo_vns);
		glBufferData (GL_ARRAY_BUFFER, mesh.pc * 3 * sizeof (float), vns,
			GL_STATIC_DRAW);
		glEnableVertexAttribArray (2);
		glVertexAttribPointer (2, 3, GL_FLOAT, GL_FALSE, 0, NULL);
		free (vps);
		free (vts);
		free (vns);
	}
	{ // load shaders
		const char* dvertex_shader =
			"#version 430\n"
			"in vec3 vp;"
			"uniform mat4 PVM;"
			"void main () {"
			"  gl_Position = PVM * vec4 (vp, 1.0);"
			"}";
		const char* dfragment_shader =
			"#version 430\n"
			"void main () {}";
		GLuint dvs = glCreateShader (GL_VERTEX_SHADER);
		glShaderSource (dvs, 1, &dvertex_shader, NULL);
		glCompileShader (dvs);
		GLuint dfs = glCreateShader (GL_FRAGMENT_SHADER);
		glShaderSource (dfs, 1, &dfragment_shader, NULL);
		glCompileShader (dfs);
		dshader_programme = glCreateProgram ();
		glAttachShader (dshader_programme, dfs);
		glAttachShader (dshader_programme, dvs);
		glLinkProgram (dshader_programme);
		dsp_PVM_loc = glGetUniformLocation (dshader_programme, "PVM");
		const char* vertex_shader =
			"#version 430\n"
			"in vec3 vp;"
			"uniform mat4 PVM;"
			"void main () {"
			"  gl_Position = PVM * vec4 (vp, 1.0);"
			"}";
		// aritificial crap put in shader to make it slower
		const char* fragment_shader =
			"#version 430\n"
			"out vec4 frag_colour;"
			"void main () {"
			"  vec3 sum = vec3 (0.0, 0.0, 0.0);"
			"  for (int i = 0; i < 32; i++) {"
			"    sum.r += float (i);"
			"    sum.g += float (i * 2);"
			"    sum.b += float (i + 1) * 2.0;"
			"    sum = normalize (sum);"
			"  }"
			"  frag_colour = vec4 (sum, 1.0);"
			"}";
		GLuint vs = glCreateShader (GL_VERTEX_SHADER);
		glShaderSource (vs, 1, &vertex_shader, NULL);
		glCompileShader (vs);
		GLuint fs = glCreateShader (GL_FRAGMENT_SHADER);
		glShaderSource (fs, 1, &fragment_shader, NULL);
		glCompileShader (fs);
		shader_programme = glCreateProgram ();
		glAttachShader (shader_programme, fs);
		glAttachShader (shader_programme, vs);
		glLinkProgram (shader_programme);
		sp_PVM_loc = glGetUniformLocation (shader_programme, "PVM");
	}
	{ // camera
		float a = (float)g_gl.fb_width / (float)g_gl.fb_height;
		P = perspective (67.0f, a, 0.1f, 1000.0f);
		V = look_at (vec3_from_3f (0.0f, 0.0f, 1.0f),
			vec3_from_3f (0.0f, 0.0f, 0.0f),
			vec3_from_3f (0.0f, 1.0f, 0.0f));
		PV = mult_mat4_mat4 (P, V);
	}
	// monkey positions (distribute the distances but not in order)
	for (int i = 0; i < NUM_MONKEYS; i++) {
		monkey_zs[i] = sinf ((float)i) * 100.0f - 100.0f;
	}
}

static void stop () {
	stop_gl ();
}

static void draw_frame (double elapsed) {
	glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	if (do_pre_pass) { // depth-writing pre-pass
		glUseProgram (dshader_programme);
		for (int i = 0; i < NUM_MONKEYS; i++) {
			mat4 M = translate_mat4 (vec3_from_3f (0.0f, 0.0f, monkey_zs[i]));
			mat4 PVM = mult_mat4_mat4 (PV, M);
			glUniformMatrix4fv (dsp_PVM_loc, 1, GL_FALSE, PVM.m);
			glBindVertexArray (mesh.vao);
			glDrawArrays (GL_TRIANGLES, 0, mesh.pc);
		}
		glDepthFunc (GL_LEQUAL); // because self is gonna be equal duh!
		glDepthMask (GL_FALSE); // disable depth writing - already done
	}
	{ // normal render pass with no depth rendering
		glUseProgram (shader_programme);
		for (int i = 0; i < NUM_MONKEYS; i++) {
			mat4 M = translate_mat4 (vec3_from_3f (0.0f, 0.0f, monkey_zs[i]));
			mat4 PVM = mult_mat4_mat4 (PV, M);
			glUniformMatrix4fv (sp_PVM_loc, 1, GL_FALSE, PVM.m);
			glBindVertexArray (mesh.vao);
			glDrawArrays (GL_TRIANGLES, 0, mesh.pc);
		}
		glDepthFunc (GL_LESS);
		glDepthMask (GL_TRUE); // disable depth writing - already done
	}
	glfwSwapBuffers (g_gl.win);
}

int main () {
	init ();
	{ // running
		int f_accum = 0;
		double s_accum = 0.0;
		double prev = glfwGetTime ();
		while (!glfwWindowShouldClose (g_gl.win)) {
			// timers and counters
			double curr = glfwGetTime ();
			double elapsed = curr - prev;
			prev = curr;
			s_accum += elapsed;
			if (s_accum > 0.5 && f_accum > 0) {
				double ms_per_frame = (s_accum / (double)f_accum) * 1000.0;
				if (ms_per_frame > 0.0) {
					double fps = 1000.0 / ms_per_frame;
					char tmp[256];
					if (do_pre_pass) {
						sprintf (tmp, "pre-pass=ON %.2lfms %.0ffps\n", ms_per_frame, fps);
					} else {
						sprintf (tmp, "pre-pass=OFF %.2lfms %.0ffps\n", ms_per_frame, fps);
					}
					glfwSetWindowTitle (g_gl.win, tmp);
				}
				s_accum = 0.0;
				f_accum = 0;
			}
			// TODO gpu time query
			{ // user input
				glfwPollEvents ();
				if (glfwGetKey (g_gl.win, GLFW_KEY_ESCAPE)) {
					break;
				}
				// TODO toggle pre-pass
				static bool was_down = false;
				bool is_down = false;
				if (glfwGetKey (g_gl.win, GLFW_KEY_F2)) {
					if (!was_down) {
						is_down = true;
						was_down = true;
					}
				} else {
					was_down = false;
				}
				if (is_down) {
					do_pre_pass = !do_pre_pass;
					printf ("pre-pass = %i\n", (int)do_pre_pass);
				}
			}
			draw_frame (elapsed);
			f_accum ++;
		} // endwhile
	} // endblock
	stop ();
	return 0;
}

