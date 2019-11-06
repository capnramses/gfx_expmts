#define STB_IMAGE_IMPLEMENTATION
#include "stb/stb_image.h"
#include "camera.h"
#include "mesh.h"
#include "text.h"
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <stdio.h>
#include <stdbool.h>
#define VP_WIDTH 1136
#define VP_HEIGHT 640

GLFWwindow* g_win;
GLuint g_vao_tri;
GLuint g_shadprog;
GLuint g_tex_cube;
GLint P_loc, V_loc, M_loc;
Mesh g_mesh;
mat4 g_globe_M;
float g_globe_yrot;
Font g_font;
GLuint first_string_vp_vbo, first_string_vt_vbo, first_string_vao;
int first_string_points = 0;

void init_gl(){
	{
		glfwInit();
		glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
		glfwWindowHint(GLFW_SRGB_CAPABLE, GLFW_TRUE);
		glfwWindowHint (GLFW_SAMPLES, 16);
		{ // NOTE OPENGL ES 3 INVOKED HERE
			glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
			glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
			glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
		}
		g_win = glfwCreateWindow(VP_WIDTH, VP_HEIGHT, "VR Demo", NULL, NULL);
		glfwMakeContextCurrent(g_win);
		glewExperimental = GL_TRUE;
		glewInit ();
	}
	{
		const GLubyte* renderer = glGetString (GL_RENDERER); // get renderer string
		const GLubyte* version = glGetString (GL_VERSION); // version as a string
		printf ("Renderer: %s\n", renderer);
		printf ("OpenGL version supported %s\n", version);
	}
	{
		glEnable(GL_DEPTH_TEST);
		glDepthFunc(GL_LESS);
		glClearColor(0.3f, 0.3f, 0.3f, 1.0f);
		glEnable (GL_CULL_FACE);
		glCullFace (GL_BACK);
		glFrontFace (GL_CCW);
		glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	}
}

void free_gl(){ glfwTerminate(); }

void init_geom() {
	/*float points[] = {
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
	};*/
	g_mesh = load_ascii_stl ("meshes/sphere.stl");
	GLuint vbo = 0;
	glGenBuffers (1, &vbo);
	glBindBuffer (GL_ARRAY_BUFFER, vbo);
	glBufferData (GL_ARRAY_BUFFER, g_mesh.vcount * 3 * sizeof (float), g_mesh.vps, GL_STATIC_DRAW);
	glGenVertexArrays (1, &g_vao_tri);
	glBindVertexArray (g_vao_tri);
	glEnableVertexAttribArray (0);
	glBindBuffer (GL_ARRAY_BUFFER, vbo);
	glVertexAttribPointer (0, 3, GL_FLOAT, GL_FALSE, 0, NULL);
}

void compile_shader(GLuint shdr_idx){
	glCompileShader (shdr_idx);
	int params = -1;
	glGetShaderiv (shdr_idx, GL_COMPILE_STATUS, &params);
	if (GL_TRUE != params) {
		fprintf (stderr, "ERROR: GL shader index %i did not compile\n", shdr_idx);
		int max_length = 2048;
		int actual_length = 0;
		char log[2048];
		glGetShaderInfoLog (shdr_idx, max_length, &actual_length, log);
		printf ("shader info log for GL index %u:\n%s\n", shdr_idx, log);
	}
}

void link_program(GLuint prog_idx){
	glLinkProgram (prog_idx);
	int params = -1;
	glGetProgramiv (prog_idx, GL_LINK_STATUS, &params);
	if (GL_TRUE != params) {
		fprintf (stderr, "ERROR: could not link shader programme GL index %u\n", prog_idx);
		int max_length = 2048;
		int actual_length = 0;
		char log[2048];
		glGetProgramInfoLog (prog_idx, max_length, &actual_length, log);
		printf ("program info log for GL index %u:\n%s", prog_idx, log);
	}
}

void init_shaders() {
	const char* vertex_shader =
		"#version 300 es\n"
		"in vec3 vp;"
		"uniform mat4 P, V, M;"
		"out vec3 texcoords;"
		"void main(){"
		"  texcoords = vec3(vp.x, vp.y, vp.z);"
		"  gl_Position = P * V * M * vec4(vp * 0.1, 1.0);"
		"}";
	const char* fragment_shader =
		"#version 300 es\n"
		"precision mediump float;" // need this
		"in vec3 texcoords;"
		"uniform samplerCube cube_texture;"
		"out vec4 frag_colour;"
		"void main(){"
		"  frag_colour = texture(cube_texture, texcoords);"
		"}";
	GLuint vs = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vs, 1, &vertex_shader, NULL);
	compile_shader(vs);
	GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource (fs, 1, &fragment_shader, NULL);
	compile_shader(fs);
	g_shadprog = glCreateProgram();
	glAttachShader(g_shadprog, fs);
	glAttachShader(g_shadprog, vs);
	link_program(g_shadprog);
	P_loc = glGetUniformLocation(g_shadprog, "P");
	assert(P_loc > -1);
	V_loc = glGetUniformLocation(g_shadprog, "V");
	assert(V_loc > -1);
	M_loc = glGetUniformLocation(g_shadprog, "M");
	assert(V_loc > -1);
}

void draw_scene(Camera cam){
	glUseProgram (g_shadprog);
	glUniformMatrix4fv(P_loc, 1, GL_FALSE, cam.P.m);
	glUniformMatrix4fv(V_loc, 1, GL_FALSE, cam.V.m);
	glUniformMatrix4fv(M_loc, 1, GL_FALSE, g_globe_M.m);
	glBindVertexArray (g_vao_tri);
	glActiveTexture (GL_TEXTURE0);
	glBindTexture (GL_TEXTURE_CUBE_MAP, g_tex_cube);
	glDrawArrays (GL_TRIANGLES, 0, g_mesh.vcount);
	{
		glActiveTexture (GL_TEXTURE0);
		glBindTexture (GL_TEXTURE_2D, g_font.texture);
		glUseProgram (g_font.shader_prog);
		glDisable (GL_DEPTH_TEST);
		glEnable (GL_BLEND);
		glBindVertexArray (first_string_vao);
		glUniform4f (g_font.sp_text_colour_loc, 1.0, 0.0, 1.0, 1.0);
		glDrawArrays (GL_TRIANGLES, 0, first_string_points);
		glEnable (GL_DEPTH_TEST);
		glDisable (GL_BLEND);
	}
}

bool load_cube_map_side(GLuint texture, GLenum side_target, const char* file_name){
  glBindTexture (GL_TEXTURE_CUBE_MAP, texture);
  int x, y, n;
  int force_channels = 4;
  unsigned char*  image_data = stbi_load (file_name, &x, &y, &n, force_channels);
  if (!image_data) {
    fprintf (stderr, "ERROR: could not load %s\n", file_name);
    return false;
  }
  if ((x & (x - 1)) != 0 || (y & (y - 1)) != 0) {
    fprintf (stderr, "WARNING: image %s is not power-of-2 dimensions\n", file_name);
  }
  glTexImage2D(side_target, 0, GL_RGBA, x, y, 0, GL_RGBA, GL_UNSIGNED_BYTE, image_data);
  free (image_data);
  return true;
}

void create_cube_map(
	const char* front,
	const char* back,
	const char* top,
  const char* bottom,
  const char* left,
  const char* right,
  GLuint* tex_cube){
  glActiveTexture(GL_TEXTURE0);
  glGenTextures(1, tex_cube);
  load_cube_map_side(*tex_cube, GL_TEXTURE_CUBE_MAP_NEGATIVE_Z, front);
  load_cube_map_side(*tex_cube, GL_TEXTURE_CUBE_MAP_POSITIVE_Z, back);
  load_cube_map_side(*tex_cube, GL_TEXTURE_CUBE_MAP_POSITIVE_Y, top);
  load_cube_map_side(*tex_cube, GL_TEXTURE_CUBE_MAP_NEGATIVE_Y, bottom);
  load_cube_map_side(*tex_cube, GL_TEXTURE_CUBE_MAP_NEGATIVE_X, left);
  load_cube_map_side(*tex_cube, GL_TEXTURE_CUBE_MAP_POSITIVE_X, right);
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
}

int main(){
	init_gl();
	init_geom();
	Camera cam = default_cam();
	g_globe_M = identity_mat4();
	init_shaders();
	fprintf(stderr, "loading cube textures...\n");
	create_cube_map(
		"textures/negz.jpg",
		"textures/posz.jpg",
		"textures/posy.jpg",
		"textures/negy.jpg",
		"textures/negx.jpg",
		"textures/posx.jpg",
		&g_tex_cube);
	fprintf(stderr, "cube loaded\n");
	{
		g_font = load_font("atlas.png", "atlas.meta");
		glGenBuffers (1, &first_string_vp_vbo);
		glGenBuffers (1, &first_string_vt_vbo);
		float x_pos = -0.75f;
		float y_pos = 0.2f;
		float pixel_scale = 190.0f;
		const char first_str[] = "Hello World VR!";
		text_to_vbo (
			first_str,
			g_font,
			VP_WIDTH, VP_HEIGHT,
			x_pos, y_pos,
			pixel_scale,
			&first_string_vp_vbo,
			&first_string_vt_vbo,
			&first_string_points
		);
		glGenVertexArrays (1, &first_string_vao);
		glBindVertexArray (first_string_vao);
		glBindBuffer (GL_ARRAY_BUFFER, first_string_vp_vbo);
		glVertexAttribPointer (0, 2, GL_FLOAT, GL_FALSE, 0, NULL);
		glEnableVertexAttribArray (0);
		glBindBuffer (GL_ARRAY_BUFFER, first_string_vt_vbo);
		glVertexAttribPointer (1, 2, GL_FLOAT, GL_FALSE, 0, NULL);
		glEnableVertexAttribArray (1);
	}
	while(!glfwWindowShouldClose(g_win)) {
		{
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
			glViewport(0, 0, VP_WIDTH / 2, VP_HEIGHT);
			draw_scene(cam);
			glViewport(VP_WIDTH / 2, 0, VP_WIDTH / 2, VP_HEIGHT);
			draw_scene(cam);
			glfwSwapBuffers(g_win);
		}
		{
			glfwPollEvents();
			if (GLFW_PRESS == glfwGetKey(g_win, GLFW_KEY_ESCAPE)) {
				glfwSetWindowShouldClose(g_win, 1);
			}
			/*if (GLFW_PRESS == glfwGetKey(g_win, GLFW_KEY_W)) {
				vec3 mv;
				mv.x = 0.0;
				mv.y = 0.0f;
				mv.z = -0.1f;
				cam_move(&cam, mv);
			}
			if (GLFW_PRESS == glfwGetKey(g_win, GLFW_KEY_S)) {
				vec3 mv;
				mv.x = 0.0;
				mv.y = 0.0f;
				mv.z = 0.1f;
				cam_move(&cam, mv);
			}*/
			if (GLFW_PRESS == glfwGetKey(g_win, GLFW_KEY_A)) {
				g_globe_yrot += 1.0f;
				g_globe_M = rot_y_deg_mat4(g_globe_yrot);
			}
			if (GLFW_PRESS == glfwGetKey(g_win, GLFW_KEY_D)) {
				g_globe_yrot -= 1.0f;
				g_globe_M = rot_y_deg_mat4(g_globe_yrot);
			}
		}
	}
	free_gl();
	return 0;
}

