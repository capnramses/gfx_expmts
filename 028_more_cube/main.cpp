/*****************************************************************************\                                               |
| Copyright Dr Anton Gerdelan, Trinity College Dublin, Ireland.               |

Notes:
* cube face textures MUST BE SQUARE (not power of two)
* i used 2048 because my original loaded image was 2048x2048
* remember to set viewport to match each time
* glframebuffertexture to correct face each time you draw to a side
* projection doesnt /need/ a depth map but it's helpful
* not sure if depth map /must/ be a cube also, but im sure it helps
* projections write upside-down into cube texture (sigh)
* can fix projection writing by inverting "up" axis for projection cam
* 4096px shadow map + 6*multisampling was required to look nice

TODO:
* try with opengl 2.1 interface and renderbuffer instead of depth textures
* try with 2d depth texture instead of cube texture

\*****************************************************************************/

#include "maths_funcs.h" // my maths functions
#include "obj_parser.h" // my little Wavefront .obj mesh loader
#include "stb_image.h" // Sean Barrett's image loader - nothings.org
#include "gl_utils.h" // common opengl functions and small utilities like logs
#include <GL/glew.h> // include GLEW and new version of GL on Windows
#include <GLFW/glfw3.h> // GLFW helper library
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <math.h>
#define MESH_FILE "suzanne.obj"

/* choose pure reflection or pure refraction here. */
#define MONKEY_VERT_FILE "reflect_vs.glsl"
#define MONKEY_FRAG_FILE "reflect_fs.glsl"
//#define MONKEY_VERT_FILE "refract_vs.glsl"
//#define MONKEY_FRAG_FILE "refract_fs.glsl"

#define CUBE_VERT_FILE "cube_vs.glsl"
#define CUBE_FRAG_FILE "cube_fs.glsl"
#define WRITE_CUBE_VERT_FILE "write_cube.vert"
#define WRITE_CUBE_FRAG_FILE "write_cube.frag"
#define FRONT "negz.jpg"
#define BACK "posz.jpg"
#define TOP "posy.jpg"
#define BOTTOM "negy.jpg"
#define LEFT "negx.jpg"
#define RIGHT "posx.jpg"

// keep track of window size for things like the viewport and the mouse cursor
int g_gl_width = 640;
int g_gl_height = 480;
int g_cube_map_dims = 512; // 1880fps@2048px, 3900@1024px, 5800@512px
GLFWwindow* g_window = NULL;

/* big cube. returns Vertex Array Object */
GLuint make_big_cube () {
	float points[] = {
		-10.0f,  10.0f, -10.0f,
		-10.0f, -10.0f, -10.0f,
		 10.0f, -10.0f, -10.0f,
		 10.0f, -10.0f, -10.0f,
		 10.0f,  10.0f, -10.0f,
		-10.0f,  10.0f, -10.0f,

		-10.0f, -10.0f,  10.0f,
		-10.0f, -10.0f, -10.0f,
		-10.0f,  10.0f, -10.0f,
		-10.0f,  10.0f, -10.0f,
		-10.0f,  10.0f,  10.0f,
		-10.0f, -10.0f,  10.0f,

		 10.0f, -10.0f, -10.0f,
		 10.0f, -10.0f,  10.0f,
		 10.0f,  10.0f,  10.0f,
		 10.0f,  10.0f,  10.0f,
		 10.0f,  10.0f, -10.0f,
		 10.0f, -10.0f, -10.0f,
		 
		-10.0f, -10.0f,  10.0f,
		-10.0f,  10.0f,  10.0f,
		 10.0f,  10.0f,  10.0f,
		 10.0f,  10.0f,  10.0f,
		 10.0f, -10.0f,  10.0f,
		-10.0f, -10.0f,  10.0f,

		-10.0f,  10.0f, -10.0f,
		 10.0f,  10.0f, -10.0f,
		 10.0f,  10.0f,  10.0f,
		 10.0f,  10.0f,  10.0f,
		-10.0f,  10.0f,  10.0f,
		-10.0f,  10.0f, -10.0f,

		-10.0f, -10.0f, -10.0f,
		-10.0f, -10.0f,  10.0f,
		 10.0f, -10.0f, -10.0f,
		 10.0f, -10.0f, -10.0f,
		-10.0f, -10.0f,  10.0f,
		 10.0f, -10.0f,  10.0f
	};
	GLuint vbo;
	glGenBuffers (1, &vbo);
	glBindBuffer (GL_ARRAY_BUFFER, vbo);
	glBufferData (
		GL_ARRAY_BUFFER, 3 * 36 * sizeof (GLfloat), &points, GL_STATIC_DRAW);

	GLuint vao;
	glGenVertexArrays (1, &vao);
	glBindVertexArray (vao);
	glEnableVertexAttribArray (0);
	glBindBuffer (GL_ARRAY_BUFFER, vbo);
	glVertexAttribPointer (0, 3, GL_FLOAT, GL_FALSE, 0, NULL);
	return vao;
}

/* use stb_image to load an image file into memory, and then into one side of
a cube-map texture. */
bool load_cube_map_side (
	GLuint texture, GLenum side_target, const char* file_name
) {
	glBindTexture (GL_TEXTURE_CUBE_MAP, texture);

	/*int x, y, n;
	int force_channels = 4;
	unsigned char*  image_data = stbi_load (
		file_name, &x, &y, &n, force_channels);
	if (!image_data) {
		fprintf (stderr, "ERROR: could not load %s\n", file_name);
		return false;
	}
	// non-power-of-2 dimensions check
	if ((x & (x - 1)) != 0 || (y & (y - 1)) != 0) {
		fprintf (
			stderr, "WARNING: image %s is not power-of-2 dimensions\n", file_name
		);
	}
	
	// copy image data into 'target' side of cube map
	glTexImage2D (
		side_target,
		0,
		GL_RGBA,
		x,
		y,
		0,
		GL_RGBA,
		GL_UNSIGNED_BYTE,
		image_data
	);
	printf ("x,y is %i,%i\n",x,y);
	free (image_data);*/

	glTexImage2D (
		side_target,
		0,
		GL_R32F,
		g_cube_map_dims,
		g_cube_map_dims,
		0,
		GL_RED, // think this is the right way around (didnt work swapped around)
		GL_FLOAT, // unsigned byte only goes from 0 to 255!
		NULL
	);

	return true;
}

/* load all 6 sides of the cube-map from images, then apply formatting to the
final texture */
void create_cube_map (
	const char* front,
	const char* back,
	const char* top,
	const char* bottom,
	const char* left,
	const char* right,
	GLuint* tex_cube
) {
	// generate a cube-map texture to hold all the sides
	glActiveTexture (GL_TEXTURE0);
	glGenTextures (1, tex_cube);
	
	// load each image and copy into a side of the cube-map texture
	assert (load_cube_map_side (*tex_cube, GL_TEXTURE_CUBE_MAP_NEGATIVE_Z, front));
	assert (load_cube_map_side (*tex_cube, GL_TEXTURE_CUBE_MAP_POSITIVE_Z, back));
	assert (load_cube_map_side (*tex_cube, GL_TEXTURE_CUBE_MAP_POSITIVE_Y, top));
	assert (load_cube_map_side (*tex_cube, GL_TEXTURE_CUBE_MAP_NEGATIVE_Y, bottom));
	assert (load_cube_map_side (*tex_cube, GL_TEXTURE_CUBE_MAP_NEGATIVE_X, left));
	assert (load_cube_map_side (*tex_cube, GL_TEXTURE_CUBE_MAP_POSITIVE_X, right));
	// format cube map texture
	glTexParameteri (GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri (GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri (GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
	glTexParameteri (GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri (GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
}

// camera matrices. it's easier if they are global
mat4 view_mat;
mat4 proj_mat;
vec3 cam_pos (0.0f, 0.0f, 5.0f);

GLuint g_light_fb;
void init_fb (GLuint tex) {
	glGenFramebuffers (1, &g_light_fb);
	glBindFramebuffer(GL_FRAMEBUFFER, g_light_fb);
	glFramebufferTexture2D (GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
		GL_TEXTURE_CUBE_MAP_NEGATIVE_Z, tex, 0);
	GLuint depth_texture;
	glGenTextures(1, &depth_texture);
	glBindTexture(GL_TEXTURE_CUBE_MAP, depth_texture);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
	for (int i = 0; i < 6; i++) {
		glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_DEPTH_COMPONENT,
			g_cube_map_dims, g_cube_map_dims, 0,
			GL_DEPTH_COMPONENT, GL_UNSIGNED_BYTE, NULL);
	}
	glFramebufferTexture2D (GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
		GL_TEXTURE_CUBE_MAP_NEGATIVE_Z, depth_texture, 0);
	glBindTexture (GL_TEXTURE_CUBE_MAP, 0);
	// NOTE TODO -- try a renderbuffer instead http://docs.gl/gl4/glGenFramebuffers
	GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	if (status != GL_FRAMEBUFFER_COMPLETE) {
		fprintf (stderr, "ERROR FB incomplete\n");
	}
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

int main () {
/*--------------------------------START OPENGL--------------------------------*/
	assert (restart_gl_log ());
	// start GL context and O/S window using the GLFW helper library
	assert (start_gl ());
	
vec3 light_pos = vec3 (0.0,0.0,0.0);
/*---------------------------------CUBE MAP-----------------------------------*/
	GLuint cube_vao = make_big_cube ();
	GLuint cube_map_texture;
	create_cube_map (FRONT, BACK, TOP, BOTTOM, LEFT, RIGHT, &cube_map_texture);

	init_fb (cube_map_texture);
/*------------------------------CREATE GEOMETRY-------------------------------*/
	GLfloat* vp = NULL; // array of vertex points
	GLfloat* vn = NULL; // array of vertex normals
	GLfloat* vt = NULL; // array of texture coordinates
	int g_point_count = 0;
	assert (load_obj_file (MESH_FILE, vp, vt, vn, g_point_count));

	GLuint vao;
	glGenVertexArrays (1, &vao);
	glBindVertexArray (vao);

	GLuint points_vbo, normals_vbo;
	if (NULL != vp) {
		glGenBuffers (1, &points_vbo);
		glBindBuffer (GL_ARRAY_BUFFER, points_vbo);
		glBufferData (
			GL_ARRAY_BUFFER, 3 * g_point_count * sizeof (GLfloat), vp, GL_STATIC_DRAW
		);
		glVertexAttribPointer (0, 3, GL_FLOAT, GL_FALSE, 0, NULL);
		glEnableVertexAttribArray (0);
	}
	if (NULL != vn) {
		glGenBuffers (1, &normals_vbo);
		glBindBuffer (GL_ARRAY_BUFFER, normals_vbo);
		glBufferData (
			GL_ARRAY_BUFFER, 3 * g_point_count * sizeof (GLfloat), vn, GL_STATIC_DRAW
		);
		glVertexAttribPointer (1, 3, GL_FLOAT, GL_FALSE, 0, NULL);
		glEnableVertexAttribArray (1);
	}
	
/*-------------------------------CREATE SHADERS-------------------------------*/
	// shaders for "Suzanne" mesh
	GLuint monkey_sp = create_programme_from_files (
		MONKEY_VERT_FILE, MONKEY_FRAG_FILE
	);
	int monkey_M_location = glGetUniformLocation (monkey_sp, "M");
	int monkey_V_location = glGetUniformLocation (monkey_sp, "V");
	int monkey_P_location = glGetUniformLocation (monkey_sp, "P");
	int monkey_cam_pos_wor_location = glGetUniformLocation (monkey_sp, "cam_pos_wor");
	
	// cube-map shaders
	GLuint cube_sp = create_programme_from_files (
		CUBE_VERT_FILE, CUBE_FRAG_FILE
	);
	// note that this view matrix should NOT contain camera translation.
	int cube_V_location = glGetUniformLocation (cube_sp, "V");
	int cube_P_location = glGetUniformLocation (cube_sp, "P");
	int cube_light_pos_wor_location = glGetUniformLocation (cube_sp, "light_pos_wor");

	GLuint write_cube_sp = create_programme_from_files (
		WRITE_CUBE_VERT_FILE, WRITE_CUBE_FRAG_FILE
	);
	int write_cube_M_location = glGetUniformLocation (write_cube_sp, "M");
	int write_cube_V_location = glGetUniformLocation (write_cube_sp, "V");
	int write_cube_P_location = glGetUniformLocation (write_cube_sp, "P");
	int write_cube_light_pos_wor_location = glGetUniformLocation (write_cube_sp, "light_pos_wor");

/*-------------------------------CREATE CAMERA--------------------------------*/
	#define ONE_DEG_IN_RAD (2.0 * M_PI) / 360.0 // 0.017444444
	// input variables
	float near = 0.1f; // clipping plane
	float far = 100.0f; // clipping plane
	float fovy = 67.0f; // 67 degrees
	float aspect = (float)g_gl_width / (float)g_gl_height; // aspect ratio
	proj_mat = perspective (fovy, aspect, near, far);
		
	float cam_speed = 3.0f; // 1 unit per second
	float cam_heading_speed = 50.0f; // 30 degrees per second
	float cam_heading = 0.0f; // y-rotation in degrees
	mat4 T = translate (
		identity_mat4 (), vec3 (-cam_pos.v[0], -cam_pos.v[1], -cam_pos.v[2])
	);
	mat4 R = rotate_y_deg (identity_mat4 (), -cam_heading);
	versor q = quat_from_axis_deg (-cam_heading, 0.0f, 1.0f, 0.0f);
	view_mat = R * T;
	// keep track of some useful vectors that can be used for keyboard movement
	vec4 fwd (0.0f, 0.0f, -1.0f, 0.0f);
	vec4 rgt (1.0f, 0.0f, 0.0f, 0.0f);
	vec4 up (0.0f, 1.0f, 0.0f, 0.0f);
	
/*---------------------------SET RENDERING DEFAULTS---------------------------*/
	glUseProgram (monkey_sp);
	glUniformMatrix4fv (monkey_V_location, 1, GL_FALSE, view_mat.m);
	glUniformMatrix4fv (monkey_P_location, 1, GL_FALSE, proj_mat.m);
	glUseProgram (cube_sp);
	glUniformMatrix4fv (cube_V_location, 1, GL_FALSE, view_mat.m);
	glUniformMatrix4fv (cube_P_location, 1, GL_FALSE, proj_mat.m);
	// unique model matrix for each sphere
	mat4 model_mat = identity_mat4 ();
	
	glEnable (GL_DEPTH_TEST); // enable depth-testing
	glDepthFunc (GL_LESS); // depth-testing interprets a smaller value as "closer"
	glEnable (GL_CULL_FACE); // cull face
	glCullFace (GL_BACK); // cull back face
	glFrontFace (GL_CCW); // set counter-clock-wise vertex order to mean the front
	glClearColor (0.2, 0.2, 0.2, 1.0); // grey background to help spot mistakes
	glViewport (0, 0, g_gl_width, g_gl_height);
	
/*-------------------------------RENDERING LOOP-------------------------------*/
	while (!glfwWindowShouldClose (g_window)) {
		// update timers
		static double previous_seconds = glfwGetTime ();
		double current_seconds = glfwGetTime ();
		double elapsed_seconds = current_seconds - previous_seconds;
		previous_seconds = current_seconds;
		_update_fps_counter (g_window);

		{ // depth pass -- only need to do this when stuff herein actually moves
			glBindFramebuffer(GL_FRAMEBUFFER, g_light_fb);
			
			
			glViewport (0, 0, g_cube_map_dims, g_cube_map_dims);
			glClearColor (0.0,0.0,0.0,1.0);
			

			// TODO use shader to write into texture
			// BUT dont draw the big cube because it'll hall-of-mirrors up in here

			//for (int i = 0; i < 6; i++) {
				// TODO attach each side of texture to fb

				// TODO view and perspective

				//
			//}
			
			vec3 targ_pos[6] = {
				light_pos + vec3 (1.0, 0.0, 0.0), // posx
				light_pos + vec3 (-1.0, 0.0, 0.0), // negx
				light_pos + vec3 (0.0, 1.0, 0.0), //posy
				light_pos + vec3 (0.0, -1.0, 0.0), //negy
				light_pos + vec3 (0.0, 0.0, 1.0), //posz
				light_pos + vec3 (0.0, 0.0, -1.0) //negz
			};
			//upside-down up to store texture right- way up. hacky...
			vec3 up[6] = {
				vec3 (0.0,-1.0,0.0),
				vec3 (0.0,-1.0,0.0),
				vec3 (0.0,0.0,1.0),//posy
				vec3 (0.0,0.0,-1.0),//negy
				vec3 (0.0,-1.0,0.0),
				vec3 (0.0,-1.0,0.0)
			};
			mat4 light_V[6];
			for (int i = 0; i < 6; i++) {
				light_V[i] = look_at (light_pos, targ_pos[i],up[i]);
			}
			// NOTE: texture was upside-down!!! -- reversing y means y val is backwards, above
			
			mat4 light_P = perspective (90.0,1.0,0.01,100.0);

			//glDisable (GL_DEPTH_TEST); // enable depth-testing
			glUseProgram (write_cube_sp);

			glUniform3f (write_cube_light_pos_wor_location, light_pos.v[0],
				light_pos.v[1], light_pos.v[2]);
			glUniformMatrix4fv (write_cube_P_location, 1, GL_FALSE, light_P.m);
			glBindVertexArray (vao);
			float tx = sinf ((float)current_seconds);

			for (int i = 0; i < 6; i++) {
				glFramebufferTexture2D (GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
					GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, cube_map_texture, 0);
				glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

				glUniformMatrix4fv (write_cube_V_location, 1, GL_FALSE, light_V[i].m);
				model_mat = translate (identity_mat4 (), vec3 (5.0, 1.0 + tx, 0.0));
				glUniformMatrix4fv (write_cube_M_location, 1, GL_FALSE, model_mat.m);
				glDrawArrays (GL_TRIANGLES, 0, g_point_count);
				model_mat = translate (identity_mat4 (), vec3 (-5.0 + tx, -1.0, 0.0));
				glUniformMatrix4fv (write_cube_M_location, 1, GL_FALSE, model_mat.m);
				glDrawArrays (GL_TRIANGLES, 0, g_point_count);
				model_mat = translate (identity_mat4 (), vec3 (0.0, -1.0, -5.0 + tx));
				glUniformMatrix4fv (write_cube_M_location, 1, GL_FALSE, model_mat.m);
				glDrawArrays (GL_TRIANGLES, 0, g_point_count);
				model_mat = translate (identity_mat4 (), vec3 (0.0, 1.0, 5.0));
				glUniformMatrix4fv (write_cube_M_location, 1, GL_FALSE, model_mat.m);
				glDrawArrays (GL_TRIANGLES, 0, g_point_count);
				model_mat = translate (identity_mat4 (), vec3 (tx, 5.0, 0.0));
				glUniformMatrix4fv (write_cube_M_location, 1, GL_FALSE, model_mat.m);
				glDrawArrays (GL_TRIANGLES, 0, g_point_count);
				model_mat = translate (identity_mat4 (), vec3 (0.0, -5.0, 0.0));
				glUniformMatrix4fv (write_cube_M_location, 1, GL_FALSE, model_mat.m);
				glDrawArrays (GL_TRIANGLES, 0, g_point_count);
			}

			//glEnable (GL_DEPTH_TEST); // enable depth-testing
			glBindFramebuffer(GL_FRAMEBUFFER, 0);
		}
		
		glViewport (0, 0, g_gl_width, g_gl_height);
		glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		
		// render a sky-box using the cube-map texture
		glDepthMask (GL_FALSE);
		glUseProgram (cube_sp);
		glUniform3f (cube_light_pos_wor_location, light_pos.v[0],
				light_pos.v[1], light_pos.v[2]);
		glActiveTexture (GL_TEXTURE0);
		glBindTexture (GL_TEXTURE_CUBE_MAP, cube_map_texture);
		glBindVertexArray (cube_vao);
		glDrawArrays (GL_TRIANGLES, 0, 36);
		glDepthMask (GL_TRUE);
		
		glUseProgram (monkey_sp);
		glUniformMatrix4fv (monkey_V_location, 1, GL_FALSE, view_mat.m);
		glUniformMatrix4fv (monkey_P_location, 1, GL_FALSE, proj_mat.m);
		glUniform3f (monkey_cam_pos_wor_location, cam_pos.v[0], cam_pos.v[1],
			cam_pos.v[2]);
		glBindVertexArray (vao);
		float tx = sinf ((float)current_seconds);
		model_mat = translate (identity_mat4 (), vec3 (5.0, 1.0 + tx, 0.0));
		glUniformMatrix4fv (monkey_M_location, 1, GL_FALSE, model_mat.m);
		glDrawArrays (GL_TRIANGLES, 0, g_point_count);
		model_mat = translate (identity_mat4 (), vec3 (-5.0 + tx, -1.0, 0.0));
		glUniformMatrix4fv (monkey_M_location, 1, GL_FALSE, model_mat.m);
		glDrawArrays (GL_TRIANGLES, 0, g_point_count);
		model_mat = translate (identity_mat4 (), vec3 (0.0, -1.0, -5.0 + tx));
		glUniformMatrix4fv (monkey_M_location, 1, GL_FALSE, model_mat.m);
		glDrawArrays (GL_TRIANGLES, 0, g_point_count);
		model_mat = translate (identity_mat4 (), vec3 (0.0, 1.0, 5.0));
		glUniformMatrix4fv (monkey_M_location, 1, GL_FALSE, model_mat.m);
		glDrawArrays (GL_TRIANGLES, 0, g_point_count);
		model_mat = translate (identity_mat4 (), vec3 (tx, 5.0, 0.0));
		glUniformMatrix4fv (monkey_M_location, 1, GL_FALSE, model_mat.m);
		glDrawArrays (GL_TRIANGLES, 0, g_point_count);
		model_mat = translate (identity_mat4 (), vec3 (0.0, -5.0, 0.0));
		glUniformMatrix4fv (monkey_M_location, 1, GL_FALSE, model_mat.m);
		glDrawArrays (GL_TRIANGLES, 0, g_point_count);
		// update other events like input handling 
		glfwPollEvents ();
		
		// control keys
		bool cam_moved = false;
		vec3 move (0.0, 0.0, 0.0);
		float cam_yaw = 0.0f; // y-rotation in degrees
		float cam_pitch = 0.0f;
		float cam_roll = 0.0;
		if (glfwGetKey (g_window, GLFW_KEY_A)) {
			move.v[0] -= cam_speed * elapsed_seconds;
			cam_moved = true;
		}
		if (glfwGetKey (g_window, GLFW_KEY_D)) {
			move.v[0] += cam_speed * elapsed_seconds;
			cam_moved = true;
		}
		if (glfwGetKey (g_window, GLFW_KEY_Q)) {
			move.v[1] += cam_speed * elapsed_seconds;
			cam_moved = true;
		}
		if (glfwGetKey (g_window, GLFW_KEY_E)) {
			move.v[1] -= cam_speed * elapsed_seconds;
			cam_moved = true;
		}
		if (glfwGetKey (g_window, GLFW_KEY_W)) {
			move.v[2] -= cam_speed * elapsed_seconds;
			cam_moved = true;
		}
		if (glfwGetKey (g_window, GLFW_KEY_S)) {
			move.v[2] += cam_speed * elapsed_seconds;
			cam_moved = true;
		}
		if (glfwGetKey (g_window, GLFW_KEY_LEFT)) {
			cam_yaw += cam_heading_speed * elapsed_seconds;
			cam_moved = true;
			versor q_yaw = quat_from_axis_deg (
				cam_yaw, up.v[0], up.v[1], up.v[2]
			);
			q = q_yaw * q;
		}
		if (glfwGetKey (g_window, GLFW_KEY_RIGHT)) {
			cam_yaw -= cam_heading_speed * elapsed_seconds;
			cam_moved = true;
			versor q_yaw = quat_from_axis_deg (
				cam_yaw, up.v[0], up.v[1], up.v[2]
			);
			q = q_yaw * q;
		}
		if (glfwGetKey (g_window, GLFW_KEY_UP)) {
			cam_pitch += cam_heading_speed * elapsed_seconds;
			cam_moved = true;
			versor q_pitch = quat_from_axis_deg (
				cam_pitch, rgt.v[0], rgt.v[1], rgt.v[2]
			);
			q = q_pitch * q;
		}
		if (glfwGetKey (g_window, GLFW_KEY_DOWN)) {
			cam_pitch -= cam_heading_speed * elapsed_seconds;
			cam_moved = true;
			versor q_pitch = quat_from_axis_deg (
				cam_pitch, rgt.v[0], rgt.v[1], rgt.v[2]
			);
			q = q_pitch * q;
		}
		if (glfwGetKey (g_window, GLFW_KEY_Z)) {
			cam_roll -= cam_heading_speed * elapsed_seconds;
			cam_moved = true;
			versor q_roll = quat_from_axis_deg (
				cam_roll, fwd.v[0], fwd.v[1], fwd.v[2]
			);
			q = q_roll * q;
		}
		if (glfwGetKey (g_window, GLFW_KEY_C)) {
			cam_roll += cam_heading_speed * elapsed_seconds;
			cam_moved = true;
			versor q_roll = quat_from_axis_deg (
				cam_roll, fwd.v[0], fwd.v[1], fwd.v[2]
			);
			q = q_roll * q;
		}
		// update view matrix
		if (cam_moved) {
			cam_heading += cam_yaw;
		
			// re-calculate local axes so can move fwd in dir cam is pointing
			R = quat_to_mat4 (q);
			fwd = R * vec4 (0.0, 0.0, -1.0, 0.0);
			rgt = R * vec4 (1.0, 0.0, 0.0, 0.0);
			up = R * vec4 (0.0, 1.0, 0.0, 0.0);
			
			cam_pos = cam_pos + vec3 (fwd) * -move.v[2];
			cam_pos = cam_pos + vec3 (up) * move.v[1];
			cam_pos = cam_pos + vec3 (rgt) * move.v[0];
			print (cam_pos);
			mat4 T = translate (identity_mat4 (), vec3 (cam_pos));
			
			view_mat = inverse (R) * inverse (T);
		
			// cube-map view matrix has rotation, but not translation
			glUseProgram (cube_sp);
			glUniformMatrix4fv (cube_V_location, 1, GL_FALSE, view_mat.m);
		}
		
		
		if (GLFW_PRESS == glfwGetKey (g_window, GLFW_KEY_ESCAPE)) {
			glfwSetWindowShouldClose (g_window, 1);
		}
		// put the stuff we've been drawing onto the display
		glfwSwapBuffers (g_window);
	}
	
	// close GL context and any other GLFW resources
	glfwTerminate();
	return 0;
}
