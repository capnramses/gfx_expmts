//
// generic shader interface prototype
// C99
// Anton Gerdelan
// 20 Nov 2015
//

#include "shader.h"
#include <stdio.h>
#include <string.h>

#define MAX_SHADER_STR_LEN 65536

Shaders g_shaders;

// returns false on error
bool init_shaders () {
	printf ("init_shaders start\n");
	memset (&g_shaders, 0, sizeof (Shaders));
	g_shaders.current_sp_i = -1;
	if (!_init_fallback_shaders ()) {
		fprintf (stderr, "ERROR: could not load default / fallback shaders\n");
		return false;
	}
	printf ("init_shaders end\n");
	return true;
}

// returns false on error
bool _init_fallback_shaders () {
	printf ("building fallback shaders\n");
	char vs_str[] = "#version 120\n"
		"attribute vec3 vp;\n"
		"uniform mat4 M, V, P;\n"
		"void main () {\n"
		"  gl_Position = P * V * M * vec4 (vp, 1.0);\n"
		"}";
	char fs_str[] = "#version 120\n"
		"void main () {\n"
		"  gl_FragColor = vec4 (0.5, 0.5, 0.5, 1.0);\n"
		"}";
	int sp_i = g_shaders.shader_count;
	g_shaders.shader_count++;
	GLuint vs = glCreateShader (GL_VERTEX_SHADER);
	g_shaders.shaders[sp_i].vs = vs;
	const char* p = (const char*)vs_str;
	glShaderSource (vs, 1, &p, NULL);
	glCompileShader (vs);
	GLint params = -1;
	glGetShaderiv (vs, GL_COMPILE_STATUS, &params);
	if (GL_TRUE != params) {
		fprintf (stderr, "ERROR: v shader %u did not compile\n", vs);
		_print_s_info_log (vs);
		return false; // or exit or something
	}
	GLuint fs = glCreateShader (GL_FRAGMENT_SHADER);
	g_shaders.shaders[sp_i].fs = fs;
	p = (const char*)fs_str;
	glShaderSource (fs, 1, &p, NULL);
	glCompileShader (fs);
	params = -1;
	glGetShaderiv (fs, GL_COMPILE_STATUS, &params);
	if (GL_TRUE != params) {
		fprintf (stderr, "ERROR: f shader %u did not compile\n", fs);
		_print_s_info_log (fs);
		return false; // or exit or something
	}
	g_shaders.shaders[sp_i].all_compiled = true;
	GLuint sp = glCreateProgram ();
	g_shaders.shaders[sp_i].sp = sp;
	glAttachShader (sp, vs);
	glAttachShader (sp, fs);
	glBindAttribLocation (sp, 0, "vp");
	if (!link_sp (sp)) {
		return false;
	}
	g_shaders.shaders[sp_i].linked = true;
	g_shaders.shaders[sp_i].vp_loc = glGetAttribLocation (sp, "vp");
	g_shaders.shaders[sp_i].vt_loc = glGetAttribLocation (sp, "vt");
	g_shaders.shaders[sp_i].vn_loc = glGetAttribLocation (sp, "vn");
	g_shaders.shaders[sp_i].M_loc = glGetUniformLocation (sp, "M");
	g_shaders.shaders[sp_i].V_loc = glGetUniformLocation (sp, "V");
	g_shaders.shaders[sp_i].P_loc = glGetUniformLocation (sp, "P");
	printf ("M_loc = %i\n", g_shaders.shaders[sp_i].M_loc);
	printf ("V_loc = %i\n", g_shaders.shaders[sp_i].V_loc);
	printf ("P_loc = %i\n", g_shaders.shaders[sp_i].P_loc);
	// set defaults in case they are never set in main code
	if (!uni_M (sp_i, identity_mat4 ())) {
		return false;
	}
	if (!uni_V (sp_i, identity_mat4 ())) {
		return false;
	}
	if (!uni_P (sp_i, identity_mat4 ())) {
		return false;
	}
	printf ("fallback shaders built\n");
	return true;
}

// returns false on error
bool create_sp_from_files (const char* vs_fn,	const char* fs_fn, int* sp_i) {
	printf ("creating shader programme from [%s] and [%s]\n", vs_fn, fs_fn);
	if (g_shaders.shader_count >= MAX_SHADERS) {
		fprintf (stderr, "ERROR: too many shaders loaded. Max %i\n", MAX_SHADERS);
		*sp_i = 0; // fallback shader index
		return false;
	}
	*sp_i = g_shaders.shader_count;
	g_shaders.shader_count++;
	GLuint vs = glCreateShader (GL_VERTEX_SHADER);
	if (!compile_s_from_file (vs, vs_fn)) {
		*sp_i = 0; // fallback shader index
		return false;
	}
	g_shaders.shaders[*sp_i].vs = vs;
	GLuint fs = glCreateShader (GL_FRAGMENT_SHADER);
	if (!compile_s_from_file (fs, fs_fn)) {
		*sp_i = 0; // fallback shader index
		return false;
	}
	g_shaders.shaders[*sp_i].fs = fs;
	g_shaders.shaders[*sp_i].all_compiled = true;
	GLuint sp = glCreateProgram ();
	g_shaders.shaders[*sp_i].sp = sp;
	glAttachShader (sp, fs);
	glAttachShader (sp, vs);
	// try to bind default locations for attribs
	glBindAttribLocation (sp, 0, "vp");
	glBindAttribLocation (sp, 1, "vt");
	glBindAttribLocation (sp, 2, "vn");
	if (!link_sp (sp)) {
		*sp_i = 0; // fallback shader index
		return false;
	}
	g_shaders.shaders[*sp_i].linked = true;
	// check for attribs
	g_shaders.shaders[*sp_i].vp_loc = glGetAttribLocation (sp, "vp");
	g_shaders.shaders[*sp_i].vt_loc = glGetAttribLocation (sp, "vt");
	g_shaders.shaders[*sp_i].vn_loc = glGetAttribLocation (sp, "vn");
	printf ("vp_loc = %i\n", g_shaders.shaders[*sp_i].vp_loc);
	printf ("vt_loc = %i\n", g_shaders.shaders[*sp_i].vt_loc);
	printf ("vn_loc = %i\n", g_shaders.shaders[*sp_i].vn_loc);
	// check for matrices
	g_shaders.shaders[*sp_i].M_loc = glGetUniformLocation (sp, "M");
	g_shaders.shaders[*sp_i].V_loc = glGetUniformLocation (sp, "V");
	g_shaders.shaders[*sp_i].P_loc = glGetUniformLocation (sp, "P");
	printf ("M_loc = %i\n", g_shaders.shaders[*sp_i].M_loc);
	printf ("V_loc = %i\n", g_shaders.shaders[*sp_i].V_loc);
	printf ("P_loc = %i\n", g_shaders.shaders[*sp_i].P_loc);
	return true;
}

// returns false on error
bool compile_s_from_file (GLuint s, const char* fn) {
	char str[MAX_SHADER_STR_LEN];
	str[0] = 0;
	printf ("parsing shader from %s\n", fn);
	if (!apg_file_to_str (fn, MAX_SHADER_STR_LEN, str)) {
		return false;
	}
	printf ("shader parsed\n");
	const char* p = (const char*)str;
	glShaderSource (s, 1, &p, NULL);
	glCompileShader (s);
	GLint params = -1;
	glGetShaderiv (s, GL_COMPILE_STATUS, &params);
	if (GL_TRUE != params) {
		fprintf (stderr, "ERROR: shader index %u [%s] did not compile\n", s, fn);
		_print_s_info_log (s);
		return false; // or exit or something
	}
	printf ("compiled shader %u from %s\n", s, fn);
	return true;
}

// does not do any malloc - fills existing buffer up to length max_len
// returns false on error
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

void _print_s_info_log (GLuint s) {
	int max_length = 4096;
	int actual_length = 0;
	char slog[4096];
	glGetShaderInfoLog (s, max_length, &actual_length, slog);
	printf ("shader info log for GL index %u\n%s\n", s, slog);
}

// create program and attach shaders first
// returns false on error
bool link_sp (GLuint sp) {
	printf ("linking shader programme %u\n", sp);
	glLinkProgram (sp);
	// check if link was successful
	GLint params = -1;
	glGetProgramiv (sp, GL_LINK_STATUS, &params);
	if (GL_TRUE != params) {
		printf ("linking FAILED\n");
		_print_sp_info_log (sp);
		return false;
	}
	printf ("linking done\n");
	return true;
}

void _print_sp_info_log (GLuint sp) {
	int max_length = 4096;
	int actual_length = 0;
	char slog[4096];
	glGetProgramInfoLog (sp, max_length, &actual_length, slog);
	printf ("programme info log for GL index %u\n%s\n", sp, slog);
}

// returns false on error
bool use_sp (int sp_i) {
	if (sp_i < 0 && sp_i >= g_shaders.shader_count) {
		fprintf (stderr, "ERROR: using shader prog. index %i invalid\n", sp_i);
		return false;
	}
	if (g_shaders.current_sp_i == sp_i) {
		return true;
	}
	glUseProgram (g_shaders.shaders[sp_i].sp);
	g_shaders.current_sp_i = sp_i;
	return true;
}

bool uni_M (int sp_i, mat4 M) {
	if (!use_sp (sp_i)) {
		fprintf (stderr, "ERROR: uniform M - invalid shader prog index %i\n", sp_i);
		return false;
	}
	if (g_shaders.shaders[sp_i].M_loc < 0) {
		fprintf (stderr, "ERROR: uniform M - no M location in sp %i\n", sp_i);
		return false;
	}
	glUniformMatrix4fv (g_shaders.shaders[sp_i].M_loc, 1, GL_FALSE, M.m);
	// TODO uniform count++
	return true;
}

bool uni_V (int sp_i, mat4 M) {
	if (!use_sp (sp_i)) {
		fprintf (stderr, "ERROR: uniform V - invalid shader prog index %i\n", sp_i);
		return false;
	}
	if (g_shaders.shaders[sp_i].V_loc < 0) {
		fprintf (stderr, "ERROR: uniform V - no M location in sp %i\n", sp_i);
		return false;
	}
	glUniformMatrix4fv (g_shaders.shaders[sp_i].V_loc, 1, GL_FALSE, M.m);
	// TODO uniform count++
	return true;
}

bool uni_P (int sp_i, mat4 M) {
	if (!use_sp (sp_i)) {
		fprintf (stderr, "ERROR: uniform P - invalid shader prog index %i\n", sp_i);
		return false;
	}
	if (g_shaders.shaders[sp_i].P_loc < 0) {
		fprintf (stderr, "ERROR: uniform P - no M location in sp %i\n", sp_i);
		return false;
	}
	glUniformMatrix4fv (g_shaders.shaders[sp_i].P_loc, 1, GL_FALSE, M.m);
	// TODO uniform count++
	return true;
}
