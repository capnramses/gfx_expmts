//
// Bezier Curve in OpenGL 4.1
// Anton Gerdelan
// 28 Nov 2014
//
#include "maths_funcs.h"
#include "obj_parser.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <stdlib.h>

//
// dimensions of the window drawing surface
int gl_width = 800;
int gl_height = 800;

int msaa = 16;

//
// copy a shader from a plain text file into a character array
bool parse_file_into_str (
	const char* file_name, char* shader_str, int max_len
) {
	FILE* file = fopen (file_name , "r");
	int current_len = 0;
	char line[2048];

	shader_str[0] = '\0'; /* reset string */
	if (!file) {
		fprintf (stderr, "ERROR: opening file for reading: %s\n", file_name);
		return false;
	}
	strcpy (line, ""); /* remember to clean up before using for first time! */
	while (!feof (file)) {
		if (NULL != fgets (line, 2048, file)) {
			current_len += strlen (line); /* +1 for \n at end */
			if (current_len >= max_len) {
				fprintf (stderr, 
					"ERROR: shader length is longer than string buffer length %i\n",
					max_len
				);
			}
			strcat (shader_str, line);
		}
	}
	if (EOF == fclose (file)) { /* probably unnecesssary validation */
		fprintf (stderr, "ERROR: closing file from reading %s\n", file_name);
		return false;
	}
	return true;
}

int main () {
	GLFWwindow* window = NULL;
	const GLubyte* renderer;
	const GLubyte* version;
	GLuint cube_sp, knot_sp;
	GLuint vao;

	//
	// Start OpenGL using helper libraries
	// --------------------------------------------------------------------------
	if (!glfwInit ()) {
		fprintf (stderr, "ERROR: could not start GLFW3\n");
		return 1;
	} 

	/* change to 3.2 if on Apple OS X
	glfwWindowHint (GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint (GLFW_CONTEXT_VERSION_MINOR, 0);
	glfwWindowHint (GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
	glfwWindowHint (GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE); */
	glfwWindowHint (GLFW_SAMPLES, msaa);
	window = glfwCreateWindow (gl_width, gl_height, "{quadratic bezier}", NULL, NULL);
	if (!window) {
		fprintf (stderr, "ERROR: opening OS window\n");
		return 1;
	}
	glfwMakeContextCurrent (window);
	
	

	glewExperimental = GL_TRUE;
	glewInit ();

	/* get version info */
	renderer = glGetString (GL_RENDERER); /* get renderer string */
	version = glGetString (GL_VERSION); /* version as a string */
	printf ("Renderer: %s\n", renderer);
	printf ("OpenGL version supported %s\n", version);

	int point_count = 0;
	//
	// Set up vertex buffers and vertex array object
	// --------------------------------------------------------------------------
	{
		GLfloat* vp = NULL; // array of vertex points
		GLfloat* vn = NULL; // array of vertex normals (we haven't used these yet)
		GLfloat* vt = NULL; // array of texture coordinates (or these)
		assert (load_obj_file ("smcube.obj", vp, vt, vn, point_count));
		
	
		GLuint points_vbo, texcoord_vbo, normal_vbo;
		glGenBuffers (1, &points_vbo);
		glBindBuffer (GL_ARRAY_BUFFER, points_vbo);
		glBufferData (GL_ARRAY_BUFFER, sizeof (float) * 3 * point_count,
			vp, GL_STATIC_DRAW);
		glGenBuffers (1, &texcoord_vbo);
		glBindBuffer (GL_ARRAY_BUFFER, texcoord_vbo);
		glBufferData (GL_ARRAY_BUFFER, sizeof (float) * 2 * point_count,
			vt, GL_STATIC_DRAW);
		glGenBuffers (1, &normal_vbo);
		glBindBuffer (GL_ARRAY_BUFFER, normal_vbo);
		glBufferData (GL_ARRAY_BUFFER, sizeof (float) * 3 * point_count,
			vn, GL_STATIC_DRAW);
	
		glGenVertexArrays (1, &vao);
		glBindVertexArray (vao);
		glEnableVertexAttribArray (0);
		glBindBuffer (GL_ARRAY_BUFFER, points_vbo);
		glVertexAttribPointer (0, 3, GL_FLOAT, GL_FALSE, 0, NULL);
		glEnableVertexAttribArray (1);
		glBindBuffer (GL_ARRAY_BUFFER, texcoord_vbo);
		glVertexAttribPointer (1, 2, GL_FLOAT, GL_FALSE, 0, NULL);
		glEnableVertexAttribArray (2);
		glBindBuffer (GL_ARRAY_BUFFER, normal_vbo);
		glVertexAttribPointer (2, 3, GL_FLOAT, GL_FALSE, 0, NULL);
		
		free (vp);
		free (vn);
		free (vt);
	}
	//
	// Load shaders from files
	// --------------------------------------------------------------------------
	{
		char* vertex_shader_str;
		char* fragment_shader_str;
		
		// allocate some memory to store shader strings
		vertex_shader_str = (char*)malloc (81920);
		fragment_shader_str = (char*)malloc (81920);
		// load shader strings from text files
		assert (parse_file_into_str ("cube.vert", vertex_shader_str, 81920));
		assert (parse_file_into_str ("cube.frag", fragment_shader_str, 81920));
		GLuint vs, fs;
		vs = glCreateShader (GL_VERTEX_SHADER);
		fs = glCreateShader (GL_FRAGMENT_SHADER);
		glShaderSource (vs, 1, (const char**)&vertex_shader_str, NULL);
		glShaderSource (fs, 1, (const char**)&fragment_shader_str, NULL);
		// free memory
		free (vertex_shader_str);
		free (fragment_shader_str);
		glCompileShader (vs);
		glCompileShader (fs);
		cube_sp = glCreateProgram ();
		glAttachShader (cube_sp, fs);
		glAttachShader (cube_sp, vs);
		glBindAttribLocation (cube_sp, 0, "vp");
		glBindAttribLocation (cube_sp, 1, "vt");
		glBindAttribLocation (cube_sp, 2, "vn");
		glLinkProgram (cube_sp);
	}
	{
		char* vertex_shader_str;
		char* fragment_shader_str;
		
		// allocate some memory to store shader strings
		vertex_shader_str = (char*)malloc (81920);
		fragment_shader_str = (char*)malloc (81920);
		// load shader strings from text files
		assert (parse_file_into_str ("knot.vert", vertex_shader_str, 81920));
		assert (parse_file_into_str ("knot.frag", fragment_shader_str, 81920));
		GLuint vs, fs;
		vs = glCreateShader (GL_VERTEX_SHADER);
		fs = glCreateShader (GL_FRAGMENT_SHADER);
		glShaderSource (vs, 1, (const char**)&vertex_shader_str, NULL);
		glShaderSource (fs, 1, (const char**)&fragment_shader_str, NULL);
		// free memory
		free (vertex_shader_str);
		free (fragment_shader_str);
		glCompileShader (vs);
		glCompileShader (fs);
		
		knot_sp = glCreateProgram ();
		glAttachShader (knot_sp, fs);
		glAttachShader (knot_sp, vs);
		glLinkProgram (knot_sp);
	}
	
	//
	// Create some matrices
	// --------------------------------------------------------------------------
		
	mat4 M, V, P;
	M = identity_mat4 ();//scale (identity_mat4 (), vec3 (0.05, 0.05, 0.05));
	vec3 cam_pos (0.0, 0.0, 15.0);
	vec3 targ_pos (0.0, 0.0, 0.0);
	vec3 up = normalise (vec3 (0.0, 1.0, 0.0));
	V = look_at (cam_pos, targ_pos, up);
	P = perspective (67.0f, (float)gl_width / (float)gl_height, 0.1, 1000.0);
	
	int M_loc = glGetUniformLocation (cube_sp, "M");
	int V_loc = glGetUniformLocation (cube_sp, "V");
	int P_loc = glGetUniformLocation (cube_sp, "P");
	int A_loc = glGetUniformLocation (cube_sp, "A");
	int B_loc = glGetUniformLocation (cube_sp, "B");
	int C_loc = glGetUniformLocation (cube_sp, "C");
	int t_loc = glGetUniformLocation (cube_sp, "t");
	// send matrix values to shader immediately
	glUseProgram (cube_sp);
	glUniformMatrix4fv (M_loc, 1, GL_FALSE, M.m);
	glUniformMatrix4fv (V_loc, 1, GL_FALSE, V.m);
	glUniformMatrix4fv (P_loc, 1, GL_FALSE, P.m);
	
	//
	// specific knots for bezier here A, C are start, end, B is control point
	//
	vec3 A = vec3 (-7.0f, -5.0f, 0.0f);
	vec3 B = vec3 (0.0f, 8.0f, 0.0f);
	vec3 C = vec3 (7.0f, -5.0f, 0.0f);
	glUniform3fv (A_loc, 1, A.v);
	glUniform3fv (B_loc, 1, B.v);
	glUniform3fv (C_loc, 1, C.v);
	
	int knot_loc = glGetUniformLocation (knot_sp, "pos");
	int knotP_loc = glGetUniformLocation (knot_sp, "P");
	int knotV_loc = glGetUniformLocation (knot_sp, "V");
	glUseProgram (knot_sp);
	glUniformMatrix4fv (knotV_loc, 1, GL_FALSE, V.m);
	glUniformMatrix4fv (knotP_loc, 1, GL_FALSE, P.m);
	
	//
	// Start rendering
	// --------------------------------------------------------------------------
	// tell GL to only draw onto a pixel if the fragment is closer to the viewer
	glEnable (GL_DEPTH_TEST); // enable depth-testing
	glDepthFunc (GL_LESS); // depth-testing interprets a smaller value as "closer"
	glDepthFunc (GL_LESS); // depth-testing is to use a "less than" function
	glEnable (GL_CULL_FACE); // enable culling of faces
	glCullFace (GL_BACK);
	glFrontFace (GL_CCW);
	glClearColor (0.04, 0.04, 0.75, 1.0);
	
	/* Render Points, allow resize in vertex shader */
	glEnable (GL_PROGRAM_POINT_SIZE);
	glPointParameteri (GL_POINT_SPRITE_COORD_ORIGIN, GL_LOWER_LEFT);

	float t = 0.0f;
	float speed = 0.5f;
	double prev = glfwGetTime ();
	while (!glfwWindowShouldClose (window)) {
		glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		// just the default viewport, covering the whole render area
		glViewport (0, 0, gl_width, gl_height);
		
		double curr = glfwGetTime ();
		double elapsed = curr - prev;
		prev = curr;
		
		//
		// move along spline
		if (GLFW_PRESS == glfwGetKey (window, GLFW_KEY_LEFT)) {
			t -= elapsed * speed;
			if (t < 0.0f) {
				t = 0.0f;
			}
		}
		if (GLFW_PRESS == glfwGetKey (window, GLFW_KEY_RIGHT)) {
			t += elapsed * speed;
			if (t > 1.0f) {
				t = 1.0f;
			}
		}
		
		//
		// render 3 knots
		glEnable (GL_PROGRAM_POINT_SIZE);
		glUseProgram (knot_sp);
		glUniform3fv (knot_loc, 1, A.v);
		glDrawArrays (GL_POINTS, 0, 1);
		glUseProgram (knot_sp);
		glUniform3fv (knot_loc, 1, B.v);
		glDrawArrays (GL_POINTS, 0, 1);
		glUseProgram (knot_sp);
		glUniform3fv (knot_loc, 1, C.v);
		glDrawArrays (GL_POINTS, 0, 1);
		glDisable (GL_PROGRAM_POINT_SIZE);
		
		glUseProgram (cube_sp);
		glBindVertexArray (vao);
		
		M = identity_mat4 ();//rotate_y_deg (identity_mat4 (), a);
		glUniformMatrix4fv (M_loc, 1, GL_FALSE, M.m);
		glUniform1f (t_loc, t);
		
		glDrawArrays (GL_TRIANGLES, 0, point_count);

		/* this just updates window events and keyboard input events (not used yet) */
		glfwPollEvents ();
		glfwSwapBuffers (window);
	}

	return 0;
}
