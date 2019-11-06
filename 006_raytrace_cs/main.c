/******************************************************************************\
| OpenGL 4 Example Code.                                                       |
| Accompanies written series "Anton's OpenGL 4 Tutorials"                      |
| Email: anton at antongerdelan dot net                                        |
| First version 27 Jan 2014                                                    |
| Copyright Dr Anton Gerdelan, Trinity College Dublin, Ireland.                |
| See individual libraries for separate legal notices                          |
|******************************************************************************|
| Extended Initialisation. Some extra detail.                                  |
\******************************************************************************/
#include "maths_funcs.h"
#include "gl_utils.h"
#include "stb_image_write.h"
#include <GL/glew.h> // include GLEW and new version of GL on Windows
#include <GLFW/glfw3.h> // GLFW helper library
#include <stdio.h>
#include <time.h>
#include <assert.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h> // for doing gl_log() functions that work like printf()

#define RAY_RATE 0.02
#define RES 512
//#define VID_REC

// keep track of window size for things like the viewport and the mouse cursor
int g_gl_width = RES;
int g_gl_height = RES;
GLFWwindow* g_window;

/* RGB scene texture */
unsigned char scene_img[RES][RES][4];
GLuint scene_tex_gl;
GLuint tex_output;

vec3 sphere_centre (0.0f, 0.0f, -10.0f);
float sphere_radius = 3.0f;

vec3 ray_os[801][801];
vec3 ray_ds[801][801];

unsigned char* g_video_memory_start = NULL;
unsigned char* g_video_memory_ptr = NULL;
int g_video_seconds_total = 10;
int g_video_fps = 25;

void reserve_video_memory () {
	// 480 MB at 800x800 resolution 230.4 MB at 640x480 resolution
	g_video_memory_ptr = (unsigned char*)malloc (
		RES * RES * 3 * g_video_fps * g_video_seconds_total
	);
	g_video_memory_start = g_video_memory_ptr;
}

void grab_video_frame () {
	// copy frame-buffer into 24-bit rgbrgb...rgb image
	glReadPixels (
		0, 0, RES, RES, GL_RGB, GL_UNSIGNED_BYTE, g_video_memory_ptr
	);
	// move video pointer along to the next frame's worth of bytes
	g_video_memory_ptr += RES * RES * 3;
}

bool dump_video_frame () {
static long int frame_number = 0;
	printf ("writing video frame %li\n", frame_number);
	// write into a file
	char name[1024];
	sprintf (name, "video_frame_%03ld.png", frame_number);
	
	unsigned char* last_row = g_video_memory_ptr +
		(g_gl_width * 3 * (g_gl_height - 1));
	if (!stbi_write_png (
		name, g_gl_width, g_gl_height, 3, last_row, -3 * g_gl_width
	)) {
		fprintf (stderr, "ERROR: could not write video file %s\n", name);
		return false;
	}

	frame_number++;
	return true;
}

bool dump_video_frames () {
	// reset iterating pointer first
	g_video_memory_ptr = g_video_memory_start;
	for (int i = 0; i < g_video_seconds_total * g_video_fps; i++) {
	if (!dump_video_frame ()) {
			return false;
		}
		g_video_memory_ptr += RES * RES * 3;
		printf ("vmp is now %p\n", g_video_memory_ptr);
	}
	free (g_video_memory_start);
	printf ("VIDEO IMAGES DUMPED\n");
	return true;
}

/* orthographic */
void generate_rays () {
	float max_x = 5.0f;
	float max_y = 5.0f;
	int col, row;
	
	glGenTextures (1, &scene_tex_gl);
	
	printf ("generating rays...\n");
	gl_log ("generating rays...\n");
	for (row = 0; row <= g_gl_height; row++) {
		for (col = 0; col <= g_gl_width; col++) {
			vec3 ray_origin (0.0f, 0.0f, 0.0f);
			float x = (float)(col * 2 - g_gl_width) / (float)g_gl_width;
			float y = (float)(row * 2 - g_gl_height) / (float)g_gl_height;
			ray_os[row][col] = vec3 (x * max_x, y * max_y, 0.0f);
			ray_ds[row][col] = vec3 (0.0f, 0.0f, -1.0f);
		}
	}
	gl_log ("done generating rays\n");
}

bool isect_ray_sphere (vec3 ray_o, vec3 ray_d, vec3 sphere_c, float sphere_r, float* t) {
	bool hit = false;
	
	// t = -b +- sqrt (b*b - c)
	vec3 omc = ray_o - sphere_c;
	float b = dot (ray_d, omc);
	float c = dot (omc, omc) - sphere_r * sphere_r;
	float bsqmc = b * b - c;
	
	// imaginary (miss)
	if (bsqmc < 0.0f) {
		return false;
	}
	
	// hit both sides
	if (bsqmc > 0.0f) {
		hit = true;
	
	// hit one side
	} else if (bsqmc == 0.0f) {
		hit = true;
	}
	
	if (hit) {
		float srbsqmc = sqrt (bsqmc);
		float pos_t = -b + srbsqmc;
		float neg_t = -b - srbsqmc;
		
		// one or more sides behind viewer (pos means `in direction of ray`)
		if (pos_t < 0.0f || neg_t < 0.0f) {
			return false;
		}
		// lesser is closer, even along -z direction
		if (pos_t < neg_t) {
			*t = pos_t;
		} else {
			*t = neg_t;
		}
		//printf ("t = %f\n", *t);
		
		return true;
	}
	
	gl_log_err ("err spher inte\n");
	return false;
}

void ray_trace_scene () {
	float max_range = 10.0f;
	int row, col;
	
	for (row = 0; row < g_gl_height; row++) {
		for (col = 0; col < g_gl_width; col++) {
			float t = 0.0f;
		
			/* clear texture */
			scene_img[row][col][0] = 32;
			scene_img[row][col][1] = 32;
			scene_img[row][col][2] = 32;
			scene_img[row][col][3] = 255;
		
			if (isect_ray_sphere (
				ray_os[row][col],
				ray_ds[row][col],
				sphere_centre,
				sphere_radius,
				&t
			)) {
				// set colour to sphere colour, and modify by range
				scene_img[row][col][0] = 0 * (1 - t / max_range);
				scene_img[row][col][1] = 255 * (1 - t / max_range) + 50;
				scene_img[row][col][2] = 0 * (1 - t / max_range);
				scene_img[row][col][3] = 255;
			}
			
		}
	}
	
	glActiveTexture (GL_TEXTURE0);
	glBindTexture (GL_TEXTURE_2D, scene_tex_gl);
	glTexImage2D (
		GL_TEXTURE_2D,
		0,
		GL_RGBA,
		g_gl_width,
		g_gl_height,
		0,
		GL_RGBA,
		GL_UNSIGNED_BYTE,
		scene_img
	);
	glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
}

int main () {
	const GLubyte* renderer;
	const GLubyte* version;
	GLfloat points[] = {
		-1.0f,  1.0f,  0.0f,
		-1.0f, -1.0f,  0.0f,
		 1.0f,  1.0f,  0.0f,
		 1.0f,  1.0f,  0.0f,
		-1.0f, -1.0f,  0.0f,
		 1.0f, -1.0f,  0.0f
	};
	GLuint vbo;
	GLuint vao;
	GLuint basic_sp, ray_sp, cs;
	char shader_str[100000];

	assert (restart_gl_log ());
	assert (start_gl ());
	
	basic_sp = create_programme_from_files ("basic.vert", "basic.frag");
	
	/* create compute programme */
	assert (parse_file_into_str ("ray.comp", shader_str, 100000));
	cs = glCreateShader (GL_COMPUTE_SHADER);
	const GLchar* p = (const GLchar*)shader_str;
	glShaderSource (cs, 1, &p, NULL);
	glCompileShader (cs);
	// check for compile errors
	int params = -1;
	glGetShaderiv (cs, GL_COMPILE_STATUS, &params);
	if (GL_TRUE != params) {
		gl_log_err ("ERROR: GL shader index %i did not compile\n", cs);
		print_shader_info_log (cs);
		return 1; // or exit or something
	}
	ray_sp = glCreateProgram ();
	glAttachShader (ray_sp, cs);
	glLinkProgram (ray_sp);
	params = -1;
	glGetProgramiv (ray_sp, GL_LINK_STATUS, &params);
	if (GL_TRUE != params) {
		gl_log_err (
			"ERROR: could not link shader programme GL index %u\n",
			ray_sp
		);
		print_programme_info_log (ray_sp);
		return 1;
	}
	assert (is_programme_valid (ray_sp));
	GLint ray_time_loc = glGetUniformLocation (ray_sp, "time");
	assert (ray_time_loc > -1);

	/* O/P texture for rays */
	glGenTextures (1, &tex_output);
	glActiveTexture (GL_TEXTURE0);
	glBindTexture (GL_TEXTURE_2D, tex_output);
	glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, RES, RES, 0, GL_RGBA, GL_FLOAT, NULL);
	// Because we're also using this tex as an image (in order to write to it),
	// we bind it to an image unit as well
	glBindImageTexture (0, tex_output, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);
	
	// CPU image for rays
	//generate_rays ();
	
	// query work group max sizes
	int work_grp_size[3], work_grp_inv;
	glGetIntegeri_v (GL_MAX_COMPUTE_WORK_GROUP_SIZE, 0, &work_grp_size[0]);
	glGetIntegeri_v (GL_MAX_COMPUTE_WORK_GROUP_SIZE, 1, &work_grp_size[1]);
	glGetIntegeri_v (GL_MAX_COMPUTE_WORK_GROUP_SIZE, 2, &work_grp_size[2]);
	printf ("max local work group sizes x:%i y:%i z:%i\n",
		work_grp_size[0],
		work_grp_size[1],
		work_grp_size[2]
	);
	glGetIntegerv (GL_MAX_COMPUTE_WORK_GROUP_INVOCATIONS, &work_grp_inv);
	printf ("max local work group invocations (volume) %i\n",
		work_grp_inv
	);
	
	// tell GL to only draw onto a pixel if the shape is closer to the viewer
	glEnable (GL_DEPTH_TEST); // enable depth-testing
	glDepthFunc (GL_LESS); // depth-testing interprets a smaller value as "closer"
	
	glGenBuffers (1, &vbo);
	glBindBuffer (GL_ARRAY_BUFFER, vbo);
	glBufferData (GL_ARRAY_BUFFER, 18 * sizeof (GLfloat), points, GL_STATIC_DRAW);
	
	glGenVertexArrays (1, &vao);
	glBindVertexArray (vao);
	glEnableVertexAttribArray (0);
	glBindBuffer (GL_ARRAY_BUFFER, vbo);
	glVertexAttribPointer (0, 3, GL_FLOAT, GL_FALSE, 0, NULL);

#ifdef VID_REC
	reserve_video_memory ();
	// initialise timers
	double video_timer = 0.0; // time video has been recording
	double video_dump_timer = 0.0; // timer for next frame grab
	double frame_time = 0.04; // 1/25 seconds of time
#endif

	double prev_time = glfwGetTime ();
	double timer = 0.0;
	while (!glfwWindowShouldClose (g_window)) {
		
		double curr_time = glfwGetTime ();
		double elapsed = curr_time - prev_time;
		prev_time = curr_time;
		timer += elapsed;
		_update_fps_counter (g_window);
				
				
#ifdef VID_REC
		// elapsed_seconds is seconds since last loop iteration
		video_timer += elapsed;
		video_dump_timer += elapsed;
		// only record 10s of video, then quit
		if (video_timer > 10.0) {
			break;
		}
#endif

		glUseProgram (ray_sp);

		if (timer >= RAY_RATE) {
			timer = 0.0;


			/* alternatives: CPU and GPGPU */
			//ray_trace_scene ();
		
			glUniform1f (ray_time_loc, (float)curr_time);
			// sends a single 'global work group' to GL for processing
			// xyz params divide this into 'local work groups' in 3 dimensions
			// local work group size in each dimension is defined in shader layout qualifier
			// each work group is a block of 'work items'
			// a compute shader is invoked to process each work item
			// example:
			// params 4,7,10 would give 4*7*10 = 280 work items per local work group
			glDispatchCompute (RES, RES, 1);
		}
		// wipe the drawing surface clear
		glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glViewport (0, 0, g_gl_width, g_gl_height);
		
		glActiveTexture (GL_TEXTURE0);
		glBindTexture (GL_TEXTURE_2D, tex_output);
		
		glUseProgram (basic_sp);
		glBindVertexArray (vao);
		// draw points 0-3 from the currently bound VAO with current in-use shader
		glDrawArrays (GL_TRIANGLES, 0, 6);

		// update other events like input handling 
		glfwPollEvents ();
		if (GLFW_PRESS == glfwGetKey (g_window, GLFW_KEY_ESCAPE)) {
			glfwSetWindowShouldClose (g_window, 1);
		}
		if (GLFW_PRESS == glfwGetKey (g_window, GLFW_KEY_F11)) {
			assert (screencapture ());
		}
#ifdef VID_REC
		while (video_dump_timer > frame_time) {
			grab_video_frame (); // 25 Hz so grab a frame
			video_dump_timer -= frame_time;
		}
#endif
		// put the stuff we've been drawing onto the display
		glfwSwapBuffers (g_window);
	}
	
#ifdef VID_REC
	dump_video_frames ();
#endif
	
	// close GL context and any other GLFW resources
	glfwTerminate();
	
	return 0;
}
