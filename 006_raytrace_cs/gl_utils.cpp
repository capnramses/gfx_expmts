/******************************************************************************\
| OpenGL 4 Example Code.                                                       |
| Accompanies written series "Anton's OpenGL 4 Tutorials"                      |
| Email: anton at antongerdelan dot net                                        |
| First version 27 Jan 2014                                                    |
| Copyright Dr Anton Gerdelan, Trinity College Dublin, Ireland.                |
| See individual libraries separate legal notices                              |
|******************************************************************************|
| This is just a file holding some commonly-used "utility" functions to keep   |
| the main file a bit easier to read. You can might build up something like    |
| this as learn more GL. Note that you don't need much code here to do good GL.|
| If you have a big object-oriented engine then maybe you can ask yourself if  |
| it is really making life easier.                                             |
\******************************************************************************/
#include "gl_utils.h"
#include "stb_image_write.h"
#include <stdio.h>
#include <time.h>
#include <string.h>
#include <assert.h>
#include <stdlib.h>
#define GL_LOG_FILE "gl.log"
#define MAX_SHADER_LENGTH 262144

/*--------------------------------LOG FUNCTIONS-------------------------------*/
bool restart_gl_log () {
	FILE* file = fopen (GL_LOG_FILE, "w");
	if (!file) {
		fprintf (
			stderr,
			"ERROR: could not open GL_LOG_FILE log file %s for writing\n",
			GL_LOG_FILE
		);
		return false;
	}
	time_t now = time (NULL);
	char* date = ctime (&now);
	fprintf (file, "GL_LOG_FILE log. local time %s\n", date);
	fclose (file);
	return true;
}

bool gl_log (const char* message, ...) {
	va_list argptr;
	FILE* file = fopen (GL_LOG_FILE, "a");
	if (!file) {
		fprintf (
			stderr,
			"ERROR: could not open GL_LOG_FILE %s file for appending\n",
			GL_LOG_FILE
		);
		return false;
	}
	va_start (argptr, message);
	vfprintf (file, message, argptr);
	va_end (argptr);
	fclose (file);
	return true;
}

/* same as gl_log except also prints to stderr */
bool gl_log_err (const char* message, ...) {
	va_list argptr;
	FILE* file = fopen (GL_LOG_FILE, "a");
	if (!file) {
		fprintf (
			stderr,
			"ERROR: could not open GL_LOG_FILE %s file for appending\n",
			GL_LOG_FILE
		);
		return false;
	}
	va_start (argptr, message);
	vfprintf (file, message, argptr);
	va_end (argptr);
	va_start (argptr, message);
	vfprintf (stderr, message, argptr);
	va_end (argptr);
	fclose (file);
	return true;
}

void debug_gl_callback (
	unsigned int source,
	unsigned int type,
	unsigned int id,
	unsigned int severity,
	int length,
	const char* message,
	void* userParam
) {
	char src_str[2048]; /* source */
	char type_str[2048]; /* type */
	char sev_str[2048]; /* severity */
	
	switch (source) {
		case 0x8246:
			strcpy (src_str, "API");
			break;
		case 0x8247:
			strcpy (src_str, "WINDOW_SYSTEM");
			break;
		case 0x8248:
			strcpy (src_str, "SHADER_COMPILER");
			break;
		case 0x8249:
			strcpy (src_str, "THIRD_PARTY");
			break;
		case 0x824A:
			strcpy (src_str, "APPLICATION");
			break;
		case 0x824B:
			strcpy (src_str, "OTHER");
			break;
		default:
			strcpy (src_str, "undefined");
			break;
	}
	
	switch (type) {
		case 0x824C:
			strcpy (type_str, "ERROR");
			break;
		case 0x824D:
			strcpy (type_str, "DEPRECATED_BEHAVIOR");
			break;
		case 0x824E:
			strcpy (type_str, "UNDEFINED_BEHAVIOR");
			break;
		case 0x824F:
			strcpy (type_str, "PORTABILITY");
			break;
		case 0x8250:
			strcpy (type_str, "PERFORMANCE");
			break;
		case 0x8251:
			strcpy (type_str, "OTHER");
			break;
		case 0x8268:
			strcpy (type_str, "MARKER");
			break;
		case 0x8269:
			strcpy (type_str, "PUSH_GROUP");
			break;
		case 0x826A:
			strcpy (type_str, "POP_GROUP");
			break;
		default:
			strcpy (type_str, "undefined");
			break;
	}
	
	switch (severity) {
		case 0x9146:
			strcpy (sev_str, "HIGH");
			break;
		case 0x9147:
			strcpy (sev_str, "MEDIUM");
			break;
		case 0x9148:
			strcpy (sev_str, "LOW");
			break;
		case 0x826B:
			strcpy (sev_str, "NOTIFICATION");
			break;
		default:
			strcpy (sev_str, "undefined");
			break;
	}
	
	fprintf (
		stderr,
		"source: %s type: %s id: %u severity: %s length: %i message: %s userParam: %i\n",
		src_str,
		type_str,
		id,
		sev_str,
		length,
		message,
		*(int*)userParam
	);
	;
}

/*--------------------------------GLFW3 and GLEW------------------------------*/
bool start_gl () {
	gl_log ("starting GLFW %s\n", glfwGetVersionString ());
	
	glfwSetErrorCallback (glfw_error_callback);
	if (!glfwInit ()) {
		fprintf (stderr, "ERROR: could not start GLFW3\n");
		return false;
	}

	// uncomment these lines if on Apple OS X
	glfwWindowHint (GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint (GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint (GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
	glfwWindowHint (GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

//	glfwWindowHint (GLFW_SAMPLES, 32);
	
	/*GLFWmonitor* mon = glfwGetPrimaryMonitor ();
	const GLFWvidmode* vmode = glfwGetVideoMode (mon);
	g_window = glfwCreateWindow (
		vmode->width, vmode->height, "Extended GL Init", mon, NULL
	);*/

	g_window = glfwCreateWindow (
		g_gl_width, g_gl_height, "Extended Init.", NULL, NULL
	);
	if (!g_window) {
		fprintf (stderr, "ERROR: could not open window with GLFW3\n");
		glfwTerminate();
		return false;
	}
	glfwSetWindowSizeCallback (g_window, glfw_window_size_callback);
	glfwMakeContextCurrent (g_window);
	
	// start GLEW extension handler
	glewExperimental = GL_TRUE;
	glewInit ();
	
	if (GLEW_KHR_debug) {
		int param = -1;
		printf ("KHR_debug extension found\n");
		glDebugMessageCallback ((GLDEBUGPROC)debug_gl_callback, &param);
		glEnable (GL_DEBUG_OUTPUT_SYNCHRONOUS);
		printf ("debug callback engaged\n");
	} else {
		printf ("KHR_debug extension NOT found\n");
	}

	// get version info
	const GLubyte* renderer = glGetString (GL_RENDERER); // get renderer string
	const GLubyte* version = glGetString (GL_VERSION); // version as a string
	printf ("Renderer: %s\n", renderer);
	printf ("OpenGL version supported %s\n", version);
	gl_log ("renderer: %s\nversion: %s\n", renderer, version);
	//vysnch
//	glfwSwapInterval (1);
	return true;
}

void glfw_error_callback (int error, const char* description) {
	fputs (description, stderr);
	gl_log_err ("%s\n", description);
}
// a call-back function
void glfw_window_size_callback (GLFWwindow* window, int width, int height) {
	g_gl_width = width;
	g_gl_height = height;
	printf ("width %i height %i\n", width, height);
	/* update any perspective matrices used here */
}

void _update_fps_counter (GLFWwindow* window) {
	static double previous_seconds = glfwGetTime ();
	static int frame_count;
	double current_seconds = glfwGetTime ();
	double elapsed_seconds = current_seconds - previous_seconds;
	if (elapsed_seconds > 0.25) {
		previous_seconds = current_seconds;
		double fps = (double)frame_count / elapsed_seconds;
		char tmp[128];
		 sprintf (tmp, "opengl @ fps: %.2f", fps);
		 glfwSetWindowTitle (window, tmp);
		 frame_count = 0;
	}
	frame_count++;
}

/*-----------------------------------SHADERS----------------------------------*/
bool parse_file_into_str (
	const char* file_name, char* shader_str, int max_len
) {
	shader_str[0] = '\0'; // reset string
	FILE* file = fopen (file_name , "r");
	if (!file) {
		gl_log_err ("ERROR: opening file for reading: %s\n", file_name);
		return false;
	}
	int current_len = 0;
	char line[2048];
	strcpy (line, ""); // remember to clean up before using for first time!
	while (!feof (file)) {
		if (NULL != fgets (line, 2048, file)) {
			current_len += strlen (line); // +1 for \n at end
			if (current_len >= max_len) {
				gl_log_err (
					"ERROR: shader length is longer than string buffer length %i\n",
					max_len
				);
			}
			strcat (shader_str, line);
		}
	}
	if (EOF == fclose (file)) { // probably unnecesssary validation
		gl_log_err ("ERROR: closing file from reading %s\n", file_name);
		return false;
	}
	return true;
}

void print_shader_info_log (GLuint shader_index) {
	int max_length = 2048;
	int actual_length = 0;
	char log[2048];
	glGetShaderInfoLog (shader_index, max_length, &actual_length, log);
	printf ("shader info log for GL index %i:\n%s\n", shader_index, log);
	gl_log ("shader info log for GL index %i:\n%s\n", shader_index, log);
}

bool create_shader (const char* file_name, GLuint* shader, GLenum type) {
	gl_log ("creating shader from %s...\n", file_name);
	char shader_string[MAX_SHADER_LENGTH];
	assert (parse_file_into_str (file_name, shader_string, MAX_SHADER_LENGTH));
	*shader = glCreateShader (type);
	const GLchar* p = (const GLchar*)shader_string;
	glShaderSource (*shader, 1, &p, NULL);
	glCompileShader (*shader);
	// check for compile errors
	int params = -1;
	glGetShaderiv (*shader, GL_COMPILE_STATUS, &params);
	if (GL_TRUE != params) {
		gl_log_err ("ERROR: GL shader index %i did not compile\n", *shader);
		print_shader_info_log (*shader);
		return false; // or exit or something
	}
	gl_log ("shader compiled. index %i\n", *shader);
	return true;
}

void print_programme_info_log (GLuint sp) {
	int max_length = 2048;
	int actual_length = 0;
	char log[2048];
	glGetProgramInfoLog (sp, max_length, &actual_length, log);
	printf ("program info log for GL index %u:\n%s", sp, log);
	gl_log ("program info log for GL index %u:\n%s", sp, log);
}

bool is_programme_valid (GLuint sp) {
	glValidateProgram (sp);
	GLint params = -1;
	glGetProgramiv (sp, GL_VALIDATE_STATUS, &params);
	if (GL_TRUE != params) {
		gl_log_err ("program %i GL_VALIDATE_STATUS = GL_FALSE\n", sp);
		print_programme_info_log (sp);
		return false;
	}
	gl_log ("program %i GL_VALIDATE_STATUS = GL_TRUE\n", sp);
	return true;
}

bool create_programme (GLuint vert, GLuint frag, GLuint* programme) {
	*programme = glCreateProgram ();
	gl_log (
		"created programme %u. attaching shaders %u and %u...\n",
		*programme,
		vert,
		frag
	);
	glAttachShader (*programme, vert);
	glAttachShader (*programme, frag);
	// link the shader programme. if binding input attributes do that before link
	glLinkProgram (*programme);
	GLint params = -1;
	glGetProgramiv (*programme, GL_LINK_STATUS, &params);
	if (GL_TRUE != params) {
		gl_log_err (
			"ERROR: could not link shader programme GL index %u\n",
			*programme
		);
		print_programme_info_log (*programme);
		return false;
	}
	assert (is_programme_valid (*programme));
	// delete shaders here to free memory
	glDeleteShader (vert);
	glDeleteShader (frag);
	return true;
}

GLuint create_programme_from_files (
	const char* vert_file_name, const char* frag_file_name
) {
	GLuint vert, frag, programme;
	assert (create_shader (vert_file_name, &vert, GL_VERTEX_SHADER));
	assert (create_shader (frag_file_name, &frag, GL_FRAGMENT_SHADER));
	assert (create_programme (vert, frag, &programme));
	return programme;
}

bool screencapture () {
	unsigned char* buffer = (unsigned char*)malloc (g_gl_width * g_gl_height * 3);
	glReadPixels (0, 0, g_gl_width, g_gl_height, GL_RGB, GL_UNSIGNED_BYTE, buffer);
	char name[1024];
	long int t = time (NULL);
	sprintf (name, "screenshot_%ld.png", t);
	unsigned char* last_row = buffer + (g_gl_width * 3 * (g_gl_height - 1));
	if (!stbi_write_png (name, g_gl_width, g_gl_height, 3, last_row, -3 * g_gl_width)) {
		fprintf (stderr, "ERROR: could not write screenshot file %s\n", name);
	}
	free (buffer);
	return true;
}

