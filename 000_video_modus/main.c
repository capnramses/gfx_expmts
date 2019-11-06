//
// video modes test
// first version: anton gerdelan <gerdela@scss.tcd.ie> 18 august 2015
// trinity college dublin, ireland
//

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <stdio.h>

int main () {
	printf ("starting glfw...\n");
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

	GLFWwindow* window = glfwCreateWindow (640, 480, "vid modes", NULL, NULL);
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

	printf ("finding monitors...\n");
	GLFWmonitor* primary = glfwGetPrimaryMonitor ();
	{
		int count = 0;
		GLFWmonitor** monitors = glfwGetMonitors (&count);
		printf ("found %i monitors\n\n", count);
		printf ("| # |primary|     name     |       vid mode       |\n");
		printf ("|---|-------|--------------|----------------------|\n");
		for (int i = 0; i < count; i++) {
			char prim = 'n';
			if (monitors[i] == primary) {
				prim = 'Y';
			}
			const char* name = glfwGetMonitorName (monitors[i]);
			const GLFWvidmode* curr_mode = glfwGetVideoMode (monitors[i]);
			int w = curr_mode->width;
			int h = curr_mode->height;
			int hz = curr_mode->refreshRate;
			int bpp = curr_mode->blueBits + curr_mode->greenBits + curr_mode->redBits;
			//const GLFWgammaramp* glfwGetGammaRamp (monitors[i]);
		
			printf ("|%2i |   %c   | %12s | %4ix%4i %ibpp %iHz |\n", i, prim, name, w,
				h, bpp,hz);
		}
		printf ("\n");
	}
	
	// TODO try this auto-set gamma ramp on laptop
	// glfwSetGamma (monitor, 1.0);

	printf ("finding video modes...\n");
	{
		int count = 0;
		const GLFWvidmode* modes = glfwGetVideoModes (primary, &count);
		printf ("found %i video modes\n\n", count);
		for (int i = 0; i < count; i++) {
			int w = modes[i].width;
			int h = modes[i].height;
			int bpp = modes[i].blueBits + modes[i].greenBits + modes[i].redBits;
			int hz = modes[i].refreshRate;
			printf ("%3i) %ix%i %ibpp %iHz\n", i, w, h, bpp, hz);
		}
		printf ("\n");
	}


	printf ("shutdown\n");
	glfwTerminate();
	return 0;
}
