//
// glfwgetkeyname test
// first version: anton gerdelan <gerdela@scss.tcd.ie> 3 december 2015
// trinity college dublin, ireland
//
// notes
// works for alphabet key printout i.e. "A"
// works for math symbols i.e. / = "divide"
// prefixes numpad keys with "num[name]"
// ~ = "grave"
// returns (null) for all modifier keys including shifts
// does not consider capslock state
// summary: still need localisation files with key name translations
// test on another (linux) machine:
// (null) for all numpad keys, '`' instead of `grave`, no space etc
// and numpad maths are identical names to others e.g. / == /

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <stdio.h>

void key_callback (GLFWwindow* window, int key, int scancode, int action,
	int mods) {
	printf ("key %i scancode %i action %i mods %i\n", key, scancode, action,
		mods);
	const char* str = glfwGetKeyName (key, scancode);
	printf ("glfw localised name: %s\n", str);
}

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

	glfwSetKeyCallback (window, key_callback);
	
	// start GLEW extension handler
	glewExperimental = GL_TRUE;
	glewInit ();
	
	// get version info
	const GLubyte* renderer = glGetString (GL_RENDERER); // get renderer string
	const GLubyte* version = glGetString (GL_VERSION); // version as a string
	printf ("Renderer: %s\n", renderer);
	printf ("OpenGL version supported %s\n", version);

	while (!glfwWindowShouldClose (window)) {
		glfwPollEvents ();
		glfwSwapBuffers (window);
	}


	printf ("shutdown\n");
	glfwTerminate();
	return 0;
}
