//
// custom gl stuff
// anton gerdelan <gerdela@scss.tcd.ie>
// 20 jan 2016
// trinity college dublin
//

#include <stdio.h>
#include <string.h>
#include "apg_gl.h"

float apg_cube_points[24] = {
	-1.0f, -1.0f, -1.0f,
	-1.0f, -1.0f, 1.0f,
	-1.0f, 1.0f, -1.0f,
	-1.0f, 1.0f, 1.0f,
	1.0f, -1.0f, -1.0f,
	1.0f, -1.0f, 1.0f,
	1.0f, 1.0f, -1.0f,
	1.0f, 1.0f, 1.0f
};

unsigned int apg_cube_ccw_tri_indices[36] = {
	7, 3, 1, // front
	1, 5, 7,
	6, 7, 5, // left
	5, 4, 6,
	2, 6, 4, // back
	4, 0, 2,
	3, 2, 0, // right
	0, 1, 3,
	6, 2, 3, // top
	3, 7, 6,
	5, 1, 0, // bottom
	0, 4, 5
};

APG_GL_Context g_gl;
GLuint ss_quad_tristrip_vao;
APG_Shader g_ss_quad_shader;

static void win_resize_cb (GLFWwindow* win, int width, int height) {
	g_gl.win_width = width;
	g_gl.win_height = height;
	printf ("window resized to %ix%i\n", width, height);
}

static void fb_resize_cb (GLFWwindow* win, int width, int height) {
	g_gl.fb_width = width;
	g_gl.fb_height = height;
	printf ("framebuffer resized to %ix%ipx\n", g_gl.fb_width, g_gl.fb_height);
	glViewport (0, 0, g_gl.fb_width, g_gl.fb_height);
}

bool start_gl () {
	{ // glfw
		if (!glfwInit ()) {
			fprintf (stderr, "ERROR: starting GLFW\n");
			return false;
		}
		g_gl.win_width = 1024;
		g_gl.win_height = 768;
		glfwWindowHint (GLFW_CONTEXT_VERSION_MAJOR, 4);
		glfwWindowHint (GLFW_CONTEXT_VERSION_MINOR, 3);
		glfwWindowHint (GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
		glfwWindowHint (GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
		g_gl.win = glfwCreateWindow (g_gl.win_width, g_gl.win_height, "demo", NULL,
			NULL);
		if (!g_gl.win) {
			fprintf (stderr, "ERROR: creating window\n");
			return false;
		}
		glfwMakeContextCurrent (g_gl.win);
		glfwSetWindowSizeCallback (g_gl.win, win_resize_cb);
		glfwSetFramebufferSizeCallback (g_gl.win, fb_resize_cb);
	}
	{ // glew
		glewExperimental = GL_TRUE;
		GLenum err = glewInit ();
		if (GLEW_OK != err) {
			fprintf (stderr, "ERROR: starting GLEW %s\n", glewGetErrorString (err));
			return false;
		}
		printf ("GLEW %s\n", glewGetString (GLEW_VERSION));
	}
	{ // report
		const GLubyte* renderer = glGetString (GL_RENDERER);
		const GLubyte* version = glGetString (GL_VERSION);
		printf ("%s\n", renderer);
		printf ("OpenGL %s\n", version);
	}
	{
		glGenVertexArrays (1, &ss_quad_tristrip_vao);
		glBindVertexArray (ss_quad_tristrip_vao);
		GLuint points_vbo;
		glGenBuffers (1, &points_vbo);
		glBindBuffer (GL_ARRAY_BUFFER, points_vbo);
		float points[] = { -1.0f, -1.0f, -1.0f, 1.0f, 1.0f, -1.0f, 1.0f, 1.0f };
		glBufferData (GL_ARRAY_BUFFER, 3 * 4 * sizeof (GLfloat), points,
			GL_STATIC_DRAW);
		glVertexAttribPointer (0, 2, GL_FLOAT, GL_FALSE, 0, NULL);
		glEnableVertexAttribArray (0);
	}
	{
		const char* vs_str =
			"#version 430\n"
			"in vec2 vp;"
			"uniform vec2 sca, pos;"
			"out vec2 op;"
			"void main () {"
			"  vec2 p = vp * sca + pos;"
			"  gl_Position = vec4 (p, 0.0, 1.0);"
			"  op = vp;"
			"}";
		const char* fs_str =
			"#version 430\n"
			"in vec2 op;"
			"uniform samplerCube tex;"
			"out vec4 fc;"
			"void main () {"
			"  fc = texture (tex, vec3 (op, -1.0));"
			//			"  fc = vec4 (st, 0.0, 1.0);"
			"}";
		g_ss_quad_shader.vs = glCreateShader (GL_VERTEX_SHADER);
		glShaderSource (g_ss_quad_shader.vs, 1, &vs_str, NULL);
		glCompileShader (g_ss_quad_shader.vs);
		g_ss_quad_shader.fs = glCreateShader (GL_FRAGMENT_SHADER);
		glShaderSource (g_ss_quad_shader.fs, 1, &fs_str, NULL);
		glCompileShader (g_ss_quad_shader.fs);
		g_ss_quad_shader.sp = glCreateProgram ();
		glAttachShader (g_ss_quad_shader.sp, g_ss_quad_shader.fs);
		glAttachShader (g_ss_quad_shader.sp, g_ss_quad_shader.vs);
		glLinkProgram (g_ss_quad_shader.sp);
		g_ss_quad_shader.tex_loc = glGetUniformLocation (g_ss_quad_shader.sp,
			"tex");
		g_ss_quad_shader.sca_loc = glGetUniformLocation (g_ss_quad_shader.sp,
			"sca");
		g_ss_quad_shader.pos_loc = glGetUniformLocation (g_ss_quad_shader.sp,
			"pos");
	}
	{ // defaults
		glEnable (GL_DEPTH_TEST);
		glDepthFunc (GL_LESS);
		glEnable (GL_CULL_FACE);
		glCullFace (GL_BACK);
		glFrontFace (GL_CCW);
		glDisable (GL_BLEND);
		glClearColor (0.5f, 0.5f, 0.5f, 1.0f);
		glfwGetWindowSize (g_gl.win, &g_gl.win_width, &g_gl.win_height);
		glfwGetFramebufferSize (g_gl.win, &g_gl.fb_width, &g_gl.fb_height);
		printf ("window %ix%ipx\n", g_gl.win_width, g_gl.win_height);
		printf ("framebuffer %ix%ipx\n", g_gl.fb_width, g_gl.fb_height);
		glViewport (0, 0, g_gl.fb_width, g_gl.fb_height);
	}
	return true;
}

void stop_gl () {
	glfwTerminate ();
}

