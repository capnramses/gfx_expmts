//
// Basic Spinning Cube in OpenGL 2.1
// Anton Gerdelan
// 23 Nov 2014
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
	GLuint shader_programme;
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
	window = glfwCreateWindow (gl_width, gl_height, "Textured Mesh", NULL, NULL);
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
		//assert (load_obj_file ("cube.obj", vp, vt, vn, point_count));
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
		assert (parse_file_into_str ("teapot.vert", vertex_shader_str, 81920));
		assert (parse_file_into_str ("teapot.frag", fragment_shader_str, 81920));
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
		shader_programme = glCreateProgram ();
		glAttachShader (shader_programme, fs);
		glAttachShader (shader_programme, vs);
		glBindAttribLocation (shader_programme, 0, "vp");
		glBindAttribLocation (shader_programme, 1, "vt");
		glBindAttribLocation (shader_programme, 2, "vn");
		glLinkProgram (shader_programme);
		/* TODO NOTE: you should check for errors and print logs after compiling and also linking shaders */
	}
	
	//
	// Create some matrices
	// --------------------------------------------------------------------------
		
	mat4 M, V, P;
	M = identity_mat4 ();//scale (identity_mat4 (), vec3 (0.05, 0.05, 0.05));
	vec3 cam_pos (0.0, 5.0, 5.0);
	vec3 targ_pos (0.0, 0.0, 0.0);
	vec3 up = normalise (vec3 (0.0, 1.0, -1.0));
	V = look_at (cam_pos, targ_pos, up);
	P = perspective (67.0f, (float)gl_width / (float)gl_height, 0.1, 1000.0);
	
	int M_loc = glGetUniformLocation (shader_programme, "M");
	int V_loc = glGetUniformLocation (shader_programme, "V");
	int P_loc = glGetUniformLocation (shader_programme, "P");
	int ol_loc = glGetUniformLocation (shader_programme, "ol_mode");
	int sm_loc = glGetUniformLocation (shader_programme, "sm_shaded");
	// send matrix values to shader immediately
	glUseProgram (shader_programme);
	glUniformMatrix4fv (M_loc, 1, GL_FALSE, M.m);
	glUniformMatrix4fv (V_loc, 1, GL_FALSE, V.m);
	glUniformMatrix4fv (P_loc, 1, GL_FALSE, P.m);
	glUniform1f (ol_loc, 0.0f);
	glUniform1f (sm_loc, 0.0f);
	
	/*int x,y,n;
	unsigned char *data = stbi_load ("sword.png", &x, &y, &n, 0);
	if (!data) {
		fprintf (stderr, "ERROR: could not load image\n");
		return 1;
	}
	printf ("loaded image with [%i,%i] res and %i chans\n", x, y, n);
	GLuint tex;
	glGenTextures (1, &tex);
	glActiveTexture (GL_TEXTURE0);
	glBindTexture (GL_TEXTURE_2D, tex);
	glTexImage2D (
		GL_TEXTURE_2D,
		0,
		GL_RGBA,
		x,
		y,
		0,
		GL_RGBA,
		GL_UNSIGNED_BYTE,
		data
	);
	stbi_image_free(data);
	glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	*/
	
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
	glClearColor (0.5, 0.5, 0.5, 1.0);

	float a = 0.0f;
	double prev = glfwGetTime ();
	while (!glfwWindowShouldClose (window)) {
		glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		// just the default viewport, covering the whole render area
		glViewport (0, 0, gl_width, gl_height);
		
		double curr = glfwGetTime ();
		double elapsed = curr - prev;
		prev = curr;
		
		glUseProgram (shader_programme);
		glBindVertexArray (vao);
		
		a += sinf (elapsed * 50.0f);
		M = rotate_y_deg (identity_mat4 (), a);
		glUniformMatrix4fv (M_loc, 1, GL_FALSE, M.m);
		
		glFrontFace (GL_CW);
		glUniform1f (ol_loc, 1.0f);
		glUniform1f (sm_loc, 0.0f); // smooth shaded or not (exception is flat-shaded, they might not be great
		// if non-cube anyway due to scaling)
		glDrawArrays (GL_TRIANGLES, 0, point_count);
		glFrontFace (GL_CCW);
		glUniform1f (ol_loc, 0.0f);
		glDrawArrays (GL_TRIANGLES, 0, point_count);

		/* this just updates window events and keyboard input events (not used yet) */
		glfwPollEvents ();
		glfwSwapBuffers (window);
	}

	return 0;
}
