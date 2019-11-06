#pragma once

#include "includes.h"

#include "GL/glew.h"
#include "GLFW/glfw3.h"
//#include "game_utils.h"

#define INIT_WIN_WIDTH 1024
#define INIT_WIN_HEIGHT 768

typedef struct Gfx {
	GLFWwindow *window;
	GLuint gpu_timer_query;
	float ssaa_factor;
	float effective_ssaa_factor;
	int win_width, win_height;
	int fb_width, fb_height;
	int msaa;
	bool fullscreen;
	bool fb_resized;
	bool wireframe; // debug mode accessible from console
	bool show_fps;
	bool gl_was_init;
	GLuint ss_quad_vao, ss_quad_vbo; // full-screen quad for general/common use
} Gfx;

typedef struct Texture_Props {
	int force_chans;
	bool clamp;
	bool mipmaps;
	bool linear;
	int active_slot;
} Texture_Props;

bool init_gl();

void start_gpu_timer();

// this function is blocking. delay call until next frame to avoid stall time
// TODO note could also measure latency cpu->gpu commands with example 3 at:
// https://www.opengl.org/registry/specs/ARB/timer_query.txt
double stop_gpu_timer_ms();

// TODO return a Texture_Meta
unsigned int load_texture( const char *file_name, Texture_Props props );

typedef struct Framebuffer_Props {
	int width;
	int height;
	bool linear;
	bool hdr32;
	bool has_second_tex;
	bool has_depth_tex32;
	bool ssaa_affects;
	float x_scale, y_scale; // used in recreation. don't need to specify
} Framebuffer_Props;

typedef struct Framebuffer_Meta {
	Framebuffer_Props props;
	GLuint fb, rb;
	GLuint texture, depth_texture, second_texture;
} Framebuffer_Meta;

bool init_framebuffer( Framebuffer_Meta *fb_meta, Framebuffer_Props props );
bool rebuild_framebuffer( Framebuffer_Meta *fb_meta );
void bind_framebuffer( Framebuffer_Meta *fb_meta );

extern Gfx g_gfx;
