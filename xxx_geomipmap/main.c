// terrain subdivision test
// anton gerdelan -- 10 jun 2017
// gcc main.c gl_utils.c ../common/lin64/libGLEW.a -lglfw -lGL -lm

// TODO
// * add skirts to hide seams/joins between LOD levels (or just put a big 2-tri plane underneath)
// * separate shaders for max/mid/min (possibly build render queues)
// * reverse orientation of every 2nd square's triangles
// * use index buffers -- probably will also fix seams if vbo is global

// NOTES
// * at the moment levels 1 and 2 may extend beyond level 0 at ends of map b/c
//   their squares are bigger and are not scaled down to match the real end with lod0

#include "gl_utils.h"
#include "linmath.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include <stdio.h>
#include <assert.h>

#define HMAP_IMG "hm2.png"
// horiz dims of node in world units
#define TERR_WIDTH_SCALE 800.0 / 512.0
// distance between entire range of height values in world units
#define TERR_HEIGHT_SCALE (1.0 / 255.0) * 100.0

#define NODES_ACROSS 8

// max quality shown under this dist
#define TERR_LOD0_DIST 150.0
#define TERR_LOD1_DIST 250.0

// if defined then add skirts of triangle to hide gaps between nodes of diff LODs
#define TERR_SKIRTS

typedef unsigned char byte;

typedef struct terrain_node_t {
	GLuint max_lod_vao, mid_lod_vao, min_lod_vao;
	GLuint max_lod_npoints, mid_lod_npoints, min_lod_npoints;
  vec2 mid_point; // xz height not considered yet
} terrain_node_t;

typedef struct terrain_t {
	// terrain nodes [across][down]
	terrain_node_t nodes[NODES_ACROSS][NODES_ACROSS];
	int hmap_cols, hmap_rows;
} terrain_t;

terrain_t terrain;

static void fill_buffer_lod( int lod_divisor,
  int fromx, int tox, int fromy, int toy,
  const byte* hmap_data, vec3 *buff, int *nverts ) {
  assert( hmap_data );
  assert( buff );
  assert( nverts );

  int buff_idx = 0;
  int x_ctr = 0, y_ctr = 0;
  for (int y_idx = fromy; y_idx < toy; y_idx += lod_divisor, y_ctr++ ){
    for (int x_idx = fromx; x_idx < tox; x_idx += lod_divisor, x_ctr++ ){
      int tl_idx = MIN( y_idx, terrain.hmap_rows - 1) * terrain.hmap_cols +
        MIN( x_idx, terrain.hmap_cols - 1);
      int tr_idx = MIN( y_idx, terrain.hmap_rows - 1) * terrain.hmap_cols +
        MIN( x_idx + lod_divisor, terrain.hmap_cols - 1);
      int bl_idx = MIN( y_idx + lod_divisor, terrain.hmap_rows - 1) * terrain.hmap_cols +
        MIN( x_idx, terrain.hmap_cols - 1);
      int br_idx = MIN( y_idx + lod_divisor, terrain.hmap_rows - 1) * terrain.hmap_cols +
        MIN( x_idx + lod_divisor, terrain.hmap_cols - 1);

      vec3 tl_pt = (vec3){
        x_idx * TERR_WIDTH_SCALE,
        hmap_data[tl_idx] * TERR_HEIGHT_SCALE,
        y_idx * TERR_WIDTH_SCALE };
      vec3 tr_pt = (vec3){
        (x_idx + lod_divisor) * TERR_WIDTH_SCALE,
        hmap_data[tr_idx] * TERR_HEIGHT_SCALE,
        y_idx * TERR_WIDTH_SCALE, };
      vec3 bl_pt = (vec3){
        x_idx * TERR_WIDTH_SCALE,
        hmap_data[bl_idx] * TERR_HEIGHT_SCALE,
        (y_idx+lod_divisor) * TERR_WIDTH_SCALE, };
      vec3 br_pt = (vec3){
        (x_idx + lod_divisor) * TERR_WIDTH_SCALE,
        hmap_data[br_idx] * TERR_HEIGHT_SCALE,
        (y_idx+lod_divisor) * TERR_WIDTH_SCALE };

      // (reverse triangles diagonal on every second square) 
      if (( y_ctr + x_ctr ) % 2 == 0 ) {
        buff[buff_idx++] = tl_pt;
        buff[buff_idx++] = bl_pt;
        buff[buff_idx++] = br_pt;
        buff[buff_idx++] = br_pt;
        buff[buff_idx++] = tr_pt;
        buff[buff_idx++] = tl_pt;
      } else {
        buff[buff_idx++] = tl_pt;
        buff[buff_idx++] = bl_pt;
        buff[buff_idx++] = tr_pt;
        buff[buff_idx++] = tr_pt;
        buff[buff_idx++] = bl_pt;
        buff[buff_idx++] = br_pt;
      } // endif

#ifdef TERR_SKIRTS
      const float skirt_length = 30.0f;
      // top row
     if (y_idx == fromy) {
        buff[buff_idx++] = tr_pt;
        buff[buff_idx++] = (vec3){.x=tr_pt.x, .y=tr_pt.y-skirt_length, .z=tr_pt.z};
        buff[buff_idx++] = (vec3){.x=tl_pt.x, .y=tl_pt.y-skirt_length, .z=tl_pt.z};
        buff[buff_idx++] = (vec3){.x=tl_pt.x, .y=tl_pt.y-skirt_length, .z=tl_pt.z};
        buff[buff_idx++] = tl_pt;
        buff[buff_idx++] = tr_pt;
      }
      // left col
      if (x_idx == fromx) {
        buff[buff_idx++] = tl_pt;
        buff[buff_idx++] = (vec3){.x=tl_pt.x, .y=tl_pt.y-skirt_length, .z=tl_pt.z};
        buff[buff_idx++] = (vec3){.x=bl_pt.x, .y=bl_pt.y-skirt_length, .z=bl_pt.z};
        buff[buff_idx++] = (vec3){.x=bl_pt.x, .y=bl_pt.y-skirt_length, .z=bl_pt.z};
        buff[buff_idx++] = bl_pt;
        buff[buff_idx++] = tl_pt;
      }
      // bottom row
      if ((y_idx + lod_divisor) >= toy) {
        buff[buff_idx++] = bl_pt;
        buff[buff_idx++] = (vec3){.x=bl_pt.x, .y=bl_pt.y-skirt_length, .z=bl_pt.z};
        buff[buff_idx++] = (vec3){.x=br_pt.x, .y=br_pt.y-skirt_length, .z=br_pt.z};
        buff[buff_idx++] = (vec3){.x=br_pt.x, .y=br_pt.y-skirt_length, .z=br_pt.z};
        buff[buff_idx++] = br_pt;
        buff[buff_idx++] = bl_pt;
      }
      // right col
      if ((x_idx + lod_divisor) >= tox) {
        buff[buff_idx++] = br_pt;
        buff[buff_idx++] = (vec3){.x=br_pt.x, .y=br_pt.y-skirt_length, .z=br_pt.z};
        buff[buff_idx++] = (vec3){.x=tr_pt.x, .y=tr_pt.y-skirt_length, .z=tr_pt.z};
        buff[buff_idx++] = (vec3){.x=tr_pt.x, .y=tr_pt.y-skirt_length, .z=tr_pt.z};
        buff[buff_idx++] = tr_pt;
        buff[buff_idx++] = br_pt;
      }
#endif

    } // endfor
  } //endfors
  *nverts = buff_idx;
}

// note: [ ][ ][ ][ ] is 4x4 pixels with 3 squares (6 tris) in-between
//       [ ][ ][ ][ ] so if a node is just 2 tris max then fromx, tox are:
//                                                        (0,1),(1,2),(2,3)
terrain_node_t create_terrain_node( const byte *data, int fromx, int tox, int fromy, int toy ) {
	assert( data );
	assert( fromx != tox );

	printf("creating node from (%i,%i) to (%i,%i)\n",fromx,fromy,tox,toy);

	terrain_node_t tn;
	tn.mid_point = (vec2){
    0.5f * (float)(tox + fromx) * TERR_WIDTH_SCALE,
    0.5f * (float)(toy + fromy) * TERR_WIDTH_SCALE };

	int tl_idx = fromy * terrain.hmap_cols + fromx;
	int tr_idx = fromy * terrain.hmap_cols + tox;
	int bl_idx = toy * terrain.hmap_cols + fromx;
	int br_idx = toy * terrain.hmap_cols + tox;

  { // min LOD version (far away)
    int lod_divisor = 16;
    int ncols = (terrain.hmap_cols / NODES_ACROSS) / lod_divisor;
    int nsquares = ncols * ncols;
    int nverts = nsquares * 6;
    size_t sz = sizeof( vec3 ) * nverts;
#ifdef TERR_SKIRTS
    int nskirt_tris = ncols * 2 * 4; // 4 sides
    int nskirt_verts = nskirt_tris * 6;
    sz += (nskirt_verts * sizeof( vec3 ));
#endif
    vec3 *buff = (vec3*)malloc( sz );
    int naddedverts = 0;
    fill_buffer_lod( lod_divisor, fromx, tox, fromy, toy, data, buff, &tn.min_lod_npoints );

    GLuint min_lod_vbo = 0;
    glGenBuffers( 1, &min_lod_vbo );
    glBindBuffer( GL_ARRAY_BUFFER, min_lod_vbo );
    glBufferData( GL_ARRAY_BUFFER, 3 * sizeof( GLfloat ) * tn.min_lod_npoints, buff, GL_STATIC_DRAW );
    glGenVertexArrays( 1, &tn.min_lod_vao );
    glBindVertexArray( tn.min_lod_vao );
    glEnableVertexAttribArray( 0 );
    glVertexAttribPointer( 0, 3, GL_FLOAT, GL_FALSE, 0, NULL );
    free(buff);
  }
  { // mid LOD version
    int lod_divisor = 4;
    int ncols = (terrain.hmap_cols / NODES_ACROSS) / lod_divisor;
    int nsquares = ncols * ncols;
    int nverts = nsquares * 6;
    size_t sz = sizeof( vec3 ) * nverts;
#ifdef TERR_SKIRTS
    int nskirt_tris = ncols * 2 * 4; // 4 sides
    int nskirt_verts = nskirt_tris * 6;
    sz += (nskirt_verts * sizeof( vec3 ));
#endif
    vec3 *buff = (vec3*)malloc( sz );
    fill_buffer_lod( lod_divisor, fromx, tox, fromy, toy, data, buff, &tn.mid_lod_npoints );

    GLuint mid_lod_vbo = 0;
    glGenBuffers( 1, &mid_lod_vbo );
    glBindBuffer( GL_ARRAY_BUFFER, mid_lod_vbo );
    glBufferData( GL_ARRAY_BUFFER, 3 * sizeof( GLfloat ) * tn.mid_lod_npoints, buff, GL_STATIC_DRAW );
    glGenVertexArrays( 1, &tn.mid_lod_vao );
    glBindVertexArray( tn.mid_lod_vao );
    glEnableVertexAttribArray( 0 );
    glVertexAttribPointer( 0, 3, GL_FLOAT, GL_FALSE, 0, NULL );
    free(buff);
  }
  { // max LOD version
    int lod_divisor = 1;
    int ncols = (terrain.hmap_cols / NODES_ACROSS) / lod_divisor;
    int nsquares = ncols * ncols;
    int nverts = nsquares * 6;
    size_t sz = sizeof( vec3 ) * nverts;
#ifdef TERR_SKIRTS
    int nskirt_tris = ncols * 2 * 4; // 4 sides
    int nskirt_verts = nskirt_tris * 6;
    sz += (nskirt_verts * sizeof( vec3 ));
#endif
    vec3 *buff = (vec3*)malloc( sz );
    fill_buffer_lod( lod_divisor, fromx, tox, fromy, toy, data, buff, &tn.max_lod_npoints );

    GLuint max_lod_vbo = 0;
    glGenBuffers( 1, &max_lod_vbo );
    glBindBuffer( GL_ARRAY_BUFFER, max_lod_vbo );
    glBufferData( GL_ARRAY_BUFFER, 3 * sizeof( GLfloat ) * tn.max_lod_npoints, buff, GL_STATIC_DRAW );
    glGenVertexArrays( 1, &tn.max_lod_vao );
    glBindVertexArray( tn.max_lod_vao );
    glEnableVertexAttribArray( 0 );
    glVertexAttribPointer( 0, 3, GL_FLOAT, GL_FALSE, 0, NULL );
    free(buff);
  }

	return tn;
}

int main() {
	printf(" hello worlds\n" );

	bool result = start_opengl();
	assert( result );

	{ // load image into geom
		int x, y, n;
		byte *data = stbi_load( HMAP_IMG, &x, &y, &n, 0);
		assert( data );
		assert( n == 1 );
		printf( "loaded img with %ix%i, %i chans\n", x, y, n );	
		terrain.hmap_cols = x;
		terrain.hmap_rows = y;

    for (int node_y_idx = 0; node_y_idx < NODES_ACROSS; node_y_idx++ ){
      for (int node_x_idx = 0; node_x_idx < NODES_ACROSS; node_x_idx++ ) {
        int width_of_node = x / NODES_ACROSS; // 512/8 = 64

        int start_y = width_of_node * node_y_idx;
        int end_y = width_of_node * (node_y_idx + 1 );
        end_y = MIN( end_y, y - 1 ); // TODO check y-1 bit

        int start_x = width_of_node * node_x_idx;
        int end_x = width_of_node * (node_x_idx + 1 );
        end_x = MIN( end_x, x - 1 );

        terrain.nodes[node_y_idx][node_x_idx] = create_terrain_node(
          data, start_x, end_x, start_y, end_y );
      }
    } //endfors

		free( data );
		printf( "terrain nodes created\n" );
	}

	GLuint shader_prog = 0;
	GLint P_loc = -1,V_loc = 1 ;
	{ // some default shader
    const char *vertex_shader = "#version 430\n"
                                "in vec3 vp;"
                                "uniform mat4 P, V;"
                                "out float h;"
                                "void main () {"
                                "h = vp.y;"
                                " gl_Position = P*V*vec4 (vp, 1.0);"
                                "}";
    /* the fragment shader colours each fragment (pixel-sized area of the
    triangle) */
    const char *fragment_shader = "#version 430\n"
                                  "in float h;"
                                  "out vec4 frag_colour;"
                                  "void main () {"
                                  " frag_colour = vec4 (h*0.01, h*0.01, h*0.01, 1.0);"
                                  "}";

		GLuint vert_shader = glCreateShader( GL_VERTEX_SHADER );
	  glShaderSource( vert_shader, 1, &vertex_shader, NULL );
  	glCompileShader( vert_shader );
  	GLuint frag_shader = glCreateShader( GL_FRAGMENT_SHADER );
  	glShaderSource( frag_shader, 1, &fragment_shader, NULL );
  	glCompileShader( frag_shader );
  	shader_prog = glCreateProgram();
  	glAttachShader( shader_prog, frag_shader );
  	glAttachShader( shader_prog, vert_shader );
  	glLinkProgram( shader_prog );
		P_loc = glGetUniformLocation( shader_prog, "P" );
		assert(P_loc >= 0 );
		V_loc = glGetUniformLocation( shader_prog, "V" );
		assert(V_loc >= 0 );
	}

	// NOTE: should this be negative to go up? seems very strange... TODO
  vec3 cam_pos = ( vec3 ){ 250.00, 150, 500.00 };
  
  float cam_heading_speed = 75.0f; // 30 degrees per second
  float cam_heading = 0.0f;         // y-rotation in degrees
  mat4 T = translate_mat4( ( vec3 ){ -cam_pos.x, -cam_pos.y, -cam_pos.z } );
  versor quaternion =
    quat_from_axis_deg( -cam_heading, ( vec3 ){ 0.0f, 1.0f, 0.0f } );
  mat4 R = quat_to_mat4( quaternion );
  mat4 V = mult_mat4_mat4( R, T );
  // keep track of some useful vectors that can be used for keyboard movement
  vec4 fwd = ( vec4 ){ 0.0f, 0.0f, -1.0f, 0.0f };
  vec4 rgt = ( vec4 ){ 1.0f, 0.0f, 0.0f, 0.0f };
  vec4 up = ( vec4 ){ 0.0f, 1.0f, 0.0f, 0.0f };
	mat4 P = perspective(67.0f,1.0f, 0.1, 1000.0f);

	glDepthFunc( GL_LESS );
  glEnable( GL_DEPTH_TEST );
  glEnable( GL_CULL_FACE );

glPolygonMode( GL_FRONT_AND_BACK, GL_LINE );
	glClearColor(0.2,0.2,0.2,1.0);
	double previous_seconds = glfwGetTime();
	while ( !glfwWindowShouldClose( g_window ) ) {
		// update timers
		double current_seconds = glfwGetTime();
		double elapsed_seconds = current_seconds - previous_seconds;
		previous_seconds = current_seconds;

		glfwPollEvents();
		if ( GLFW_PRESS == glfwGetKey( g_window, GLFW_KEY_ESCAPE ) ) {
      glfwSetWindowShouldClose( g_window, 1 );
    }

		static int polygon_mode = 1;
      static bool pdown = false;
      if ( GLFW_PRESS == glfwGetKey( g_window, GLFW_KEY_P ) ) {
        if ( !pdown ) {
          pdown = true;
          polygon_mode = ( polygon_mode + 1 ) % 3;

          switch ( polygon_mode ) {
            case 1: {
              glPolygonMode( GL_FRONT_AND_BACK, GL_LINE );
              glDisable( GL_CULL_FACE );
            } break;
            case 0: {
              glPolygonMode( GL_FRONT_AND_BACK, GL_POINT );
              glDisable( GL_CULL_FACE );
              static bool pdown = false;
              if ( GLFW_PRESS == glfwGetKey( g_window, GLFW_KEY_P ) ) {
                if ( !pdown ) {
                  pdown = true;
                  polygon_mode = ( polygon_mode + 1 ) % 3;

                  switch ( polygon_mode ) {
                    case 1: {
                      glPolygonMode( GL_FRONT_AND_BACK, GL_LINE );
                      glDisable( GL_CULL_FACE );
                    } break;
                    case 0: {
                      glPolygonMode( GL_FRONT_AND_BACK, GL_POINT );
                      glDisable( GL_CULL_FACE );
                    } break;
                    default: {
                      glPolygonMode( GL_FRONT_AND_BACK, GL_FILL );
                      glEnable( GL_CULL_FACE );
                    }
                  }
                }
              } else {
                pdown = false;
              }
            } break;
            default: {
              glPolygonMode( GL_FRONT_AND_BACK, GL_FILL );
              glEnable( GL_CULL_FACE );
            }
          }
        }
      } else {
        pdown = false;
      }
      { // camera update
        const float cam_speed = 50.0f;

        bool cam_moved = false;
        vec3 move = ( vec3 ){ 0.0, 0.0, 0.0 };
        float cam_yaw = 0.0f; // y-rotation in degrees
        //static float cam_heading = 0.0f;
        float cam_pitch = 0.0f;
        float cam_roll = 0.0;

        if ( GLFW_PRESS == glfwGetKey( g_window, GLFW_KEY_W ) ) {
          move.z -= ( cam_speed * elapsed_seconds );
          cam_moved = true;
        }
        if ( GLFW_PRESS == glfwGetKey( g_window, GLFW_KEY_S ) ) {
          move.z += ( cam_speed * elapsed_seconds );
          cam_moved = true;
        }
        if ( GLFW_PRESS == glfwGetKey( g_window, GLFW_KEY_A ) ) {
          move.x -= ( cam_speed * elapsed_seconds );
          cam_moved = true;
        }
        if ( GLFW_PRESS == glfwGetKey( g_window, GLFW_KEY_D ) ) {
          move.x += ( cam_speed * elapsed_seconds );
          cam_moved = true;
        }
        if ( GLFW_PRESS == glfwGetKey( g_window, GLFW_KEY_Q ) ) {
          move.y -= ( cam_speed * elapsed_seconds );
          cam_moved = true;
        }
        if ( GLFW_PRESS == glfwGetKey( g_window, GLFW_KEY_E ) ) {
          move.y += ( cam_speed * elapsed_seconds );
          cam_moved = true;
        }
        if ( glfwGetKey( g_window, GLFW_KEY_LEFT ) ) {
          cam_yaw += cam_heading_speed * elapsed_seconds;
          cam_heading += cam_yaw;
          cam_moved = true;
          versor q_yaw = quat_from_axis_deg( cam_yaw, v3_v4( up ) );
          mat4 Ry = rot_y_deg_mat4( cam_heading );

          quaternion = mult_quat_quat( q_yaw, quaternion );
          mat4 R = quat_to_mat4( quaternion );

          fwd = mult_mat4_vec4( Ry, ( vec4 ){ 0.0, 0.0, -1.0, 0.0 } );
          // print_vec4( fwd );
          rgt = mult_mat4_vec4( R, ( vec4 ){ 1.0, 0.0, 0.0, 0.0 } );
          // up = mult_mat4_vec4( R, ( vec4 ){ 0.0, 1.0, 0.0, 0.0 } );
        }
        if ( glfwGetKey( g_window, GLFW_KEY_RIGHT ) ) {
          cam_yaw -= cam_heading_speed * elapsed_seconds;
          cam_heading += cam_yaw;
          cam_moved = true;
          versor q_yaw = quat_from_axis_deg( cam_yaw, v3_v4( up ) );
          mat4 Ry = rot_y_deg_mat4( cam_heading );
          quaternion = mult_quat_quat( q_yaw, quaternion );
          mat4 R = quat_to_mat4( quaternion );
          fwd = mult_mat4_vec4( Ry, ( vec4 ){ 0.0, 0.0, -1.0, 0.0 } );

          rgt = mult_mat4_vec4( R, ( vec4 ){ 1.0, 0.0, 0.0, 0.0 } );
          // up = mult_mat4_vec4( R, ( vec4 ){ 0.0, 1.0, 0.0, 0.0 } );
        }
        if ( glfwGetKey( g_window, GLFW_KEY_UP ) ) {
          cam_pitch += cam_heading_speed * elapsed_seconds;
          cam_moved = true;
          versor q_pitch = quat_from_axis_deg( cam_pitch, v3_v4( rgt ) );
          quaternion = mult_quat_quat( q_pitch, quaternion );
          mat4 R = quat_to_mat4( quaternion );
          // fwd = mult_mat4_vec4( R, ( vec4 ){ 0.0, 0.0, -1.0, 0.0 } );
          rgt = mult_mat4_vec4( R, ( vec4 ){ 1.0, 0.0, 0.0, 0.0 } );
          // up = mult_mat4_vec4( R, ( vec4 ){ 0.0, 1.0, 0.0, 0.0 } );
        }
        if ( glfwGetKey( g_window, GLFW_KEY_DOWN ) ) {
          cam_pitch -= cam_heading_speed * elapsed_seconds;
          cam_moved = true;
          versor q_pitch = quat_from_axis_deg( cam_pitch, v3_v4( rgt ) );
          quaternion = mult_quat_quat( q_pitch, quaternion );
          // recalc axes to suit new orientation
          mat4 R = quat_to_mat4( quaternion );
          // fwd = mult_mat4_vec4( R, ( vec4 ){ 0.0, 0.0, -1.0, 0.0 } );
          rgt = mult_mat4_vec4( R, ( vec4 ){ 1.0, 0.0, 0.0, 0.0 } );
          // up = mult_mat4_vec4( R, ( vec4 ){ 0.0, 1.0, 0.0, 0.0 } );
        }
        if ( cam_moved ) {
          mat4 R = quat_to_mat4( quaternion );
          cam_pos = add_vec3_vec3( cam_pos, mult_vec3_f( v3_v4( fwd ), -move.z ) );
          cam_pos = add_vec3_vec3( cam_pos, mult_vec3_f( v3_v4( up ), move.y ) );
          cam_pos = add_vec3_vec3( cam_pos, mult_vec3_f( v3_v4( rgt ), move.x ) );
          // print_vec3( cam_pos );
          mat4 T = translate_mat4( cam_pos );

          mat4 inverse_R = inverse_mat4( R );
          V = mult_mat4_mat4( inverse_R, inverse_mat4( T ) );
        }
        float aspect = (float)g_gl_width / (float)g_gl_height;
        P = perspective( 67, aspect, 0.1, 1000.0 );
      }
			//print_mat4( V );
			glProgramUniformMatrix4fv(shader_prog, P_loc, 1, GL_FALSE, P.m);
			glProgramUniformMatrix4fv(shader_prog, V_loc, 1, GL_FALSE, V.m);

      glViewport( 0, 0, g_gl_width, g_gl_height );

    glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

    glUseProgram( shader_prog );

    int verts_drawn = 0;
    int draws = 0;

    for (int node_y_idx = 0; node_y_idx < NODES_ACROSS; node_y_idx++ ){
      for (int node_x_idx = 0; node_x_idx < NODES_ACROSS; node_x_idx++ ) {
        vec2 midpt = terrain.nodes[node_y_idx][node_x_idx].mid_point;
        vec2 dist3d = (vec2){ .x = midpt.x - cam_pos.x, .y = midpt.y - cam_pos.z };
        float dist1d = sqrt(dist3d.x * dist3d.x + dist3d.y * dist3d.y);

        //printf("dist=%f\n", dist1d);

        // TODO also do a dot product for angle

        if (dist1d < TERR_LOD0_DIST) {
          glBindVertexArray( terrain.nodes[node_y_idx][node_x_idx].max_lod_vao );
          glDrawArrays( GL_TRIANGLES, 0, terrain.nodes[node_y_idx][node_x_idx].max_lod_npoints );
          verts_drawn += terrain.nodes[node_y_idx][node_x_idx].max_lod_npoints;
        } else if (dist1d < TERR_LOD1_DIST) {
          glBindVertexArray( terrain.nodes[node_y_idx][node_x_idx].mid_lod_vao );
          glDrawArrays( GL_TRIANGLES, 0, terrain.nodes[node_y_idx][node_x_idx].mid_lod_npoints );
          verts_drawn += terrain.nodes[node_y_idx][node_x_idx].mid_lod_npoints;
        } else {
          glBindVertexArray( terrain.nodes[node_y_idx][node_x_idx].min_lod_vao );
          glDrawArrays( GL_TRIANGLES, 0, terrain.nodes[node_y_idx][node_x_idx].min_lod_npoints );
          verts_drawn += terrain.nodes[node_y_idx][node_x_idx].min_lod_npoints;
        }
        draws++;
      }
    } // endfors

    _update_fps_counter( g_window, verts_drawn, draws );
    
    glfwSwapBuffers( g_window );
  }

  glfwTerminate();

	return 0;
}
