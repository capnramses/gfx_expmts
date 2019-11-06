//
// OpenGL stuff
// first v. 2016 by Dr Anton Gerdelan
// C99
//

#include "gl_utils.h"

#define glog printf
#define glog_err printf

Gfx g_gfx;

static void _glfw_error_callback( int error, const char *description ) {
	glog_err( "GLFW ERROR: code %i msg: %s\n", error, description );
}

static void _glfw_window_size_callback( GLFWwindow *window, int width,
																				int height ) {
	glog( "window dims changed to %ix%i\n", width, height );
	g_gfx.win_width = width;
	g_gfx.win_height = height;
}

static void _glfw_fb_size_callback( GLFWwindow *window, int width, int height ) {
	glog( "framebuffer dims changed to %ix%i\n", width, height );
	g_gfx.fb_width = width;
	g_gfx.fb_height = height;
	g_gfx.fb_resized = true; // flag to tell matrices and framebuffers to resize
}

static void _init_ss_quad() {
	GLfloat ss_quad_pos[] = { -1.0, -1.0, -1.0, 1.0, 1.0, -1.0, 1.0, 1.0 };
	glGenVertexArrays( 1, &g_gfx.ss_quad_vao );
	glBindVertexArray( g_gfx.ss_quad_vao );
	glGenBuffers( 1, &g_gfx.ss_quad_vbo );
	glBindBuffer( GL_ARRAY_BUFFER, g_gfx.ss_quad_vbo );
	glBufferData( GL_ARRAY_BUFFER, sizeof( GLfloat ) * 8, ss_quad_pos,
								GL_STATIC_DRAW );
	glVertexAttribPointer( 0, 2, GL_FLOAT, GL_FALSE, 0, NULL );
	glEnableVertexAttribArray( 0 );
}

static bool _extension_checks() {
	bool has_all_vital = true;
	if ( GLEW_ARB_debug_output ) { // glcapsviewer reports 47% support
		glog( "ARB_debug_output = yes\n" );
	} else {
		glog( "ARB_debug_output = no\n" );
	}
	if ( GLEW_ARB_timer_query ) { // (core since GL 3.3)
		glog( "ARB_timer_query = yes\n" );
		glGenQueries( 1, &g_gfx.gpu_timer_query );
	} else {
		glog( "ARB_timer_query = no\n" );
	}
	if ( GLEW_ARB_framebuffer_object ) { // glcapsviewer gives 89% support
		glog( "ARB_framebuffer_object = yes\n" );
	} else {
		glog( "ARB_framebuffer_object = no\n" );
		has_all_vital = false;
	}
	if ( GLEW_ARB_vertex_array_object ) { // 87% support
		glog( "ARB_vertex_array_object = yes\n" );
	} else {
		glog( "ARB_vertex_array_object = no\n" );
		has_all_vital = false;
	}
	if ( GLEW_ARB_uniform_buffer_object ) { // 80%
		glog( "ARB_uniform_buffer_object = yes\n" );
	} else {
		glog( "ARB_uniform_buffer_object = no\n" );
		has_all_vital = false;
	}
	if ( GLEW_ARB_texture_float ) { // 94% support (core since GL 3.0) 2004 works
																	// with opengl 1.5
		glog( "ARB_texture_float = yes\n" );
	} else {
		glog( "ARB_texture_float = no\n" );
		// has_all_vital = false; // APPLE reports it doesn't have this
	}
	return has_all_vital;
}

bool init_gl() {
	assert( !g_gfx.gl_was_init );
	glfwSetErrorCallback( _glfw_error_callback );
	if ( !glfwInit() ) {
		glog_err( "ERROR: starting GLFW\n" );
		return false;
	}

#ifdef APPLE
	glfwWindowHint( GLFW_CONTEXT_VERSION_MAJOR, 3 );
	glfwWindowHint( GLFW_CONTEXT_VERSION_MINOR, 2 );
	glfwWindowHint( GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE );
	glfwWindowHint( GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE );
#endif
#ifdef CRAPPY_LAPTOP
	printf( "crappy laptop mode engaged\n" );
	glfwWindowHint( GLFW_CONTEXT_VERSION_MAJOR, 2 );
	glfwWindowHint( GLFW_CONTEXT_VERSION_MINOR, 1 );
#endif
	// (note on mesa with "vblank_mode=0 ./castle")
	// 16x=7.7ms (full size also about 0.2ms slower than 0)
	// 8x=7.7ms
	// 4x=7.7ms
	// 0x=7.5ms (full size =14ms)
	g_gfx.msaa = 16;
	glfwWindowHint( GLFW_SAMPLES, g_gfx.msaa ); // MSAA
	g_gfx.window = glfwCreateWindow( INIT_WIN_WIDTH, INIT_WIN_HEIGHT,
																	 "BSP demo -- Anton Gerdelan", NULL, NULL );
	assert( g_gfx.window );
	glfwSetWindowSizeCallback( g_gfx.window, _glfw_window_size_callback );
	glfwSetFramebufferSizeCallback( g_gfx.window, _glfw_fb_size_callback );
	// glfwSetKeyCallback( g_gfx.window, key_callback );
	// glfwSetMouseButtonCallback( g_gfx.window, mouse_button_callback );
	glfwMakeContextCurrent( g_gfx.window );
	glfwGetWindowSize( g_gfx.window, &g_gfx.win_width, &g_gfx.win_height );
	glfwGetFramebufferSize( g_gfx.window, &g_gfx.fb_width, &g_gfx.fb_height );
	glewExperimental = GL_TRUE;
	glewInit();
	const GLubyte *renderer = glGetString( GL_RENDERER );
	const GLubyte *version = glGetString( GL_VERSION );
	glog( "Renderer: %s\n", renderer );
	glog( "OpenGL version supported %s\n", version );
	if ( !_extension_checks() ) {
		return false;
	}
	glDepthFunc( GL_LESS );
	glEnable( GL_DEPTH_TEST );
	glDisable( GL_CULL_FACE );
	// glCullFace( GL_BACK );
	// glFrontFace( GL_CCW );
	// glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
	// glClearColor(0.543, 0.899, 0.970, 1.0f); //lblue
	// glClearColor(0.745, 0.792, 0.920, 1.0f); //lgrey
	_init_ss_quad();
	// TODO -- only in debug/internal build?
	g_gfx.show_fps = true;
	g_gfx.gl_was_init = true;
	g_gfx.ssaa_factor = 1.0f;
	g_gfx.effective_ssaa_factor = g_gfx.ssaa_factor;
	return true;
}

void start_gpu_timer() {
	assert( g_gfx.gl_was_init );
	if ( !GLEW_ARB_timer_query ) {
		return;
	}
	if ( 0 == g_gfx.gpu_timer_query ) {
		return;
	}

	if ( GLEW_ARB_timer_query ) {
		glBeginQuery( GL_TIME_ELAPSED, g_gfx.gpu_timer_query );
	}
}

// NOTE this causes a driver memory leak detectable by valgrind and sanitisers
double stop_gpu_timer_ms() {
	assert( g_gfx.gl_was_init );
	if ( !GLEW_ARB_timer_query ) {
		return 0.0;
	}
	if ( 0 == g_gfx.gpu_timer_query ) {
		return 0.0;
	}

	GLuint64 elapsed_ns = 0;
	if ( GLEW_ARB_timer_query ) {
		glEndQuery( GL_TIME_ELAPSED );
		GLint available = 0;
		while ( available != 1 ) {
			glGetQueryObjectiv( g_gfx.gpu_timer_query, GL_QUERY_RESULT_AVAILABLE,
													&available ); // valgrind: "Conditional jump or move
																				// depends on uninitialised value(s)"
		}
		glGetQueryObjectui64v( g_gfx.gpu_timer_query, GL_QUERY_RESULT, &elapsed_ns );
	}
	double ms = (double)elapsed_ns / 1000000.0;
	return ms;
}

bool init_framebuffer( Framebuffer_Meta *fb_meta, Framebuffer_Props props ) {
	assert( fb_meta );
	assert( props.width > 0 );
	assert( props.height > 0 );

	fb_meta->props = props;
	fb_meta->props.x_scale = (float)props.width / (float)g_gfx.fb_width;
	fb_meta->props.y_scale = (float)props.height / (float)g_gfx.fb_height;
	{
		glGenFramebuffers( 1, &fb_meta->fb );
		glGenTextures( 1, &fb_meta->texture );
		if ( props.has_depth_tex32 ) {
			glGenTextures( 1, &fb_meta->depth_texture );
		}
		if ( props.has_second_tex ) {
			glGenTextures( 1, &fb_meta->second_texture );
		} else {
			glGenRenderbuffers( 1, &fb_meta->rb );
		}
		if ( !rebuild_framebuffer( fb_meta ) ) {
			return false;
		}
	}

	return true;
}

// was ~1.5ms before fullscreen -> 1.4 without all the depth32 textures. -> 1.3 with
// /2 sized refraction map
bool rebuild_framebuffer( Framebuffer_Meta *fb_meta ) {
	{
		assert( fb_meta );
		assert( fb_meta->props.width > 0 );
		assert( fb_meta->props.height > 0 );
		assert( fb_meta->fb > 0 );
		assert( fb_meta->texture > 0 );
		assert( fb_meta->rb > 0 || fb_meta->depth_texture > 0 );
	}
	int w = fb_meta->props.width =
		(int)( (float)fb_meta->props.x_scale * (float)g_gfx.fb_width );
	int h = fb_meta->props.height =
		(int)( (float)fb_meta->props.y_scale * (float)g_gfx.fb_height );
	if ( fb_meta->props.ssaa_affects ) {
		w *= g_gfx.effective_ssaa_factor;
		h *= g_gfx.effective_ssaa_factor;
	}
	glBindFramebuffer( GL_FRAMEBUFFER, fb_meta->fb );
	{
		glBindTexture( GL_TEXTURE_2D, fb_meta->texture );
		if ( fb_meta->props.hdr32 ) {
			glTexImage2D( GL_TEXTURE_2D, 0, GL_RGB32F, w, h, 0, GL_RGB, GL_FLOAT, NULL );
		} else {
			glTexImage2D( GL_TEXTURE_2D, 0, GL_RGB, w, h, 0, GL_RGB, GL_UNSIGNED_BYTE,
										NULL );
		}
		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );
		if ( fb_meta->props.linear ) {
			glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
			glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
		} else {
			glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
			glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
		}
		glFramebufferTexture2D( GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
														fb_meta->texture, 0 );

		if ( fb_meta->props.has_second_tex ) {
			glBindTexture( GL_TEXTURE_2D, fb_meta->second_texture );
			if ( fb_meta->props.hdr32 ) {
				glTexImage2D( GL_TEXTURE_2D, 0, GL_RGB32F, w, h, 0, GL_RGB, GL_FLOAT,
											NULL );
			} else {
				glTexImage2D( GL_TEXTURE_2D, 0, GL_RGB, w, h, 0, GL_RGB, GL_UNSIGNED_BYTE,
											NULL );
			}
			glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
			glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );
			if ( fb_meta->props.linear ) {
				glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
				glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
			} else {
				glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
				glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
			}
			glFramebufferTexture2D( GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D,
															fb_meta->second_texture, 0 );
		}

		if ( fb_meta->props.has_depth_tex32 ) {
			glBindTexture( GL_TEXTURE_2D, fb_meta->depth_texture );
			glTexImage2D( GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32, w, h, 0,
										GL_DEPTH_COMPONENT, GL_FLOAT, NULL );
			glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
			glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );
			if ( fb_meta->props.linear ) {
				glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
				glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
			} else {
				glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
				glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
			}
			glFramebufferTexture2D( GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D,
															fb_meta->depth_texture, 0 );
		} else {
			glBindRenderbuffer( GL_RENDERBUFFER, fb_meta->rb );
			glRenderbufferStorage( GL_RENDERBUFFER, GL_DEPTH_COMPONENT, w, h );
			glFramebufferRenderbuffer( GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
																 GL_RENDERBUFFER, fb_meta->rb );
		}
		if ( fb_meta->props.has_second_tex ) {
			GLenum draw_bufs[] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };
			glDrawBuffers( 2, draw_bufs );
		} else {
			GLenum draw_bufs[] = { GL_COLOR_ATTACHMENT0 };
			glDrawBuffers( 1, draw_bufs );
		}
	}
	{
		GLenum status = glCheckFramebufferStatus( GL_FRAMEBUFFER );
		if ( GL_FRAMEBUFFER_COMPLETE != status ) {
			fprintf( stderr, "ERROR: incomplete framebuffer\n" );
			if ( GL_FRAMEBUFFER_UNDEFINED == status ) {
				fprintf( stderr, "GL_FRAMEBUFFER_UNDEFINED\n" );
			} else if ( GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT == status ) {
				fprintf( stderr, "GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT\n" );
			} else if ( GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT == status ) {
				fprintf( stderr, "GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT\n" );
			} else if ( GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER == status ) {
				fprintf( stderr, "GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER\n" );
			} else if ( GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER == status ) {
				fprintf( stderr, "GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER\n" );
			} else if ( GL_FRAMEBUFFER_UNSUPPORTED == status ) {
				fprintf( stderr, "GL_FRAMEBUFFER_UNSUPPORTED\n" );
			} else if ( GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE == status ) {
				fprintf( stderr, "GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE\n" );
			} else if ( GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS == status ) {
				fprintf( stderr, "GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS\n" );
			} else {
				fprintf( stderr, "unspecified error\n" );
			}
			return false;
		}
	}
	glBindFramebuffer( GL_FRAMEBUFFER, 0 );
	return true;
}

void bind_framebuffer( Framebuffer_Meta *fb_meta ) {
	if ( !fb_meta ) {
		glBindFramebuffer( GL_FRAMEBUFFER, 0 );
		glViewport( 0, 0, g_gfx.fb_width, g_gfx.fb_height );
		return;
	}
	glBindFramebuffer( GL_FRAMEBUFFER, fb_meta->fb );
	int w = fb_meta->props.width;
	int h = fb_meta->props.height;
	if ( fb_meta->props.ssaa_affects ) {
		w *= g_gfx.effective_ssaa_factor;
		h *= g_gfx.effective_ssaa_factor;
	}
	glViewport( 0, 0, w, h );
}
