#include "maths_funcs.h"
#include "teapot.h" // teapot mesh
#include <GL/glew.h> // include GLEW and new version of GL on Windows
#include <GLFW/glfw3.h> // GLFW helper library
#include <stdio.h>
#include <math.h>

int width = 640;
int height = 480;
mat4 persp_proj = identity_mat4 ();
unsigned int teapot_vao = 0;

void load_teapot_xyz () {
	unsigned int vp_vbo = 0;
  glGenBuffers (1, &vp_vbo);
  glBindBuffer (GL_ARRAY_BUFFER, vp_vbo);
  glBufferData (
  	GL_ARRAY_BUFFER,
  	3 * teapot_vertex_count * sizeof (float),
  	teapot_vertex_points,
  	GL_STATIC_DRAW
  );
  unsigned int vn_vbo = 0;
  glGenBuffers (1, &vn_vbo);
  glBindBuffer (GL_ARRAY_BUFFER, vn_vbo);
  glBufferData (
  	GL_ARRAY_BUFFER,
  	3 * teapot_vertex_count * sizeof (float),
  	teapot_normals,
  	GL_STATIC_DRAW
  );
  
  glGenVertexArrays (1, &teapot_vao);
  glBindVertexArray (teapot_vao);
  glEnableVertexAttribArray (0);
  glBindBuffer (GL_ARRAY_BUFFER, vp_vbo);
  glVertexAttribPointer (0, 3, GL_FLOAT, GL_FALSE, 0, NULL);
  glEnableVertexAttribArray (1);
  glBindBuffer (GL_ARRAY_BUFFER, vn_vbo);
  glVertexAttribPointer (1, 3, GL_FLOAT, GL_FALSE, 0, NULL);
}

void window_size_callback (GLFWwindow* window, int w, int h) {
	width = w;
	height = h;
	float near = 0.1f; // clipping plane
	float far = 100.0f; // clipping plane
	float fov = 67.0f;
	float aspect = (float)width / (float)height; // aspect ratio
	persp_proj = perspective (fov, aspect, near, far);
}

int main () {
  // start GL context and O/S window using the GLFW helper library
  if (!glfwInit ()) {
    fprintf (stderr, "ERROR: could not start GLFW3\n");
    return 1;
  } 

	glfwWindowHint (GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint (GLFW_CONTEXT_VERSION_MINOR, 2);
	glfwWindowHint (GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
	glfwWindowHint (GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);


	GLFWmonitor* mon = glfwGetPrimaryMonitor ();
  GLFWwindow* window = glfwCreateWindow (width, height, "Hello Triangle", NULL, NULL);
  if (!window) {
    fprintf (stderr, "ERROR: could not open window with GLFW3\n");
    glfwTerminate();
    return 1;
  }
 
 glfwMakeContextCurrent (window);
 glfwSetWindowSizeCallback (window, window_size_callback);
                                  
  // start GLEW extension handler
	glewExperimental = GL_TRUE; 
	GLenum err = glewInit();
  if (GLEW_OK != err) {
    /* Problem: glewInit failed, something is seriously wrong. */
    printf ("Problem: glewInit failed, something is seriously wrong.\n");
    return false;
  }

                                        
  // get version info
  const GLubyte* renderer = glGetString (GL_RENDERER); // get renderer string
  const GLubyte* version = glGetString (GL_VERSION); // version as a string
  printf ("Renderer: %s\n", renderer);
  printf ("OpenGL version supported %s\n", version);

	// load teapot mesh into an array
	load_teapot_xyz ();

	const char* vertex_shader =
	"#version 150\n"
	"in vec3 vertex_position;"
	"in vec3 vertex_normals;"

	"uniform mat4 view, proj;"
	"uniform mat4 matrix;"

	"out vec3 n_eye;"

	"void main () {"
	"  n_eye = (view * vec4 (vertex_normals, 0.0)).xyz;"
	"  gl_Position = proj * view * matrix * vec4 (vertex_position, 1.0);"
	"}";
	
	const char* fragment_shader =
	"#version 150\n"
	"in vec3 n_eye;"
	"out vec4 frag_colour;"
	"void main () {"
	"  frag_colour = vec4 (n_eye, 1.0);"
	"}";
	
	unsigned int vs = glCreateShader (GL_VERTEX_SHADER);
	glShaderSource (vs, 1, &vertex_shader, NULL);
	glCompileShader (vs);
	unsigned int fs = glCreateShader (GL_FRAGMENT_SHADER);
	glShaderSource (fs, 1, &fragment_shader, NULL);
	glCompileShader (fs);
	unsigned int shader_programme = glCreateProgram ();
	glAttachShader (shader_programme, fs);
	glAttachShader (shader_programme, vs);
	glBindAttribLocation (shader_programme, 0, "vertex_position");
	glBindAttribLocation (shader_programme, 1, "vertex_normals");
	
	glLinkProgram (shader_programme);
	
	float matrix[] = {
		1.0f, 0.0f, 0.0f, 0.0f, // first column
		0.0f, 1.0f, 0.0f, 0.0f, // second column
		0.0f, 0.0f, 1.0f, 0.0f, // third column
		0.0f, 0.0f, 0.0f, 1.0f // fourth column
	};
	
	// input variables
	float near = 0.1f; // clipping plane
	float far = 100.0f; // clipping plane
	float fov = 67.0f;
	float aspect = (float)width / (float)height; // aspect ratio
	persp_proj = perspective (fov, aspect, near, far);
	float y_max = 20.0f;
	float y_min = -20.0f;
	float z_max = 100.0f;
	
	vec3 cam_pos (0.0f, 0.0f, -10.0f);
	vec3 target (0.0f, 0.0f, 0.0f);
	vec3 up (0.0f, 1.0f, 0.0f);
	mat4 view = identity_mat4 ();
	
	glUseProgram (shader_programme);
	int matrix_location = glGetUniformLocation (shader_programme, "matrix");
	int view_mat_location = glGetUniformLocation (shader_programme, "view");
	int proj_mat_location = glGetUniformLocation (shader_programme, "proj");
	
	// tell GL to only draw onto a pixel if the shape is closer to the viewer
  glEnable (GL_DEPTH_TEST); // enable depth-testing
  glDepthFunc (GL_LESS); // depth-testing interprets a smaller value as "closer"
	glClearColor (0.5f, 0.5f, 0.5f, 1.0f);
	//glEnable(GL_CULL_FACE);
 glCullFace(GL_BACK);
 glFrontFace(GL_CCW);



	double prev_time = glfwGetTime ();
	while (!glfwWindowShouldClose (window)) {
		double curr_time = glfwGetTime ();
		double elapsed_seconds = curr_time - prev_time;
		prev_time = curr_time;
		
		// rebuild every frame in case resize. too lazy to write out event-driven
		float aspect = (float)width / (float)height; // aspect ratio
		float x_max = 20.0f * aspect;
		float x_min = -20.0f * aspect;
		float ortho_proj[] = {
			2.0f / (x_max - x_min),
			0.0f,
			0.0f,
			0.0f,
			0.0f,
			2.0f / (y_max - y_min),
			0.0f,
			0.0f,
			0.0f,
			0.0f,
			-2.0f / z_max,
			0.0f,
			-(x_max + x_min) / (x_max - x_min),
			-(y_max + y_min) / (y_max - y_min),
			-1.0f,
			1.0f
		};
		
		glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		
		glUseProgram (shader_programme);
		glBindVertexArray (teapot_vao);
		
		// bottom-left
		cam_pos = vec3 (0.0f, 0.0f, 30.0f);
		target = vec3 (0.0f, 0.0f, 0.0f);
		up = vec3 (0.0f, 1.0f, 0.0f);
		view = look_at (cam_pos, target, up);
		
		glViewport (0, 0, width / 2, height / 2);
		glUniformMatrix4fv (proj_mat_location, 1, GL_FALSE, ortho_proj);
		glUniformMatrix4fv (view_mat_location, 1, GL_FALSE, view.m);
		glUniformMatrix4fv (matrix_location, 1, GL_FALSE, matrix);
		glDrawArrays (GL_TRIANGLES, 0, teapot_vertex_count);
		
		// bottom-right
		cam_pos = vec3 (20.0f, 20.0f, 20.0f);
		target = vec3 (0.0f, 0.0f, 0.0f);
		up = normalise (vec3 (-1.0f, 1.0f, -1.0f));
		view = look_at (cam_pos, target, up);
		
		glViewport (width / 2, 0, width / 2, height / 2);
		glUniformMatrix4fv (proj_mat_location, 1, GL_FALSE, persp_proj.m);
		glUniformMatrix4fv (view_mat_location, 1, GL_FALSE, view.m);
		glUniformMatrix4fv (matrix_location, 1, GL_FALSE, matrix);
		glDrawArrays (GL_TRIANGLES, 0, teapot_vertex_count);
		
		// top-left
		cam_pos = vec3 (0.0f, 30.0f, 0.0f);
		target = vec3 (0.0f, 0.0f, 0.0f);
		up = vec3 (0.0f, 0.0f, -1.0f);
		view = look_at (cam_pos, target, up);
		
		glViewport (0, height / 2, width / 2, height / 2);
		glUniformMatrix4fv (proj_mat_location, 1, GL_FALSE, ortho_proj);
		glUniformMatrix4fv (view_mat_location, 1, GL_FALSE, view.m);
		glUniformMatrix4fv (matrix_location, 1, GL_FALSE, matrix);
		glDrawArrays (GL_TRIANGLES, 0, teapot_vertex_count);
		
		// top-right
		cam_pos = vec3 (-30.0f, 0.0f, 0.0f);
		target = vec3 (0.0f, 0.0f, 0.0f);
		up = vec3 (0.0f, 1.0f, 0.0f);
		view = look_at (cam_pos, target, up);
		
		glViewport (width / 2, height / 2, width / 2, height / 2);
		glUniformMatrix4fv (proj_mat_location, 1, GL_FALSE, ortho_proj);
		glUniformMatrix4fv (view_mat_location, 1, GL_FALSE, view.m);
		glUniformMatrix4fv (matrix_location, 1, GL_FALSE, matrix);
		glDrawArrays (GL_TRIANGLES, 0, teapot_vertex_count);
		
		
		glfwSwapBuffers (window);
		glfwPollEvents ();
		if (GLFW_PRESS == glfwGetKey (window, GLFW_KEY_ESCAPE)) {
  		glfwSetWindowShouldClose (window, 1);
		}
		
		
	}
  
  // close GL context and any other GLFW resources
  glfwTerminate();
  return 0;
}


