#include "util.h"
#include "obj_parser.h"

GLFWwindow* start_gl () {
	GLFWwindow* window = NULL;
	{
		glfwInit ();
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
		glfwWindowHint (GLFW_SAMPLES, 16);
		window = glfwCreateWindow (VP_WIDTH, VP_HEIGHT, "sp omnishad", NULL, NULL);
		glfwMakeContextCurrent (window);
		glfwSwapInterval (1);
	}
	{
		glewExperimental = GL_TRUE;
		glewInit ();
		const GLubyte* renderer = glGetString (GL_RENDERER);
		const GLubyte* version = glGetString (GL_VERSION);
		printf ("Renderer: %s\n", renderer);
		printf ("OpenGL version supported %s\n", version);
	}
	{
		glEnable (GL_DEPTH_TEST);
		glDepthFunc (GL_LESS);
		glClearColor (0.01, 0.01, 0.25, 1.0);
	}
	return window;
}

uint init_db_panel () {
	float pts[] = {-1.0f, -1.0f, -1.0f, 1.0f, 1.0f, -1.0f, 1.0f, 1.0f};
	uint vao = 0, pts_vbo = 0;
	glGenVertexArrays (1, &vao);
	glBindVertexArray (vao);
	glEnableVertexAttribArray (0);
	glGenBuffers (1, &pts_vbo);
	glBindBuffer (GL_ARRAY_BUFFER, pts_vbo);
	glBufferData (GL_ARRAY_BUFFER, 32, pts, GL_STATIC_DRAW);
	glBindBuffer (GL_ARRAY_BUFFER, pts_vbo);
	glVertexAttribPointer (0, 2, GL_FLOAT, GL_FALSE, 0, NULL);
	return vao;
}

bool apg_file_to_str (const char* fn, size_t max_len, char* str) {
	FILE* fp = fopen (fn, "rb"); // because why not be safe
	if (!fp) {
		fprintf (stderr, "ERROR: opening file %s\n", fn);
		return false;
	}
	size_t cnt = fread (str, 1, max_len - 1, fp);
	if (cnt >= max_len - 1) {
		fprintf (stderr, "WARNING: file %s too big - truncated.\n", fn);
	}
	if (ferror (fp)) {
		fprintf (stderr, "ERROR: reading file %s\n", fn);
		fclose (fp);
		return false;
	}
	str[cnt] = 0; // append \0 to end of file string
	fclose (fp);
	return true;
}

void _print_s_info_log (uint s) {
	int max_length = 4096;
	int actual_length = 0;
	char slog[4096];
	glGetShaderInfoLog (s, max_length, &actual_length, slog);
	printf ("shader info log for GL index %u\n%s\n", s, slog);
}

void _print_sp_info_log (uint sp) {
	int max_length = 4096;
	int actual_length = 0;
	char slog[4096];
	glGetProgramInfoLog (sp, max_length, &actual_length, slog);
	printf ("programme info log for GL index %u\n%s\n", sp, slog);
}

uint create_sp_from_files (const char* vs_fn, const char* fs_fn) {
	printf ("creating sp from %s, %s\n", vs_fn, fs_fn);
	char vs_str[MAX_SHDR_LEN], fs_str[MAX_SHDR_LEN];
	assert (apg_file_to_str (vs_fn, MAX_SHDR_LEN, vs_str));
	assert (apg_file_to_str (fs_fn, MAX_SHDR_LEN, fs_str));
	uint vs = 0, fs = 0;
	vs = glCreateShader (GL_VERTEX_SHADER);
	fs = glCreateShader (GL_FRAGMENT_SHADER);
	const char* p = (const char*)vs_str;
	glShaderSource (vs, 1, &p, NULL);
	p = (const char*)fs_str;
	glShaderSource (fs, 1, &p, NULL);
	glCompileShader (vs);
	int params = -1;
	glGetShaderiv (vs, GL_COMPILE_STATUS, &params);
	if (GL_TRUE != params) {
		fprintf (stderr, "ERROR: v shader %u did not compile\n", vs);
		_print_s_info_log (vs);
		return 0;
	}
	glCompileShader (fs);
	params = -1;
	glGetShaderiv (fs, GL_COMPILE_STATUS, &params);
	if (GL_TRUE != params) {
		fprintf (stderr, "ERROR: f shader %u did not compile\n", fs);
		_print_s_info_log (fs);
		return 0;
	}
	uint sp = glCreateProgram ();
	glAttachShader (sp, vs);
	glAttachShader (sp, fs);
	glLinkProgram (sp);
	params = -1;
	glGetProgramiv (sp, GL_LINK_STATUS, &params);
	if (GL_TRUE != params) {
		printf ("linking FAILED\n");
		_print_sp_info_log (sp);
		return 0;
	}
	printf ("sp linked\n");
	return sp;
}

void create_vao_from_obj (const char* fn, uint* vao, int* pc) {
	float* v[3];
	assert (load_obj_file (fn, &v[0], &v[1], &v[2], pc));
	uint vbos[3];
	glGenVertexArrays (1, vao);
	glBindVertexArray (*vao);
	glGenBuffers (3, vbos);
	glEnableVertexAttribArray (0);
	glBindBuffer (GL_ARRAY_BUFFER, vbos[0]);
	glBufferData (GL_ARRAY_BUFFER, 4 * 3 * *pc, v[0], GL_STATIC_DRAW);
	glVertexAttribPointer (0, 3, GL_FLOAT, GL_FALSE, 0, NULL);
	glEnableVertexAttribArray (1);
	glBindBuffer (GL_ARRAY_BUFFER, vbos[1]);
	glBufferData (GL_ARRAY_BUFFER, 4 * 2 * *pc, v[1], GL_STATIC_DRAW);
	glVertexAttribPointer (1, 2, GL_FLOAT, GL_FALSE, 0, NULL);
	glEnableVertexAttribArray (2);
	glBindBuffer (GL_ARRAY_BUFFER, vbos[2]);
	glBufferData (GL_ARRAY_BUFFER, 4 * 3 * *pc, v[2], GL_STATIC_DRAW);
	glVertexAttribPointer (2, 3, GL_FLOAT, GL_FALSE, 0, NULL);
}