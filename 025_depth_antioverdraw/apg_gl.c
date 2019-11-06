//
// custom gl stuff
// anton gerdelan <gerdela@scss.tcd.ie>
// 20 jan 2016
// trinity college dublin
//

#include <stdio.h>
#include "apg_gl.h"

APG_GL_Context g_gl;

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
	{ // defaults
		glEnable (GL_DEPTH_TEST);
		glDepthFunc (GL_LESS);
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

