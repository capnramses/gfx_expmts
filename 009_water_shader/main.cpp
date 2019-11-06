/**** NOTE
reason refraction wasnt working is because view matrix for cube map had
translations in it. modified and removed. also did the same to V in the
sampling shaders for inverse(V)
and depth mask off for cube
*/

#include "obj_parser.h"
#include "maths_funcs.h"
#include "stb_image_write.h"
#include "stb_image.h"
#include <GL/glew.h> // include GLEW and new version of GL on Windows
#include <GLFW/glfw3.h> // GLFW helper library
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <assert.h>
#include <stdarg.h>
#include <string.h>
#define GL_LOG_FILE "gl.log"
#define AA_SAMPLES 16
#define VS_FILENAME "vs.glsl"
#define FS_FILENAME "fs.glsl"
#define MAX_SHADER_LEN 16384
#define MESH_NAME "water_8subs.obj"

int g_gl_width = 800;
int g_gl_height = 800;
unsigned char* g_video_memory_start = NULL;
unsigned char* g_video_memory_ptr = NULL;
int g_video_seconds_total = 10;
int g_video_fps = 25;

bool restart_gl_log () {
  FILE* file = fopen (GL_LOG_FILE, "w+");
  if (!file) {
    fprintf (stderr, "ERROR: could not open %s log file for writing\n", GL_LOG_FILE);
    return false;
  }
  time_t now = time (NULL);
  char* date = ctime (&now);
  fprintf (file, "%s log. local time %s", GL_LOG_FILE, date);
  fclose (file);
  return true;
}

bool gl_log (const char* message, ...) {
  FILE* file = fopen (GL_LOG_FILE, "a+");
  if (!file) {
    fprintf (stderr, "ERROR: could not open %s for writing\n", GL_LOG_FILE);
    return false;
  }
	va_list args;
  va_start (args, message);
  vfprintf (file, message, args);
  fclose (file);
	va_end (args);
  return true;
}

void log_gl_params () {
  GLenum params[] = {
		GL_COMPRESSED_TEXTURE_FORMATS,
		GL_MAX_COMPUTE_SHADER_STORAGE_BLOCKS,
		GL_MAX_COMPUTE_UNIFORM_BLOCKS,
		GL_MAX_COMPUTE_TEXTURE_IMAGE_UNITS,
		GL_MAX_COMPUTE_UNIFORM_COMPONENTS,
		GL_MAX_COMPUTE_ATOMIC_COUNTERS,
		GL_MAX_COMBINED_COMPUTE_UNIFORM_COMPONENTS,
		GL_MAX_COMPUTE_WORK_GROUP_INVOCATIONS,
    GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS,
    GL_MAX_CUBE_MAP_TEXTURE_SIZE,
    GL_MAX_DRAW_BUFFERS,
    GL_MAX_FRAGMENT_UNIFORM_COMPONENTS,
    GL_MAX_TEXTURE_IMAGE_UNITS,
    GL_MAX_TEXTURE_SIZE,
    GL_MAX_VARYING_FLOATS,
    GL_MAX_VERTEX_ATTRIBS,
    GL_MAX_VERTEX_TEXTURE_IMAGE_UNITS,
    GL_MAX_VERTEX_UNIFORM_COMPONENTS,
		GL_MAX_FRAMEBUFFER_WIDTH,
		GL_MAX_FRAMEBUFFER_HEIGHT,
    GL_MAX_VIEWPORT_DIMS,
    GL_STEREO,
  };
  const char* names[] = {
		"GL_COMPRESSED_TEXTURE_FORMATS",
		"GL_MAX_COMPUTE_SHADER_STORAGE_BLOCKS",
		"GL_MAX_COMPUTE_UNIFORM_BLOCKS",
		"GL_MAX_COMPUTE_TEXTURE_IMAGE_UNITS",
		"GL_MAX_COMPUTE_UNIFORM_COMPONENTS",
		"GL_MAX_COMPUTE_ATOMIC_COUNTERS",
		"GL_MAX_COMBINED_COMPUTE_UNIFORM_COMPONENTS",
		"GL_MAX_COMPUTE_WORK_GROUP_INVOCATIONS",
    "GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS",
    "GL_MAX_CUBE_MAP_TEXTURE_SIZE",
    "GL_MAX_DRAW_BUFFERS",
    "GL_MAX_FRAGMENT_UNIFORM_COMPONENTS",
    "GL_MAX_TEXTURE_IMAGE_UNITS",
    "GL_MAX_TEXTURE_SIZE",
    "GL_MAX_VARYING_FLOATS",
    "GL_MAX_VERTEX_ATTRIBS",
    "GL_MAX_VERTEX_TEXTURE_IMAGE_UNITS",
    "GL_MAX_VERTEX_UNIFORM_COMPONENTS",
		"GL_MAX_FRAMEBUFFER_WIDTH",
		"GL_MAX_FRAMEBUFFER_HEIGHT",
    "GL_MAX_VIEWPORT_DIMS",
    "GL_STEREO",
  };
  gl_log ("GL Context Params:\n");
	// strings

  // integers - only works if the order is 0-10 integer return types
  for (int i = 1; i < 20; i++) {
    int v = 0;
    glGetIntegerv (params[i], &v);
    gl_log ("%s %i\n", names[i], v);
  }
  // others
  int v[2];
  v[0] = v[1] = 0;
  glGetIntegerv (params[20], v);
  gl_log ("%s %i %i\n", names[20], v[0], v[1]);
  unsigned char s = 0;
  glGetBooleanv (params[21], &s);
  gl_log ("%s %i\n", names[21], (unsigned int)s);
  gl_log ("-----------------------------------------------\n");
}

void glfw_error_callback (int error, const char* description) {
  fputs (description, stderr);
  gl_log ("%s:%i: %s\n", __FILE__, __LINE__, description);
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
     sprintf (tmp, "opengl @ fps: %.2lf", fps);
     glfwSetWindowTitle (window, tmp);
     frame_count = 0;
  }
  frame_count++;
}

void _print_shader_info_log (unsigned int shader_index) {
  int max_length = 2048;
  int actual_length = 0;
  char log[2048];
  glGetShaderInfoLog (shader_index, max_length, &actual_length, log);
  printf ("shader info log for GL index %i:\n%s\n", shader_index, log);
}

void _print_programme_info_log (unsigned int sp) {
  int max_length = 2048;
  int actual_length = 0;
  char log[2048];
  glGetProgramInfoLog (sp, max_length, &actual_length, log);
  printf ("program info log for GL index %i:\n%s", sp, log);
}

bool parse_file_into_str (const char* file_name, char* shader_str, int max_len) {
	shader_str[0] = '\0'; // reset string
	FILE* file = fopen (file_name , "r");
	if (!file) {
		printf ("ERROR: opening file for reading %s\n", file_name);
		gl_log ("ERROR: opening file for reading ", file_name);
		return false;
	}
	int current_len = 0;
	char line[2048];
	strcpy (line, ""); // remember to clean up before using for first time!
	while (!feof (file)) {
		if (NULL != fgets (line, 2048, file)) {
			current_len += strlen (line); // +1 for \n at end
			if (current_len >= max_len) {
				fprintf (stderr, "ERROR: shader %s length is longer than string buffer length %i\n", file_name, max_len);
				gl_log ("ERROR: shader length is longer than string buffer length %i", max_len);
			}
			strcat (shader_str, line);
		}
	}
	if (EOF == fclose (file)) { // probably unnecesssary validation
		fprintf (stderr, "ERROR: closing file from reading %s\n", file_name);
		gl_log ("ERROR: closing file from reading %s", file_name);
		return false;
	}
	return true;
}

bool screenshot () {
	unsigned char* buffer = (unsigned char*)malloc (g_gl_width * g_gl_height * 3);
	glReadPixels (0, 0, g_gl_width, g_gl_height, GL_RGB, GL_UNSIGNED_BYTE, buffer);
	char name[1024];
	long int t = time (NULL);
	sprintf (name, "screenshot_%ld.png", t);
	unsigned char* last_row = buffer + (g_gl_width * 3 * (g_gl_height - 1));
	if (!stbi_write_png (name, g_gl_width, g_gl_height, 3, last_row, -3 * g_gl_width)) {
		fprintf (stderr, "ERROR: could not write screenshot file %s\n", name);
		gl_log ("ERROR: could not write screenshot file %s\n", name);
		return false;
	}
	free (buffer);
	return true;
}

void reserve_video_memory () {
	// 480 MB at 800x800 resolution 230.4 MB at 640x480 resolution
	g_video_memory_ptr = (unsigned char*)malloc (g_gl_width * g_gl_height * 3 * g_video_fps * g_video_seconds_total);
	g_video_memory_start = g_video_memory_ptr;
}

void grab_video_frame () {
	// copy frame-buffer into 24-byte rgbrgb...rgb image
	glReadPixels (0, 0, g_gl_width, g_gl_height, GL_RGB, GL_UNSIGNED_BYTE, g_video_memory_ptr);
	// move video pointer along to the next frame's worth of bytes
	g_video_memory_ptr += g_gl_width * g_gl_height * 3;
}

/* using Sean Barrett's stb_image_write.h to quickly convert to PNG */
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
		g_video_memory_ptr += g_gl_width * g_gl_height * 3;
	}
	free (g_video_memory_start);
	printf ("VIDEO IMAGES DUMPED\n");
	return true;
}

bool load_image_to_texture (
	const char* file_name, unsigned int& tex, bool gen_mips
) {
	gl_log ("loading image %s...\n", file_name);
	
	int x, y, n;
	unsigned char* image_data = stbi_load (file_name, &x, &y, &n, 4);
	if (!image_data) {
		fprintf (
			stderr,
			"ERROR: could not load image %s. Check file type and path\n",
			file_name
		);
		return false;
	}
	gl_log ("image loaded: %ix%i %i bytes per pixel\n", x, y, n);
	// NPOT check
	if (x & (x - 1) != 0 || y & (y - 1) != 0) {
		fprintf (
			stderr, "WARNING: texture %s is not power-of-2 dimensions\n", file_name
		);
	}
	
	// FLIP UP-SIDE DIDDLY-DOWN
	// make upside-down copy for GL
	unsigned char *imagePtr = &image_data[0];
	int halfTheHeightInPixels = y / 2;
	int heightInPixels = y;
	
	// Assuming RGBA for 4 components per pixel.
	int numColorComponents = 4;
	// Assuming each color component is an unsigned char.
	int widthInChars = x * numColorComponents;
	unsigned char *top = NULL;
	unsigned char *bottom = NULL;
	unsigned char temp = 0;
	for( int h = 0; h < halfTheHeightInPixels; h++) {

		top = imagePtr + h * widthInChars;
		bottom = imagePtr + (heightInPixels - h - 1) * widthInChars;
		for( int w = 0; w < widthInChars; ++w )
		{

			// Swap the chars around.
			temp = *top;
			*top = *bottom;
			*bottom = temp;
			++top;
			++bottom;
		}
	}
	
	// Copy into an OpenGL texture
	glGenTextures (1, &tex);
	glActiveTexture (GL_TEXTURE0);
	glBindTexture (GL_TEXTURE_2D, tex);
	
	/*
	Specifies the alignment requirements for the start of each pixel row in
	memory. The allowable values are 1 (byte-alignment), 2 (rows aligned to
	even-numbered bytes), 4 (word-alignment), and 8 (rows start on double-word
	boundaries).
	*/
	
	glPixelStorei (GL_UNPACK_ALIGNMENT, 1);
	glTexImage2D (
		GL_TEXTURE_2D,
		0,
		GL_RGBA,
		x,
		y,
		0,
		GL_RGBA,
		GL_UNSIGNED_BYTE,
		image_data
	);
	// NOTE: need this or it will not load the texture at all
	if (gen_mips) {
		// shd be in core since 3.0 according to:
		// http://www.opengl.org/wiki/Common_Mistakes#Automatic_mipmap_generation
		// next line is to circumvent possible extant ATI bug
		// but NVIDIA throws a warning glEnable (GL_TEXTURE_2D);
		glGenerateMipmap (GL_TEXTURE_2D);
		gl_log ("mipmaps generated\n");
		glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	} else {
		glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	}
	glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	
	gl_log ("image loading done\n");
	stbi_image_free(image_data);
	return true;
}

bool load_cube_map (
	const char* posx,
	const char* negx,
	const char* posy,
	const char* negy,
	const char* posz,
	const char* negz,
	unsigned int& cube_tex
) {

	int x, y, n;
	unsigned char* image_data[6];
	image_data[0] = stbi_load (posx, &x, &y, &n, 4);
	image_data[1] = stbi_load (negx, &x, &y, &n, 4);
	image_data[2] = stbi_load (posy, &x, &y, &n, 4);
	image_data[3] = stbi_load (negy, &x, &y, &n, 4);
	image_data[4] = stbi_load (posz, &x, &y, &n, 4);
	image_data[5] = stbi_load (negz, &x, &y, &n, 4);

	// TODO assert image loaded, and NPOT check, and w/h consist

	GLenum target[] = {
		GL_TEXTURE_CUBE_MAP_POSITIVE_X,
		GL_TEXTURE_CUBE_MAP_NEGATIVE_X,
		GL_TEXTURE_CUBE_MAP_POSITIVE_Y,
		GL_TEXTURE_CUBE_MAP_NEGATIVE_Y,
		GL_TEXTURE_CUBE_MAP_POSITIVE_Z,
		GL_TEXTURE_CUBE_MAP_NEGATIVE_Z,
	};

	glGenTextures (1, &cube_tex);
	glBindTexture (GL_TEXTURE_CUBE_MAP, cube_tex);

	for (int i = 0; i < 6; i++) {
		glTexImage2D (
			target[i],
		 	0,
		 	GL_RGBA,
		 	x,
		 	y,
		 	0,
		 	GL_RGBA,
		 	GL_UNSIGNED_BYTE,
		 	image_data[i]
		);
	}
	glTexParameteri (GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri (GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri (GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
	glTexParameteri (GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri (GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	return true;
}

int main () {
/*-----------------------------------------------------------------------------\
|                                    INIT                                      |
\---------------------------------------------------------------------------- */
	reserve_video_memory ();
	bool dump_video = false;
	double video_timer = 0.0; // time video has been recording
	double video_dump_timer = 0.0; // timer for next frame grab
	double frame_time = 0.04; // 1/25 seconds of time

	restart_gl_log ();
	timespec clock_res;
	int err_code = clock_getres (CLOCK_MONOTONIC, &clock_res);
	gl_log ("CLOCK_MONOTONIC res. nanosecs: %li\n", clock_res.tv_nsec);

	assert (gl_log ("starting GLFW %s\n", glfwGetVersionString ()));
	glfwSetErrorCallback (glfw_error_callback);
	if (!glfwInit ()) {
		fprintf (stderr, "ERROR: could not start GLFW3\n");
		return 1;
	}

  // start GL context and O/S window using the GLFW helper library
  if (!glfwInit ()) {
    fprintf (stderr, "ERROR: could not start GLFW3\n");
    return 1;
  }
	// try to shut down glfw properly whenever exiting normally
	atexit (glfwTerminate);
	glfwWindowHint (GLFW_SAMPLES, AA_SAMPLES);
	// uncomment these lines if on Apple OS X
  /*glfwWindowHint (GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint (GLFW_CONTEXT_VERSION_MINOR, 2);
  glfwWindowHint (GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
  glfwWindowHint (GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);*/

  GLFWwindow* window = glfwCreateWindow (
		g_gl_width, g_gl_height, "Water Shader", NULL, NULL
	);
  if (!window) {
    fprintf (stderr, "ERROR: could not open window with GLFW3\n");
    return 1;
  }
  glfwMakeContextCurrent (window);
                                  
  // start GLEW extension handler
  glewExperimental = GL_TRUE;
  glewInit ();
	
  // get version info
  const GLubyte* renderer = glGetString (GL_RENDERER); // get renderer string
  const GLubyte* version = glGetString (GL_VERSION); // version as a string
	gl_log ("Renderer: %s\n", renderer);
	gl_log ("OpenGL version supported %s\n", version);
	log_gl_params ();

  // tell GL to only draw onto a pixel if the shape is closer to the viewer
  glEnable (GL_DEPTH_TEST); // enable depth-testing
  glDepthFunc (GL_LESS); // depth-testing interprets a smaller value as "closer"

/*-----------------------------------------------------------------------------\
|                                    MESH                                      |
\---------------------------------------------------------------------------- */
	float* normals = NULL;
	float* tex_coords = NULL;
	float* points = NULL;
	int point_count;
	assert (load_obj_file (
		MESH_NAME,
		points,
		tex_coords,
		normals,
		point_count
	));
	gl_log ("mesh %s loaded with %i points\n", MESH_NAME, point_count);

	unsigned int vbo = 0;
	glGenBuffers (1, &vbo);
	glBindBuffer (GL_ARRAY_BUFFER, vbo);
	glBufferData (GL_ARRAY_BUFFER, point_count * 3 * sizeof (float), points, GL_STATIC_DRAW);
  
	unsigned int vao = 0;
	glGenVertexArrays (1, &vao);
	glBindVertexArray (vao);
	glEnableVertexAttribArray (0);
	glBindBuffer (GL_ARRAY_BUFFER, vbo);
	glVertexAttribPointer (0, 3, GL_FLOAT, GL_FALSE, 0, NULL);
/*-----------------------------------------------------------------------------\
|                                    CUBE                                      |
\---------------------------------------------------------------------------- */
	float cube_points[] = {
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
	unsigned int cube_vbo = 0;
	glGenBuffers (1, &cube_vbo);
	glBindBuffer (GL_ARRAY_BUFFER, cube_vbo);
	glBufferData (
		GL_ARRAY_BUFFER, 3 * 36 * sizeof (float), &cube_points, GL_STATIC_DRAW
	);

	unsigned int cube_vao = 0;
	glGenVertexArrays (1, &cube_vao);
	glBindVertexArray (cube_vao);
	glEnableVertexAttribArray (0);
	glBindBuffer (GL_ARRAY_BUFFER, cube_vbo);
	glVertexAttribPointer (0, 3, GL_FLOAT, GL_FALSE, 0, NULL);
/*-----------------------------------------------------------------------------\
|                                   CUBEMAP                                    |
\---------------------------------------------------------------------------- */
	if (GLEW_ARB_texture_cube_map) {
		//printf ("cube map ext \"glGenProgramsARB\" found\n");
		gl_log ("ERROR: cube map ext \"glGenProgramsARB\" found\n");
	} else {
		gl_log ("ERROR: cube map ext not found\n");
		fprintf (stderr, "ERROR: cube map ext not found\n");
		return 1;
	}

	glEnable (GL_TEXTURE_CUBE_MAP_SEAMLESS);
	unsigned int cube_tex = 0;
	load_cube_map (
		"posx.jpg",
		"negx.jpg",
		"posy.jpg",
		"negy.jpg",
		"posz.jpg",
		"negz.jpg",
		cube_tex
	);


	unsigned int cvs = glCreateShader (GL_VERTEX_SHADER);
	char cvertex_shader[MAX_SHADER_LEN];
	assert (parse_file_into_str ("cube_vs.glsl", cvertex_shader, MAX_SHADER_LEN));
	const GLchar* sptr = cvertex_shader;
	glShaderSource (cvs, 1, &sptr, NULL);
	glCompileShader (cvs);
	// check for compile errors
	int params = -1;
	glGetShaderiv (cvs, GL_COMPILE_STATUS, &params);
	if (GL_TRUE != params) {
		fprintf (stderr, "ERROR: GL shader index %i did not compile\n", cvs);
		_print_shader_info_log (cvs);
		return false; // or exit or something
	}
	unsigned int cfs = glCreateShader (GL_FRAGMENT_SHADER);
	char cfragment_shader[MAX_SHADER_LEN];
	assert (parse_file_into_str ("cube_fs.glsl", cfragment_shader, MAX_SHADER_LEN));
	sptr = cfragment_shader;
	glShaderSource (cfs, 1, &sptr, NULL);
	glCompileShader (cfs);
	// check for compile errors
	glGetShaderiv (cfs, GL_COMPILE_STATUS, &params);
	if (GL_TRUE != params) {
		fprintf (stderr, "ERROR: GL shader index %i did not compile\n", cfs);
		_print_shader_info_log (cfs);
		return false; // or exit or something
	}

	unsigned int cshader_programme = glCreateProgram ();
	glAttachShader (cshader_programme, cfs);
	glAttachShader (cshader_programme, cvs);
	glLinkProgram (cshader_programme);

	int cV_loc = glGetUniformLocation (cshader_programme, "V");
	int cP_loc = glGetUniformLocation (cshader_programme, "P");

	// check if link was successful
	params = -1;
	glGetProgramiv (cshader_programme, GL_LINK_STATUS, &params);
	if (GL_TRUE != params) {
		fprintf (
			stderr,
			"ERROR: could not link shader programme GL index %i\n",
			cshader_programme
		);
		_print_programme_info_log (cshader_programme);
		return false;
	}
	glValidateProgram (cshader_programme);
  params = -1;
  glGetProgramiv (cshader_programme, GL_VALIDATE_STATUS, &params);
  if (GL_TRUE != params) {
    fprintf (stderr, "program %i GL_VALIDATE_STATUS = GL_FALSE\n", cshader_programme);
    return false;
  }
  gl_log ("program %i GL_VALIDATE_STATUS = GL_TRUE\n", cshader_programme);


/*-----------------------------------------------------------------------------\
|                                   SHADERS                                    |
\---------------------------------------------------------------------------- */
	unsigned int vs = glCreateShader (GL_VERTEX_SHADER);
	char vertex_shader[MAX_SHADER_LEN];
	assert (parse_file_into_str (VS_FILENAME, vertex_shader, MAX_SHADER_LEN));
	sptr = vertex_shader;
	glShaderSource (vs, 1, &sptr, NULL);
	glCompileShader (vs);
	// check for compile errors
	params = -1;
	glGetShaderiv (vs, GL_COMPILE_STATUS, &params);
	if (GL_TRUE != params) {
		fprintf (stderr, "ERROR: GL shader index %i did not compile\n", vs);
		_print_shader_info_log (vs);
		return false; // or exit or something
	}
	unsigned int fs = glCreateShader (GL_FRAGMENT_SHADER);
	char fragment_shader[MAX_SHADER_LEN];
	assert (parse_file_into_str (FS_FILENAME, fragment_shader, MAX_SHADER_LEN));
	sptr = fragment_shader;
	glShaderSource (fs, 1, &sptr, NULL);
	glCompileShader (fs);
	// check for compile errors
	glGetShaderiv (fs, GL_COMPILE_STATUS, &params);
	if (GL_TRUE != params) {
		fprintf (stderr, "ERROR: GL shader index %i did not compile\n", fs);
		_print_shader_info_log (fs);
		return false; // or exit or something
	}

	unsigned int shader_programme = glCreateProgram ();
	glAttachShader (shader_programme, fs);
	glAttachShader (shader_programme, vs);
	glLinkProgram (shader_programme);

	int M_loc = glGetUniformLocation (shader_programme, "M");
	int V_loc = glGetUniformLocation (shader_programme, "V");
	int P_loc = glGetUniformLocation (shader_programme, "P");
	int t_loc = glGetUniformLocation (shader_programme, "t");
	//assert (M_loc > -1 && V_loc > -1 && P_loc > -1 && t_loc > -1);

	// check if link was successful
	params = -1;
	glGetProgramiv (shader_programme, GL_LINK_STATUS, &params);
	if (GL_TRUE != params) {
		fprintf (
			stderr,
			"ERROR: could not link shader programme GL index %i\n",
			shader_programme
		);
		_print_programme_info_log (shader_programme);
		return false;
	}
	glValidateProgram (shader_programme);
  params = -1;
  glGetProgramiv (shader_programme, GL_VALIDATE_STATUS, &params);
  char value[32];
  if (GL_TRUE != params) {
    fprintf (stderr, "program %i GL_VALIDATE_STATUS = GL_FALSE\n", shader_programme);
    return false;
  }
  gl_log ("program %i GL_VALIDATE_STATUS = GL_TRUE\n", shader_programme);

/*-----------------------------------------------------------------------------\
|                                   CAMERA                                     |
\---------------------------------------------------------------------------- */
	mat4 V = look_at (
		vec3 (0.0f, 5.0f, -10.0f),
		vec3 (0.0, 0.0, 0.0),
		normalise (vec3 (0.0, 1.0, 2.0))
	);
	mat4 P = perspective (
		67.0f, (float)g_gl_width / (float)g_gl_height, 0.1f, 100.0f
	);
	glUseProgram (shader_programme);
	glUniformMatrix4fv (V_loc, 1, GL_FALSE, V.m);
	glUniformMatrix4fv (P_loc, 1, GL_FALSE, P.m);
	glUseProgram (cshader_programme);
	glUniformMatrix4fv (cV_loc, 1, GL_FALSE, V.m);
	glUniformMatrix4fv (cP_loc, 1, GL_FALSE, P.m);

	mat4 M[49];
	int i = 0;
	for (int x = -1; x <= 1; x+= 2) {
		for (int z = -1; z <= 1; z+= 2) {
			M[i++] = translate (identity_mat4 (), vec3 (x, 0.0f, z));
		}
	}
/*-----------------------------------------------------------------------------\
|                                  MAIN LOOP                                   |
\---------------------------------------------------------------------------- */

	glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	

	//glPolygonMode(GL_FRONT, GL_LINE);
	glViewport (0, 0, g_gl_width, g_gl_height);
	glClearColor (0.6, 0.2, 0.2, 1.0);
	double last_time = glfwGetTime ();
	while (!glfwWindowShouldClose (window)) {
		_update_fps_counter (window);
		double current_seconds = glfwGetTime ();
		double elapsed_time = current_seconds - last_time;
		last_time = current_seconds;
		

		glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		glDisable (GL_BLEND);
		glDepthMask (GL_FALSE);
		glBindTexture (GL_TEXTURE_CUBE_MAP, cube_tex);
		glUseProgram (cshader_programme);
		glBindVertexArray (cube_vao);
		glDrawArrays (GL_TRIANGLES, 0, 36);
		glDepthMask (GL_TRUE);

		//glEnable (GL_BLEND);
		glBindTexture (GL_TEXTURE_CUBE_MAP, cube_tex);
		glUseProgram (shader_programme);
		glUniform1f (t_loc, (GLfloat)current_seconds);
		glBindVertexArray (vao);
		for (int i = 0; i < 9; i++) {
			glUniformMatrix4fv (M_loc, 1, GL_FALSE, M[i].m);
			glDrawArrays (GL_TRIANGLES, 0, point_count);
		}

		if (dump_video) {
		  // elapsed_time is seconds since last loop iteration
		  video_timer += elapsed_time;
		  video_dump_timer += elapsed_time;
		  // only record 10s of video, then quit
		  if (video_timer > 10.0) {
		    break;
		  }
		}


		glfwPollEvents ();
		glfwSwapBuffers (window);

		if (dump_video) { // check if recording mode is enabled
			while (video_dump_timer > frame_time) {
				grab_video_frame (); // 25 Hz so grab a frame
				video_dump_timer -= frame_time;
			}
		}

		if (GLFW_PRESS == glfwGetKey (window, GLFW_KEY_ESCAPE)) {
			glfwSetWindowShouldClose (window, 1);
		}
		if (GLFW_PRESS == glfwGetKey (window, GLFW_KEY_F11)) {
			screenshot ();
		}
	}

	if (dump_video) {
		dump_video_frames ();
	}

  return 0;
}

