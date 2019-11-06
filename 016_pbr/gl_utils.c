#include "gl_utils.h"
#include <stdio.h>
#include <assert.h>
#define MAX_SHADER_LENGTH 262144

GLuint g_gl_width = 1024, g_gl_height = 768;
GLFWwindow* g_window;

bool start_gl () {
	if (!glfwInit ()) {
		fprintf (stderr, "ERROR: could not start GLFW3\n");
		return false;
	}
#ifdef APPLE
	glfwWindowHint (GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint (GLFW_CONTEXT_VERSION_MINOR, 2);
	glfwWindowHint (GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
	glfwWindowHint (GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#endif
	glfwWindowHint (GLFW_SAMPLES, 16);
	g_window = glfwCreateWindow (g_gl_width, g_gl_height, "PBR", NULL, NULL);
	if (!g_window) {
		fprintf (stderr, "ERROR: could not open window with GLFW3\n");
		glfwTerminate ();
		return false;
	}
	glfwMakeContextCurrent (g_window);
	glewExperimental = GL_TRUE;
	glewInit ();
	const GLubyte* renderer = glGetString (GL_RENDERER); // get renderer string
	const GLubyte* version = glGetString (GL_VERSION); // version as a string
	printf ("renderer: %s\nversion: %s\n", renderer, version);
	return true;
}

bool parse_file_into_str (const char* file_name, char* shader_str, int max_len
	) {
	FILE* file = fopen (file_name , "r");
	if (!file) {
		fprintf (stderr, "ERROR: opening file for reading: %s\n", file_name);
		return false;
	}
	size_t cnt = fread (shader_str, 1, max_len - 1, file);
	if (cnt >= max_len - 1) {
		fprintf (stderr, "WARNING: file %s too big - truncated.\n", file_name);
	}
	if (ferror (file)) {
		fprintf (stderr, "ERROR: reading shader file %s\n", file_name);
		fclose (file);
		return false;
	}
	shader_str[cnt] = 0; // append \0 to end of file string
	fclose (file);
	return true;
}

void print_shader_info_log (GLuint shader_index) {
	int max_length = 2048;
	int actual_length = 0;
	char slog[2048];
	glGetShaderInfoLog (shader_index, max_length, &actual_length, slog);
	printf ("shader info log for GL index %i:\n%s\n", shader_index, slog);
}

bool create_shader (const char* file_name, GLuint* shader, GLenum type) {
	printf ("creating shader from %s...\n", file_name);
	char shader_string[MAX_SHADER_LENGTH];
	if (!parse_file_into_str (file_name, shader_string, MAX_SHADER_LENGTH)) {
		return false;
	}
	*shader = glCreateShader (type);
	const GLchar* p = (const GLchar*)shader_string;
	glShaderSource (*shader, 1, &p, NULL);
	glCompileShader (*shader);
	int params = -1;
	glGetShaderiv (*shader, GL_COMPILE_STATUS, &params);
	if (GL_TRUE != params) {
		fprintf (stderr, "ERROR: GL shader index %i did not compile\n", *shader);
		print_shader_info_log (*shader);
		return false;
	}
	printf ("shader compiled. index %i\n", *shader);
	return true;
}

void print_programme_info_log (GLuint sp) {
	int max_length = 2048;
	int actual_length = 0;
	char plog[2048];
	glGetProgramInfoLog (sp, max_length, &actual_length, plog);
	printf ("program info log for GL index %u:\n%s", sp, plog);
}

bool is_programme_valid (GLuint sp) {
	glValidateProgram (sp);
	GLint params = -1;
	glGetProgramiv (sp, GL_VALIDATE_STATUS, &params);
	if (GL_TRUE != params) {
		fprintf (stderr, "program %i GL_VALIDATE_STATUS = GL_FALSE\n", sp);
		print_programme_info_log (sp);
		return false;
	}
	printf ("program %i GL_VALIDATE_STATUS = GL_TRUE\n", sp);
	return true;
}

bool create_programme (GLuint vert, GLuint frag, GLuint* programme) {
	*programme = glCreateProgram ();
	glAttachShader (*programme, vert);
	glAttachShader (*programme, frag);
	// link the shader programme. if binding input attributes do that before link
	glLinkProgram (*programme);
	GLint params = -1;
	glGetProgramiv (*programme, GL_LINK_STATUS, &params);
	if (GL_TRUE != params) {
		fprintf (stderr, "ERROR: could not link shader programme GL index %u\n",
			*programme);
		print_programme_info_log (*programme);
		return false;
	}
	if (!is_programme_valid (*programme)) {
		return false;
	}
	// delete shaders here to free memory
	glDeleteShader (vert);
	glDeleteShader (frag);
	return true;
}

GLuint create_programme_from_files (const char* vert_file_name,
	const char* frag_file_name) {
	GLuint vert, frag, programme;
	assert (create_shader (vert_file_name, &vert, GL_VERTEX_SHADER));
	assert (create_shader (frag_file_name, &frag, GL_FRAGMENT_SHADER));
	assert (create_programme (vert, frag, &programme));
	return programme;
}
