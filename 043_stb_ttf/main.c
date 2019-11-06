// Test Program for Font atlas and string geometry generator
// Based on Sean Barret's stb_truetype.h library
// Anton Gerdelan - 3 May 2018 <antonofnote@gmail.com>
// baked fonts
// C99
// Licence: GNU GPL v3
//
// Compile- Linux:
// gcc main.c font.c -g -DGLEW_STATIC ../common/lin64/libGLEW.a ../common/lin64/libglfw3.a -I ../common/include/  -lm -lGL -lX11 -lXxf86vm -lpthread -ldl -lXrandr -lXinerama -lXcursor -lXi

#include "font.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <stdbool.h>
#include <stdio.h>

#define FONT_FILE "NotoSans-Regular.ttf"
#define THAI_FONT_FILE "NotoSansThai-Regular.ttf"
#define CJKFONT_FILE "NotoSansCJK-Regular.ttc"
#define FONT_SCALE_PX 40.0
#define ATLAS_DIMS 2048
#define OVERSAMPLING 2

#define WIN_DIMS 1024
GLFWwindow *window;

void init_gl() {
	if ( !glfwInit() ) {
		fprintf( stderr, "ERROR: could not start GLFW3\n" );
		assert( false );
	}
	glfwWindowHint( GLFW_CONTEXT_VERSION_MAJOR, 4 );
	glfwWindowHint( GLFW_CONTEXT_VERSION_MINOR, 1 );
	glfwWindowHint( GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE );
	glfwWindowHint( GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE );
	window = glfwCreateWindow( WIN_DIMS, WIN_DIMS, "stb_truetype test", NULL, NULL );
	assert( window );
	glfwMakeContextCurrent( window );
	glewExperimental = GL_TRUE;
	glewInit();
	printf( "Renderer: %s\n", glGetString( GL_RENDERER ) );
	printf( "OpenGL version supported %s\n", glGetString( GL_VERSION ) );
}

int main() {

	init_gl();

	font_t my_font;
	{ // set up font and atlas texture
		load_font( FONT_FILE, FONT_SCALE_PX, ATLAS_DIMS, ATLAS_DIMS, OVERSAMPLING, &my_font );
		pack_thai( THAI_FONT_FILE, &my_font );
		pack_cjk( CJKFONT_FILE, &my_font );
		finalise_font( &my_font );
		stbi_write_png( "test_adv_atlas.png", ATLAS_DIMS, ATLAS_DIMS, 1, my_font.atlas_image, ATLAS_DIMS );
		create_opengl_font_atlas( &my_font );
	}

	GLuint program = 0;
	{ // shaders for rendering text
		const char *vertex_shader_str = "#version 410\n"
																		"layout(location=0) in vec2 apos;\n"
																		"layout(location=1) in vec2 ast;\n"
																		"out vec2 vst;\n"
																		"void main() {\n"
																		"  vst = ast;\n"
																		"  gl_Position = vec4( apos, 0.0, 1.0 );\n"
																		"}";
		const char *fragment_shader_str = "#version 410\n"
																			"in vec2 vst;\n"
																			"uniform sampler2D uatlas;\n"
																			"uniform vec4 utext_colour;\n"
																			"out vec4 frag_colour;\n"
																			"void main() {\n"
																			"  float texel = texture( uatlas, vst ).r;\n"
																			"  frag_colour = vec4( texel ) * utext_colour;\n"
																			"}";
		GLuint vs = glCreateShader( GL_VERTEX_SHADER );
		GLuint fs = glCreateShader( GL_FRAGMENT_SHADER );
		glShaderSource( vs, 1, (const char **)&vertex_shader_str, NULL );
		glShaderSource( fs, 1, (const char **)&fragment_shader_str, NULL );
		glCompileShader( vs );
		glCompileShader( fs );
		program = glCreateProgram();
		glAttachShader( program, fs );
		glAttachShader( program, vs );
		glLinkProgram( program );
	}
	GLint utext_colour_loc = glGetUniformLocation( program, "utext_colour" );

	GLuint vao = 0, vbo = 0;
	glGenVertexArrays( 1, &vao );
	glGenBuffers( 1, &vbo );

	glBindVertexArray( vao );
	glBindBuffer( GL_ARRAY_BUFFER, vbo );
	{
		glEnableVertexAttribArray( 0 );
		glEnableVertexAttribArray( 1 );

		GLsizei stride = sizeof( float ) * 4;
		GLintptr vertex_texcoord_offset = 2 * sizeof( float );
		glVertexAttribPointer( 0, 2, GL_FLOAT, GL_FALSE, stride, NULL );
		glVertexAttribPointer( 1, 2, GL_FLOAT, GL_FALSE, stride, (GLvoid *)vertex_texcoord_offset );
	}
	glBindBuffer( GL_ARRAY_BUFFER, 0 );
	glBindVertexArray( 0 );

	const char *string = "Fáilte!\n/AW WA AV lj Avslutt //\nŁączenie...\n主機遊戲\nHálózati játék létrehozása\nアドレス\n ไทย "
											 "เริ่มเกมใหม่ ออก ตัวเลือก\n เป็นเจ้าภาพ ร่วมเล่น ยกเลิก\n เริ่ม ที่อยู่ IP กำลังเชื่อมต่อ...\nΣύνδεση...";
	int ncodepoints = update_string_geom( &my_font, -180, -170, string, vbo, 0.005 );
	int vcount = ncodepoints * 6;

	glEnable( GL_CULL_FACE ); // TODO(anton) fix winding
	glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
	glEnable( GL_BLEND );
	glDisable( GL_DEPTH_TEST );

	glClearColor( 0.2, 0.2, 0.2, 1.0 );

	while ( !glfwWindowShouldClose( window ) ) {
		glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
		glViewport( 0, 0, WIN_DIMS, WIN_DIMS );

		glProgramUniform4f( program, utext_colour_loc, 1.0, 1.0, 0.0, 1.0 );

		glUseProgram( program );
		glBindVertexArray( vao );
		glActiveTexture( GL_TEXTURE0 );
		glBindTexture( GL_TEXTURE_2D, my_font.atlas_texture );

		glDrawArrays( GL_TRIANGLES, 0, vcount );

		glBindTexture( GL_TEXTURE_2D, 0 );
		glBindVertexArray( 0 );
		glUseProgram( 0 );

		glfwPollEvents();
		glfwSwapBuffers( window );
	}

	free_font( &my_font );

	return 0;
}
