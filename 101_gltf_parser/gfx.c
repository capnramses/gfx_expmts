#include "gfx.h"
#include "glcontext.h"
#include "apg_maths.h"
#include "apg_ply.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define GFX_SHADER_BINDING_VP 0
#define GFX_SHADER_BINDING_VT 1
#define GFX_SHADER_BINDING_VN 2
#define GFX_SHADER_BINDING_VC 3
#define GFX_SHADER_BINDING_VTAN 4
#define GFX_MAX_SHADER_STR 20000

GLFWwindow* gfx_window_ptr; // extern in apg_glcontext.h

static GLFWmonitor* gfx_monitor_ptr;
static int g_win_width = 1920, g_win_height = 1080;

gfx_shader_t gfx_default_shader, gfx_textured_quad_shader;
gfx_mesh_t gfx_cube_mesh;
gfx_mesh_t gfx_ss_quad_mesh;

bool gfx_start( const char* window_title, int w, int h, bool fullscreen ) {
  g_win_width  = w;
  g_win_height = h;
  { // GLFW
    if ( !glfwInit() ) {
      fprintf( stderr, "ERROR: could not start GLFW3\n" );
      return false;
    }
    glfwWindowHint( GLFW_CONTEXT_VERSION_MAJOR, 4 );
    glfwWindowHint( GLFW_CONTEXT_VERSION_MINOR, 1 );
    glfwWindowHint( GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE );
    glfwWindowHint( GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE );
    { // MSAA hint
      // NOTE(Anton) fetching max samples didn't work on my linux nvidia driver before window creation, which kind of defeats the purpose of asking
      int nsamples = 16;
      glfwWindowHint( GLFW_SAMPLES, nsamples );
    }
    if ( fullscreen ) {
      gfx_monitor_ptr         = glfwGetPrimaryMonitor();
      const GLFWvidmode* mode = glfwGetVideoMode( gfx_monitor_ptr );
      glfwWindowHint( GLFW_RED_BITS, mode->redBits );
      glfwWindowHint( GLFW_GREEN_BITS, mode->greenBits );
      glfwWindowHint( GLFW_BLUE_BITS, mode->blueBits );
      glfwWindowHint( GLFW_REFRESH_RATE, mode->refreshRate );
      g_win_width  = mode->width;
      g_win_height = mode->height;
    }

    gfx_window_ptr = glfwCreateWindow( g_win_width, g_win_height, window_title, gfx_monitor_ptr, NULL );
    if ( !gfx_window_ptr ) {
      fprintf( stderr, "ERROR: could not open window with GLFW3\n" );
      glfwTerminate();
      return false;
    }
    glfwMakeContextCurrent( gfx_window_ptr );
  }
  { // GLAD
    if ( !gladLoadGL() ) {
      fprintf( stderr, "FATAL ERROR: Could not load OpenGL header using Glad.\n" );
      return false;
    }
    printf( "OpenGL %d.%d\n", GLVersion.major, GLVersion.minor );
  }

  const GLubyte* renderer = glGetString( GL_RENDERER );
  const GLubyte* version  = glGetString( GL_VERSION );
  printf( "Renderer: %s\n", renderer );
  printf( "OpenGL version supported %s\n", version );

  {
    // IMPORTANT!!!! NOTE THE SCALE of x0.1 here!
    const char* vertex_shader =
      "#version 410 core\n"
      "in vec3 a_vp;\n"
      "in vec3 a_vc;\n"
      "uniform mat4 u_P, u_V, u_M;\n"
      "out vec3 vc;\n"
      "void main () {\n"
      "  vc = a_vc;\n"
      "  gl_Position = u_P * u_V * u_M * vec4( a_vp * 0.1, 1.0 );\n"
      "}\n";
    const char* fragment_shader =
      "#version 410 core\n"
      "in vec3 vc;\n"
      "uniform float u_alpha;\n"
      "out vec4 o_frag_colour;\n"
      "void main () {\n"
      "  o_frag_colour = vec4( vc, u_alpha );\n"
      "  o_frag_colour.rgb = pow( o_frag_colour.rgb, vec3( 1.0 / 2.2 ) );\n"
      "}\n";
    gfx_default_shader = gfx_create_shader_program_from_strings( vertex_shader, fragment_shader );
    if ( !gfx_default_shader.loaded ) { return false; }
  }
  {
    const char* vertex_shader =
      "#version 410 core\n"
      "in vec2 a_vp;\n"
      "uniform mat4 u_P, u_V, u_M;\n"
      "out vec2 v_st;\n"
      "void main() {\n"
      "  v_st        = a_vp * 0.5 + 0.5;\n"
      "  v_st.t      = 1.0 - v_st.t;\n"
      "  gl_Position = u_P * u_V * u_M * vec4( a_vp, 0.0, 1.0 );\n"
      "}\n";
    const char* fragment_shader =
      "#version 410 core\n"
      "in vec2 v_st;\n"
      "uniform sampler2D u_texture_a;\n"
      "out vec4 o_frag_colour;\n"
      "void main() {\n"
      "  o_frag_colour = texture( u_texture_a, v_st );\n"
      "}\n";
    gfx_textured_quad_shader = gfx_create_shader_program_from_strings( vertex_shader, fragment_shader );
    if ( !gfx_textured_quad_shader.loaded ) { return false; }
  }
  {
    float ss_quad_pos[] = { -1.0, 1.0, -1.0, -1.0, 1.0, 1.0, 1.0, -1.0 };
    gfx_ss_quad_mesh    = gfx_create_mesh_from_mem( ss_quad_pos, 2, NULL, 0, NULL, 0, NULL, 0, NULL, 0, 0, 4, false, false );
  }
  { //
    gfx_cube_mesh  = ( gfx_mesh_t ){ .n_vertices = 36 };
    float points[] = { -1.0f, 1.0f, -1.0f, -1.0f, -1.0f, -1.0f, 1.0f, -1.0f, -1.0f, 1.0f, -1.0f, -1.0f, 1.0f, 1.0f, -1.0f, -1.0f, 1.0f, -1.0f, -1.0f, -1.0f,
      1.0f, -1.0f, -1.0f, -1.0f, -1.0f, 1.0f, -1.0f, -1.0f, 1.0f, -1.0f, -1.0f, 1.0f, 1.0f, -1.0f, -1.0f, 1.0f, 1.0f, -1.0f, -1.0f, 1.0f, -1.0f, 1.0f, 1.0f,
      1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, -1.0f, 1.0f, -1.0f, -1.0f, -1.0f, -1.0f, 1.0f, -1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f,
      -1.0f, 1.0f, -1.0f, -1.0f, 1.0f, -1.0f, 1.0f, -1.0f, 1.0f, 1.0f, -1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, -1.0f, 1.0f, 1.0f, -1.0f, 1.0f, -1.0f, -1.0f,
      -1.0f, -1.0f, -1.0f, -1.0f, 1.0f, 1.0f, -1.0f, -1.0f, 1.0f, -1.0f, -1.0f, -1.0f, -1.0f, 1.0f, 1.0f, -1.0f, 1.0f };
    glGenBuffers( 1, &gfx_cube_mesh.points_vbo );
    glBindBuffer( GL_ARRAY_BUFFER, gfx_cube_mesh.points_vbo );
    glBufferData( GL_ARRAY_BUFFER, 3 * 36 * sizeof( GLfloat ), &points, GL_STATIC_DRAW );
    glGenVertexArrays( 1, &gfx_cube_mesh.vao );
    glBindVertexArray( gfx_cube_mesh.vao );
    glEnableVertexAttribArray( 0 );
    glVertexAttribPointer( 0, 3, GL_FLOAT, GL_FALSE, 0, NULL );
    glBindBuffer( GL_ARRAY_BUFFER, 0 );
    glBindVertexArray( 0 );
  }
  {
    glClearColor( 0.5, 0.5, 0.5, 1.0 );
    glDepthFunc( GL_LESS );
    glEnable( GL_DEPTH_TEST );
    glCullFace( GL_BACK );
    glFrontFace( GL_CCW );
    glEnable( GL_CULL_FACE );

    glfwSwapInterval( 1 ); // vsync
  }

  return true;
}

void gfx_stop() {
  gfx_delete_shader_program( &gfx_default_shader );
  gfx_delete_mesh( &gfx_cube_mesh );
  glfwTerminate();
}

bool gfx_should_window_close() {
  assert( gfx_window_ptr );
  return glfwWindowShouldClose( gfx_window_ptr );
}

void gfx_viewport( int x, int y, int w, int h ) { glViewport( x, y, w, h ); }

void gfx_clear_colour_and_depth_buffers( float r, float g, float b, float a ) {
  glClearColor( r, g, b, a );
  glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
}

void gfx_framebuffer_dims( int* width, int* height ) {
  assert( width && height && gfx_window_ptr );
  glfwGetFramebufferSize( gfx_window_ptr, width, height );
}
void gfx_window_dims( int* width, int* height ) {
  assert( width && height && gfx_window_ptr );
  glfwGetWindowSize( gfx_window_ptr, width, height );
}

void gfx_swap_buffer() {
  assert( gfx_window_ptr );

  glfwSwapBuffers( gfx_window_ptr );
}

void gfx_poll_events() {
  assert( gfx_window_ptr );

  glfwPollEvents();
}

void gfx_alpha_blend( bool enable ) {
  if ( enable ) {
    glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
    glEnable( GL_BLEND );
  } else {
    glDisable( GL_BLEND );
  }
}

void gfx_backface_culling( bool enable ) {
  if ( enable ) {
    glCullFace( GL_BACK );
    glFrontFace( GL_CCW );
    glEnable( GL_CULL_FACE );
  } else {
    glDisable( GL_CULL_FACE );
  }
}

void gfx_depth_testing( bool enable ) {
  if ( enable ) {
    glEnable( GL_DEPTH_TEST );
  } else {
    glDisable( GL_DEPTH_TEST );
  }
}

void gfx_depth_mask( bool enable ) {
  if ( enable ) {
    glDepthMask( GL_TRUE );
  } else {
    glDepthMask( GL_FALSE );
  }
}

void gfx_cubemap_seamless( bool enable ) {
  if ( enable ) {
    glEnable( GL_TEXTURE_CUBE_MAP_SEAMLESS );
  } else {
    glDisable( GL_TEXTURE_CUBE_MAP_SEAMLESS );
  }
}

gfx_mesh_t gfx_create_mesh_from_mem(                                                     //
  const float* points_buffer, int n_points_comps,                                        //
  const float* texcoords_buffer, int n_texcoord_comps,                                   //
  const float* normals_buffer, int n_normal_comps,                                       //
  const float* colours_buffer, int n_colours_comps,                                      //
  const void* indices_buffer, size_t indices_buffer_sz, gfx_indices_type_t indices_type, //
  int n_vertices, bool dynamic, bool calc_tangents ) {
  gfx_mesh_t mesh = ( gfx_mesh_t ){ .indices_type = indices_type, .n_vertices = n_vertices, .dynamic = dynamic };

  if ( !points_buffer || n_points_comps <= 0 ) {
    fprintf( stderr, "ERROR:loading mesh from memory. No vertex points given.\n" );
    return mesh;
  }

  glGenVertexArrays( 1, &mesh.vao );
  glGenBuffers( 1, &mesh.points_vbo );
  if ( calc_tangents ) { glGenBuffers( 1, &mesh.tangents_vbo ); }
  if ( colours_buffer && n_colours_comps > 0 ) { glGenBuffers( 1, &mesh.colours_vbo ); }
  if ( texcoords_buffer && n_texcoord_comps > 0 ) { glGenBuffers( 1, &mesh.texcoords_vbo ); }
  if ( normals_buffer && n_normal_comps > 0 ) { glGenBuffers( 1, &mesh.normals_vbo ); }
  if ( indices_buffer ) { glGenBuffers( 1, &mesh.indices_vbo ); }

  gfx_update_mesh_from_mem( &mesh, points_buffer, n_points_comps, texcoords_buffer, n_texcoord_comps, normals_buffer, n_normal_comps, colours_buffer,
    n_colours_comps, indices_buffer, indices_buffer_sz, indices_type, n_vertices, dynamic, calc_tangents );

  return mesh;
}

void gfx_update_mesh_from_mem( gfx_mesh_t* mesh,                                         //
  const float* points_buffer, int n_points_comps,                                        //
  const float* texcoords_buffer, int n_texcoord_comps,                                   //
  const float* normals_buffer, int n_normal_comps,                                       //
  const float* colours_buffer, int n_colours_comps,                                      //
  const void* indices_buffer, size_t indices_buffer_sz, gfx_indices_type_t indices_type, //
  int n_vertices, bool dynamic, bool calc_tangents ) {
  assert( points_buffer && n_points_comps > 0 );
  if ( !points_buffer || n_points_comps <= 0 ) { return; }
  assert( mesh && mesh->vao && mesh->points_vbo );
  if ( !mesh || !mesh->vao || !mesh->points_vbo ) { return; }

  GLenum usage     = !dynamic ? GL_STATIC_DRAW : GL_DYNAMIC_DRAW;
  mesh->n_vertices = n_vertices;
  mesh->dynamic    = dynamic;

  mesh->indices_type = indices_type;
  mesh->n_indices    = 0;
  if ( indices_buffer_sz > 0 ) {
    switch ( indices_type ) {
    case GFX_INDICES_TYPE_UBYTE: mesh->n_indices = indices_buffer_sz; break;      // ubyte
    case GFX_INDICES_TYPE_UINT16: mesh->n_indices = indices_buffer_sz / 2; break; // ushort
    case GFX_INDICES_TYPE_UINT32: mesh->n_indices = indices_buffer_sz / 4; break; // uint
    default: assert( false && "unhandled indices type" ); break;
    }
  }

  float* tan_ptr = NULL;

  // NOTE: assumes triangles here
  if ( calc_tangents ) {
    assert( points_buffer && 3 == n_points_comps );
    assert( texcoords_buffer && 2 == n_texcoord_comps );
    size_t tans_sz = sizeof( float ) * 3 * n_vertices;
    tan_ptr        = malloc( tans_sz );

    if ( indices_buffer && mesh->indices_vbo && indices_buffer_sz > 0 ) {
      // NB idx is an index into indices!! and i'm using i as the actual vector array index
      for ( size_t idx = 0; idx < mesh->n_indices; idx += 3 ) {
        size_t index_a = 0, index_b = 0, index_c = 0;
        switch ( indices_type ) {
        case GFX_INDICES_TYPE_UBYTE: {
          uint8_t* indices_ptr = (uint8_t*)indices_buffer;
          index_a              = indices_ptr[idx + 0];
          index_b              = indices_ptr[idx + 1];
          index_c              = indices_ptr[idx + 2];
        }; break; // ubyte
        case GFX_INDICES_TYPE_UINT16: {
          uint16_t* indices_ptr = (uint16_t*)indices_buffer;
          index_a               = indices_ptr[idx + 0];
          index_b               = indices_ptr[idx + 1];
          index_c               = indices_ptr[idx + 2];
        }; break; // ushort
        case GFX_INDICES_TYPE_UINT32: {
          uint32_t* indices_ptr = (uint32_t*)indices_buffer;
          index_a               = indices_ptr[idx + 0];
          index_b               = indices_ptr[idx + 1];
          index_c               = indices_ptr[idx + 2];
        }; break; // uint
        default: assert( false && "unhandled indices type" ); break;
        }

        vec3 pos[3];
        memcpy( &pos[0].x, &points_buffer[index_a * n_points_comps], n_points_comps * sizeof( float ) );
        memcpy( &pos[1].x, &points_buffer[index_b * n_points_comps], n_points_comps * sizeof( float ) );
        memcpy( &pos[2].x, &points_buffer[index_c * n_points_comps], n_points_comps * sizeof( float ) );
        vec2 uv[3];
        memcpy( &uv[0].x, &texcoords_buffer[index_a * n_texcoord_comps], n_texcoord_comps * sizeof( float ) );
        memcpy( &uv[1].x, &texcoords_buffer[index_b * n_texcoord_comps], n_texcoord_comps * sizeof( float ) );
        memcpy( &uv[2].x, &texcoords_buffer[index_c * n_texcoord_comps], n_texcoord_comps * sizeof( float ) );
        vec3 edge_a     = sub_vec3_vec3( pos[1], pos[0] );
        vec3 edge_b     = sub_vec3_vec3( pos[2], pos[0] );
        vec2 delta_uv_a = sub_vec2_vec2( uv[1], uv[0] );
        vec2 delta_uv_b = sub_vec2_vec2( uv[2], uv[0] );
        float denom     = delta_uv_a.x * delta_uv_b.y - delta_uv_a.y * delta_uv_b.x;
        // NB this is true for Damaged Helmet
        if ( 0.0f == denom ) { denom = 0.0001f; }
        float r  = 1.0f / denom;
        vec3 tan = ( vec3 ){
          .x = r * ( delta_uv_b.y * edge_a.x - delta_uv_a.y * edge_b.x ), //
          .y = r * ( delta_uv_b.y * edge_a.y - delta_uv_a.y * edge_b.y ), //
          .z = r * ( delta_uv_b.y * edge_a.z - delta_uv_a.y * edge_b.z )  //
        };
        // each triangle has 1 tangent ( same for all 3 vertices )
        if ( index_c * 3 * sizeof( float ) >= tans_sz ) {
          fprintf( stderr, "ERROR: (non-indexed) at vert idx=%i. index a =%i, b= %i, c=%i, and total sz = %u\n", (int)idx, (int)index_a, (int)index_b,
            (int)index_c, (uint32_t)tans_sz );
        }
        assert( index_c * 3 * sizeof( float ) < tans_sz );
        memcpy( &tan_ptr[index_a * 3], &tan.x, sizeof( float ) * 3 );
        memcpy( &tan_ptr[index_b * 3], &tan.x, sizeof( float ) * 3 );
        memcpy( &tan_ptr[index_c * 3], &tan.x, sizeof( float ) * 3 );
        // printf( "stored tan %i,%i,%i\n", index_a, index_b, index_c );
      } // endfor each triangle

    } else {
      // NB idx is an index into indices!! and i'm using i as the actual vector array index
      for ( size_t idx = 0; idx < mesh->n_vertices; idx += 3 ) {
        size_t index_a = idx, index_b = idx + 1, index_c = idx + 2;

        vec3 pos[3];
        memcpy( &pos[0].x, &points_buffer[index_a * n_points_comps], n_points_comps * sizeof( float ) );
        memcpy( &pos[1].x, &points_buffer[index_b * n_points_comps], n_points_comps * sizeof( float ) );
        memcpy( &pos[2].x, &points_buffer[index_c * n_points_comps], n_points_comps * sizeof( float ) );
        vec2 uv[3];
        memcpy( &uv[0].x, &texcoords_buffer[index_a * n_texcoord_comps], n_texcoord_comps * sizeof( float ) );
        memcpy( &uv[1].x, &texcoords_buffer[index_b * n_texcoord_comps], n_texcoord_comps * sizeof( float ) );
        memcpy( &uv[2].x, &texcoords_buffer[index_c * n_texcoord_comps], n_texcoord_comps * sizeof( float ) );
        vec3 edge_a     = sub_vec3_vec3( pos[1], pos[0] );
        vec3 edge_b     = sub_vec3_vec3( pos[2], pos[0] );
        vec2 delta_uv_a = sub_vec2_vec2( uv[1], uv[0] );
        vec2 delta_uv_b = sub_vec2_vec2( uv[2], uv[0] );
        float denom     = delta_uv_a.x * delta_uv_b.y - delta_uv_a.y * delta_uv_b.x;
        // NB this is true for Damaged Helmet
        if ( 0.0f == denom ) { denom = 0.0001f; }
        float r  = 1.0f / denom;
        vec3 tan = ( vec3 ){
          .x = r * ( delta_uv_b.y * edge_a.x - delta_uv_a.y * edge_b.x ), //
          .y = r * ( delta_uv_b.y * edge_a.y - delta_uv_a.y * edge_b.y ), //
          .z = r * ( delta_uv_b.y * edge_a.z - delta_uv_a.y * edge_b.z )  //
        };
        // each triangle has 1 tangent ( same for all 3 vertices )
        if ( index_c * 3 * sizeof( float ) >= tans_sz ) {
          fprintf( stderr, "ERROR: (non-indexed) at vert idx=%i. index a =%i, b= %i, c=%i, and total sz = %u\n", (int)idx, (int)index_a, (int)index_b,
            (int)index_c, (uint32_t)tans_sz );
        }
        assert( index_c * 3 * sizeof( float ) < tans_sz );
        memcpy( &tan_ptr[index_a * 3], &tan.x, sizeof( float ) * 3 );
        memcpy( &tan_ptr[index_b * 3], &tan.x, sizeof( float ) * 3 );
        memcpy( &tan_ptr[index_c * 3], &tan.x, sizeof( float ) * 3 );
        // printf( "stored tan %i,%i,%i\n", index_a, index_b, index_c );
      } // endfor each triangle
    }
    //
    //
    //
  }

  glBindVertexArray( mesh->vao );
  {
    glBindBuffer( GL_ARRAY_BUFFER, mesh->points_vbo );
    glBufferData( GL_ARRAY_BUFFER, sizeof( float ) * n_points_comps * n_vertices, points_buffer, usage );
    glEnableVertexAttribArray( GFX_SHADER_BINDING_VP );
    glVertexAttribPointer( GFX_SHADER_BINDING_VP, n_points_comps, GL_FLOAT, GL_FALSE, 0, NULL );
    glBindBuffer( GL_ARRAY_BUFFER, 0 );
  }
  if ( texcoords_buffer && n_texcoord_comps > 0 && mesh->texcoords_vbo ) {
    glBindBuffer( GL_ARRAY_BUFFER, mesh->texcoords_vbo );
    glBufferData( GL_ARRAY_BUFFER, sizeof( float ) * n_texcoord_comps * n_vertices, texcoords_buffer, usage );
    glEnableVertexAttribArray( GFX_SHADER_BINDING_VT );
    glVertexAttribPointer( GFX_SHADER_BINDING_VT, n_texcoord_comps, GL_FLOAT, GL_FALSE, 0, NULL );
    glBindBuffer( GL_ARRAY_BUFFER, 0 );
  }
  if ( normals_buffer && n_normal_comps > 0 && mesh->normals_vbo ) {
    glBindBuffer( GL_ARRAY_BUFFER, mesh->normals_vbo );
    glBufferData( GL_ARRAY_BUFFER, sizeof( float ) * n_normal_comps * n_vertices, normals_buffer, usage );
    glEnableVertexAttribArray( GFX_SHADER_BINDING_VN );
    glVertexAttribPointer( GFX_SHADER_BINDING_VN, n_normal_comps, GL_FLOAT, GL_FALSE, 0, NULL );
    glBindBuffer( GL_ARRAY_BUFFER, 0 );
  }
  if ( colours_buffer && n_colours_comps > 0 && mesh->colours_vbo ) {
    glBindBuffer( GL_ARRAY_BUFFER, mesh->colours_vbo );
    glBufferData( GL_ARRAY_BUFFER, sizeof( float ) * n_colours_comps * n_vertices, colours_buffer, usage );
    glEnableVertexAttribArray( GFX_SHADER_BINDING_VC );
    glVertexAttribPointer( GFX_SHADER_BINDING_VC, n_colours_comps, GL_FLOAT, GL_FALSE, 0, NULL );
    glBindBuffer( GL_ARRAY_BUFFER, 0 );
  }
  if ( tan_ptr && mesh->tangents_vbo ) {
    glBindBuffer( GL_ARRAY_BUFFER, mesh->tangents_vbo );
    glBufferData( GL_ARRAY_BUFFER, sizeof( float ) * 3 * n_vertices, tan_ptr, usage );
    glEnableVertexAttribArray( GFX_SHADER_BINDING_VTAN );
    glVertexAttribPointer( GFX_SHADER_BINDING_VTAN, 3, GL_FLOAT, GL_FALSE, 0, NULL );
    glBindBuffer( GL_ARRAY_BUFFER, 0 );
  }
  if ( indices_buffer && mesh->indices_vbo ) {
    glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, mesh->indices_vbo );
    glBufferData( GL_ELEMENT_ARRAY_BUFFER, indices_buffer_sz, indices_buffer, usage );
  }
  glBindVertexArray( 0 );

  if ( tan_ptr ) { free( tan_ptr ); }
}

// requires apg_ply
gfx_mesh_t gfx_mesh_create_from_ply( const char* ply_filename, bool calc_tangents ) {
  gfx_mesh_t mesh = ( gfx_mesh_t ){ .n_vertices = 0 };
  if ( !ply_filename ) { return mesh; }
  apg_ply_t ply = ( apg_ply_t ){ .n_vertices = 0 };
  if ( !apg_ply_read( ply_filename, &ply ) ) {
    fprintf( stderr, "ERROR reading ply mesh from file `%s`\n", ply_filename );
    return mesh;
  }
  if ( !ply.n_vertices ) { return mesh; }

  // convert colours to floats
  float* rgb_f = NULL;
  if ( ply.n_colours_comps > 0 ) {
    rgb_f = malloc( sizeof( float ) * ply.n_colours_comps * ply.n_vertices );
    assert( rgb_f );
    for ( uint32_t i = 0; i < ply.n_vertices; i++ ) {
      rgb_f[i * ply.n_colours_comps + 0] = ply.colours_ptr[i * ply.n_colours_comps + 0] / 255.0f;
      rgb_f[i * ply.n_colours_comps + 1] = ply.colours_ptr[i * ply.n_colours_comps + 1] / 255.0f;
      rgb_f[i * ply.n_colours_comps + 2] = ply.colours_ptr[i * ply.n_colours_comps + 2] / 255.0f;
      if ( 4 == ply.n_colours_comps ) { rgb_f[i * ply.n_colours_comps + 3] = ply.colours_ptr[i * ply.n_colours_comps + 3] / 255.0f; }
    }
  }

  mesh = gfx_create_mesh_from_mem( ply.positions_ptr, ply.n_positions_comps, ply.texcoords_ptr, ply.n_texcoords_comps, ply.normals_ptr, ply.n_normals_comps,
    rgb_f, ply.n_colours_comps, NULL, 0, GFX_INDICES_TYPE_UBYTE, ply.n_vertices, false, calc_tangents );

  apg_ply_free( &ply );
  free( rgb_f );

  return mesh;
}

void gfx_delete_mesh( gfx_mesh_t* mesh ) {
  assert( mesh );

  if ( mesh->indices_vbo ) { glDeleteBuffers( 1, &mesh->indices_vbo ); }
  if ( mesh->tangents_vbo ) { glDeleteBuffers( 1, &mesh->tangents_vbo ); }
  if ( mesh->colours_vbo ) { glDeleteBuffers( 1, &mesh->colours_vbo ); }
  if ( mesh->normals_vbo ) { glDeleteBuffers( 1, &mesh->normals_vbo ); }
  if ( mesh->texcoords_vbo ) { glDeleteBuffers( 1, &mesh->texcoords_vbo ); }
  if ( mesh->points_vbo ) { glDeleteBuffers( 1, &mesh->points_vbo ); }
  if ( mesh->vao ) { glDeleteVertexArrays( 1, &mesh->vao ); }
  *mesh = ( gfx_mesh_t ){ .n_vertices = 0 };
}

gfx_shader_t gfx_create_shader_program_from_files( const char* vert_shader_filename, const char* frag_shader_filename ) {
  gfx_shader_t shader = ( gfx_shader_t ){ .program_gl = 0 };

  char vert_shader_str[GFX_MAX_SHADER_STR], frag_shader_str[GFX_MAX_SHADER_STR];
  {
    FILE* f_ptr = fopen( vert_shader_filename, "rb" );
    if ( !f_ptr ) {
      fprintf( stderr, "ERROR: opening shader file `%s`\n", vert_shader_filename );
      return shader;
    }
    size_t count = fread( vert_shader_str, 1, GFX_MAX_SHADER_STR - 1, f_ptr );
    assert( count < GFX_MAX_SHADER_STR - 1 ); // file was too long
    vert_shader_str[count] = '\0';
    fclose( f_ptr );
  }
  {
    FILE* f_ptr = fopen( frag_shader_filename, "rb" );
    if ( !f_ptr ) {
      fprintf( stderr, "ERROR: opening shader file `%s`\n", frag_shader_filename );
      return shader;
    }
    size_t count = fread( frag_shader_str, 1, GFX_MAX_SHADER_STR - 1, f_ptr );
    assert( count < GFX_MAX_SHADER_STR - 1 ); // file was too long
    frag_shader_str[count] = '\0';
    fclose( f_ptr );
  }

  shader = gfx_create_shader_program_from_strings( vert_shader_str, frag_shader_str );
  strncpy( shader.vs_filename, vert_shader_filename, GFX_SHADER_PATH_MAX - 1 );
  strncpy( shader.fs_filename, frag_shader_filename, GFX_SHADER_PATH_MAX - 1 );
  shader.vs_filename[GFX_SHADER_PATH_MAX - 1] = shader.fs_filename[GFX_SHADER_PATH_MAX - 1] = '\0';
  if ( !shader.program_gl ) { fprintf( stderr, "ERROR: failed to compile shader from files `%s` and `%s`\n", vert_shader_filename, frag_shader_filename ); }
  return shader;
}

static bool _recompile_shader_with_check( GLuint shader, const char* src_str ) {
  glShaderSource( shader, 1, &src_str, NULL );
  glCompileShader( shader );
  int params = -1;
  glGetShaderiv( shader, GL_COMPILE_STATUS, &params );
  if ( GL_TRUE != params ) {
    fprintf( stderr, "ERROR: shader index %u did not compile. src:\n%s\n", shader, src_str );
    int max_length    = 2048;
    int actual_length = 0;
    char slog[2048];
    glGetShaderInfoLog( shader, max_length, &actual_length, slog );
    fprintf( stderr, "shader info log for GL index %u:\n%s\n", shader, slog );
    if ( 0 != shader ) { glDeleteShader( shader ); }
    return false;
  }
  return true;
}

gfx_shader_t gfx_create_shader_program_from_strings( const char* vert_shader_str, const char* frag_shader_str ) {
  assert( vert_shader_str && frag_shader_str );

  gfx_shader_t shader = ( gfx_shader_t ){ .program_gl = 0 };
  GLuint vs           = glCreateShader( GL_VERTEX_SHADER );
  GLuint fs           = glCreateShader( GL_FRAGMENT_SHADER );
  bool res_a          = _recompile_shader_with_check( vs, vert_shader_str );
  bool res_b          = _recompile_shader_with_check( fs, frag_shader_str );
  shader.program_gl   = glCreateProgram();
  glAttachShader( shader.program_gl, fs );
  glAttachShader( shader.program_gl, vs );
  glBindAttribLocation( shader.program_gl, GFX_SHADER_BINDING_VP, "a_vp" );
  glBindAttribLocation( shader.program_gl, GFX_SHADER_BINDING_VT, "a_vt" );
  glBindAttribLocation( shader.program_gl, GFX_SHADER_BINDING_VN, "a_vn" );
  glBindAttribLocation( shader.program_gl, GFX_SHADER_BINDING_VC, "a_vc" );
  glBindAttribLocation( shader.program_gl, GFX_SHADER_BINDING_VTAN, "a_vtan" );
  glLinkProgram( shader.program_gl );
  glDeleteShader( vs );
  glDeleteShader( fs );

  shader.u_M                         = glGetUniformLocation( shader.program_gl, "u_M" );
  shader.u_V                         = glGetUniformLocation( shader.program_gl, "u_V" );
  shader.u_P                         = glGetUniformLocation( shader.program_gl, "u_P" );
  shader.u_texture_a                 = glGetUniformLocation( shader.program_gl, "u_texture_a" );
  shader.u_texture_b                 = glGetUniformLocation( shader.program_gl, "u_texture_b" );
  shader.u_texture_albedo            = glGetUniformLocation( shader.program_gl, "u_texture_albedo" );
  shader.u_texture_metal_roughness   = glGetUniformLocation( shader.program_gl, "u_texture_metal_roughness" );
  shader.u_texture_emissive          = glGetUniformLocation( shader.program_gl, "u_texture_emissive" );
  shader.u_texture_ambient_occlusion = glGetUniformLocation( shader.program_gl, "u_texture_ambient_occlusion" );
  shader.u_texture_normal            = glGetUniformLocation( shader.program_gl, "u_texture_normal" );
  shader.u_texture_environment       = glGetUniformLocation( shader.program_gl, "u_texture_environment" );
  shader.u_texture_irradiance_map    = glGetUniformLocation( shader.program_gl, "u_texture_irradiance_map" );
  shader.u_texture_prefilter_map     = glGetUniformLocation( shader.program_gl, "u_texture_prefilter_map" );
  shader.u_texture_brdf_lut          = glGetUniformLocation( shader.program_gl, "u_texture_brdf_lut" );
  shader.u_alpha                     = glGetUniformLocation( shader.program_gl, "u_alpha" );
  shader.u_base_colour_rgba          = glGetUniformLocation( shader.program_gl, "u_base_colour_rgba" );
  shader.u_roughness_factor          = glGetUniformLocation( shader.program_gl, "u_roughness_factor" );
  shader.u_metallic_factor           = glGetUniformLocation( shader.program_gl, "u_metallic_factor" );
  shader.u_cam_pos_wor               = glGetUniformLocation( shader.program_gl, "u_cam_pos_wor" );
  shader.u_light_pos_wor             = glGetUniformLocation( shader.program_gl, "u_light_pos_wor" );

  glProgramUniform1i( shader.program_gl, shader.u_texture_a, 0 );
  glProgramUniform1i( shader.program_gl, shader.u_texture_b, 1 );
  glProgramUniform1i( shader.program_gl, shader.u_texture_albedo, GFX_TEXTURE_UNIT_ALBEDO );
  glProgramUniform1i( shader.program_gl, shader.u_texture_metal_roughness, GFX_TEXTURE_UNIT_METAL_ROUGHNESS );
  glProgramUniform1i( shader.program_gl, shader.u_texture_emissive, GFX_TEXTURE_UNIT_EMISSIVE );
  glProgramUniform1i( shader.program_gl, shader.u_texture_ambient_occlusion, GFX_TEXTURE_UNIT_AMBIENT_OCCLUSION );
  glProgramUniform1i( shader.program_gl, shader.u_texture_normal, GFX_TEXTURE_UNIT_NORMAL );
  glProgramUniform1i( shader.program_gl, shader.u_texture_environment, GFX_TEXTURE_UNIT_ENVIRONMENT );
  glProgramUniform1i( shader.program_gl, shader.u_texture_irradiance_map, GFX_TEXTURE_UNIT_IRRADIANCE );
  glProgramUniform1i( shader.program_gl, shader.u_texture_prefilter_map, GFX_TEXTURE_UNIT_PREFILTER );
  glProgramUniform1i( shader.program_gl, shader.u_texture_brdf_lut, GFX_TEXTURE_UNIT_BRDF_LUT );

  bool linked = true;
  int params  = -1;
  glGetProgramiv( shader.program_gl, GL_LINK_STATUS, &params );
  if ( GL_TRUE != params ) {
    fprintf( stderr, "ERROR: could not link shader program GL index %u\n", shader.program_gl );
    int max_length    = 2048;
    int actual_length = 0;
    char plog[2048];
    glGetProgramInfoLog( shader.program_gl, max_length, &actual_length, plog );
    fprintf( stderr, "program info log for GL index %u:\n%s", shader.program_gl, plog );
    gfx_delete_shader_program( &shader );
    linked = false;
  }
  // fall back to default shader
  if ( !res_a || !res_b || !linked ) { return shader; } // should return default shader here
  shader.loaded = true;
  return shader;
}

void gfx_delete_shader_program( gfx_shader_t* shader ) {
  assert( shader );
  if ( shader->program_gl ) { glDeleteProgram( shader->program_gl ); }
  *shader = ( gfx_shader_t ){ .program_gl = 0 };
}

bool gfx_uniform1f( gfx_shader_t shader, int uniform_location, float f ) {
  if ( !shader.program_gl ) { return false; }
  if ( uniform_location < 0 ) { return false; }
  glProgramUniform1f( shader.program_gl, uniform_location, f );
  return true;
}

bool gfx_uniform3f( gfx_shader_t shader, int uniform_location, float x, float y, float z ) {
  if ( !shader.program_gl ) { return false; }
  if ( uniform_location < 0 ) { return false; }
  glProgramUniform3f( shader.program_gl, uniform_location, x, y, z );
  return true;
}

bool gfx_uniform4f( gfx_shader_t shader, int uniform_location, float x, float y, float z, float w ) {
  if ( !shader.program_gl ) { return false; }
  if ( uniform_location < 0 ) { return false; }
  glProgramUniform4f( shader.program_gl, uniform_location, x, y, z, w );
  return true;
}

void gfx_update_texture_sub_image( gfx_texture_t* texture, const uint8_t* img_buffer, int x_offs, int y_offs, int w, int h ) {
  assert( texture && texture->handle_gl ); // NOTE: it is valid for pixels to be NULL

  GLenum format = GL_RGBA;
  glPixelStorei( GL_UNPACK_ALIGNMENT, 4 );
  switch ( texture->n_channels ) {
  case 4: {
    format = GL_RGBA;
  } break;
  case 3: {
    format = texture->properties.is_bgr ? GL_BGR : GL_RGB;
    glPixelStorei( GL_UNPACK_ALIGNMENT, 1 ); // for small 1-channel npot images and framebuffer reading
  } break;
  case 2: {
    format = GL_RG;
    glPixelStorei( GL_UNPACK_ALIGNMENT, 1 ); // for small 1-channel npot images and framebuffer reading
  } break;
  case 1: {
    format = GL_RED;
    glPixelStorei( GL_UNPACK_ALIGNMENT, 1 ); // for small 1-channel npot images and framebuffer reading
  } break;
  default: {
    fprintf( stderr, "WARNING: unhandled texture channel number: %i\n", texture->n_channels );
    gfx_delete_texture( texture );
    return;
    // TODO(Anton) - when have default texture: *texture = g_default_texture
    return;
  } break;
  } // endswitch
  // NOTE can use 32-bit, GL_FLOAT depth component for eg DOF
  if ( texture->properties.is_depth ) { format = GL_DEPTH_COMPONENT; }

  glActiveTexture( GL_TEXTURE0 );
  glBindTexture( GL_TEXTURE_2D, texture->handle_gl );
  glTexSubImage2D( GL_TEXTURE_2D, 0, x_offs, y_offs, w, h, format, GL_UNSIGNED_BYTE, img_buffer );
  glBindTexture( GL_TEXTURE_2D, 0 );
}

void gfx_update_texture( gfx_texture_t* texture, const uint8_t* img_buffer, int w, int h, int n_channels ) {
  assert( texture && texture->handle_gl ); // NOTE: it is valid for pixels to be NULL
  assert( 4 == n_channels || 3 == n_channels || 2 == n_channels || 1 == n_channels );
  texture->w          = w;
  texture->h          = h;
  texture->n_channels = n_channels;

  GLint internal_format = GL_RGBA;
  GLenum format         = GL_RGBA;
  GLenum type           = GL_UNSIGNED_BYTE;
  glPixelStorei( GL_UNPACK_ALIGNMENT, 4 );
  switch ( texture->n_channels ) {
  case 4: {
    internal_format = texture->properties.is_srgb ? GL_SRGB_ALPHA : GL_RGBA;
    format          = texture->properties.is_bgr ? GL_BGRA : GL_RGBA;
    if ( texture->properties.is_hdr ) {
      internal_format = GL_RGBA16F;
      type            = GL_FLOAT;
    }
  } break;
  case 3: {
    internal_format = texture->properties.is_srgb ? GL_SRGB : GL_RGB;
    format          = texture->properties.is_bgr ? GL_BGR : GL_RGB;
    if ( texture->properties.is_hdr ) {
      internal_format = GL_RGB16F;
      type            = GL_FLOAT;
    }
    glPixelStorei( GL_UNPACK_ALIGNMENT, 1 ); // for small 1-channel npot images and framebuffer reading
  } break;
  case 2: {
    internal_format = GL_RG;
    format          = GL_RG;
    if ( texture->properties.is_hdr ) {
      internal_format = GL_RG16F;
      type            = GL_FLOAT;
    }
    glPixelStorei( GL_UNPACK_ALIGNMENT, 1 ); // for small 1-channel npot images and framebuffer reading
  } break;
  case 1: {
    internal_format = GL_RED;
    format          = GL_RED;
    if ( texture->properties.is_hdr ) {
      internal_format = GL_RG16F;
      type            = GL_FLOAT;
    }
    glPixelStorei( GL_UNPACK_ALIGNMENT, 1 ); // for small 1-channel npot images and framebuffer reading
  } break;
  default: {
    fprintf( stderr, "WARNING: unhandled texture channel number: %i\n", texture->n_channels );
    gfx_delete_texture( texture );
    // TODO(Anton) - when have default texture: *texture = g_default_texture
    return;
  } break;
  } // endswitch

  // NOTE can use 32-bit, GL_FLOAT depth component for eg DOF
  if ( texture->properties.is_depth ) {
    internal_format = GL_DEPTH_COMPONENT;
    format          = GL_DEPTH_COMPONENT;
  }

  glActiveTexture( GL_TEXTURE0 );
  glBindTexture( GL_TEXTURE_2D, texture->handle_gl );
  glTexImage2D( GL_TEXTURE_2D, 0, internal_format, texture->w, texture->h, 0, format, type, img_buffer );
  if ( texture->properties.repeats ) {
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT );
  } else {
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );
  }
  if ( texture->properties.bilinear ) {
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
  } else {
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
  }
  if ( texture->properties.has_mips ) {
    glGenerateMipmap( GL_TEXTURE_2D );
    if ( texture->properties.bilinear ) {
      glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR );
    } else {
      glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_LINEAR );
    }
    glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, 4.0f );
  } else if ( texture->properties.bilinear ) {
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
  } else {
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
  }

  glBindTexture( GL_TEXTURE_2D, 0 );
}

gfx_texture_t gfx_create_texture_from_mem( const uint8_t* img_buffer, int w, int h, int n_channels, gfx_texture_properties_t properties ) {
  assert( 4 == n_channels || 3 == n_channels || 2 == n_channels || 1 == n_channels );
  gfx_texture_t texture = ( gfx_texture_t ){ .properties = properties };
  glGenTextures( 1, &texture.handle_gl );
  gfx_update_texture( &texture, img_buffer, w, h, n_channels );
  return texture;
}

gfx_texture_t gfx_create_cube_texture_from_mem( uint8_t* imgs_buffer[6], int w, int h, int n_channels, gfx_texture_properties_t properties ) {
  assert( 4 == n_channels || 3 == n_channels || 1 == n_channels ); // 2 not used yet so not impl
  gfx_texture_t texture      = ( gfx_texture_t ){ .properties = properties };
  texture.properties.is_cube = true;

  GLint internal_format = GL_RGBA;
  GLenum format         = GL_RGBA;
  GLenum type           = GL_UNSIGNED_BYTE;
  if ( 4 == n_channels ) {
    if ( properties.is_hdr ) {
      internal_format = GL_RGBA16F;
      type            = GL_FLOAT;
    }
  } else if ( 3 == n_channels ) {
    internal_format = GL_RGB;
    format          = GL_RGB;
    if ( properties.is_hdr ) {
      internal_format = GL_RGB16F;
      type            = GL_FLOAT;
    }
  } else if ( 2 == n_channels ) {
    internal_format = GL_RG;
    format          = GL_RG;
    if ( properties.is_hdr ) {
      internal_format = GL_RG16F;
      type            = GL_FLOAT;
    }
  } else if ( 1 == n_channels ) {
    internal_format = GL_RED;
    format          = GL_RED;
  }

  glGenTextures( 1, &texture.handle_gl );
  glBindTexture( GL_TEXTURE_CUBE_MAP, texture.handle_gl );
  {
    for ( int i = 0; i < 6; i++ ) {
      uint8_t* img_ptr = imgs_buffer != NULL ? imgs_buffer[i] : NULL;
      glTexImage2D( GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, internal_format, w, h, 0, format, type, img_ptr );
    }
    // format cube map texture
    glTexParameteri( GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
    glTexParameteri( GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
    glTexParameteri( GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE );
    glTexParameteri( GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
    glTexParameteri( GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );
  }

  if ( properties.has_mips ) {
    if ( properties.cube_max_mip_level > 0 ) {
      glTexParameteri( GL_TEXTURE_CUBE_MAP, GL_TEXTURE_BASE_LEVEL, 0 );
      glTexParameteri( GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAX_LEVEL, properties.cube_max_mip_level );
    }
    if ( properties.bilinear ) {
      glTexParameteri( GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR );
    } else {
      glTexParameteri( GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_LINEAR );
    }
    glGenerateMipmap( GL_TEXTURE_CUBE_MAP );
  }

  glBindTexture( GL_TEXTURE_CUBE_MAP, 0 );
  return texture;
}

void gfx_delete_texture( gfx_texture_t* texture ) {
  assert( texture && texture->handle_gl );
  glDeleteTextures( 1, &texture->handle_gl );
  memset( texture, 0, sizeof( gfx_texture_t ) );
}

void gfx_draw_mesh( gfx_mesh_t mesh, gfx_primitive_type_t pt, gfx_shader_t shader, float* P, float* V, float* M, gfx_texture_t* textures, int n_textures ) {
  if ( 0 == mesh.points_vbo || 0 == mesh.n_vertices ) { return; }

  GLenum mode = GL_TRIANGLES;
  if ( pt == GFX_PT_TRIANGLE_STRIP ) { mode = GL_TRIANGLE_STRIP; }
  if ( pt == GFX_PT_POINTS ) { mode = GL_POINTS; }

  for ( int i = 0; i < n_textures; i++ ) {
    GLenum tex_type = !textures[i].properties.is_array ? GL_TEXTURE_2D : GL_TEXTURE_2D_ARRAY;
    if ( textures[i].properties.is_cube ) { tex_type = GL_TEXTURE_CUBE_MAP; }
    glActiveTexture( GL_TEXTURE0 + i );
    glBindTexture( tex_type, textures[i].handle_gl );
  }

  glUseProgram( shader.program_gl );
  glProgramUniformMatrix4fv( shader.program_gl, shader.u_P, 1, GL_FALSE, P );
  glProgramUniformMatrix4fv( shader.program_gl, shader.u_V, 1, GL_FALSE, V );
  glProgramUniformMatrix4fv( shader.program_gl, shader.u_M, 1, GL_FALSE, M );
  glBindVertexArray( mesh.vao );
  if ( mesh.indices_vbo ) {
    GLenum type = GL_UNSIGNED_BYTE;
    switch ( mesh.indices_type ) {
    case GFX_INDICES_TYPE_UBYTE: type = GL_UNSIGNED_BYTE; break;
    case GFX_INDICES_TYPE_UINT16: type = GL_UNSIGNED_SHORT; break;
    case GFX_INDICES_TYPE_UINT32: type = GL_UNSIGNED_INT; break;
    default: break;
    }
    glDrawElements( mode, mesh.n_indices, type, NULL );
  } else {
    glDrawArrays( mode, 0, mesh.n_vertices );
  }
  glBindVertexArray( 0 );
  glUseProgram( 0 );
  for ( int i = 0; i < n_textures; i++ ) {
    GLenum tex_type = !textures[i].properties.is_array ? GL_TEXTURE_2D : GL_TEXTURE_2D_ARRAY;
    if ( textures[i].properties.is_cube ) { tex_type = GL_TEXTURE_CUBE_MAP; }
    glActiveTexture( GL_TEXTURE0 + i );
    glBindTexture( tex_type, 0 );
  }
}

void gfx_wireframe_mode() { glPolygonMode( GL_FRONT_AND_BACK, GL_LINE ); }

void gfx_polygon_mode() { glPolygonMode( GL_FRONT_AND_BACK, GL_FILL ); }

double gfx_get_time_s() { return glfwGetTime(); }

bool gfx_framebuffer_status( gfx_framebuffer_t fb ) {
  glBindFramebuffer( GL_FRAMEBUFFER, fb.handle_gl );
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
      fprintf( stderr, "glCheckFramebufferStatus unspecified error\n" );
    }
    glBindFramebuffer( GL_FRAMEBUFFER, 0 );
    return false;
  }
  glBindFramebuffer( GL_FRAMEBUFFER, 0 );
  return true;
}

void gfx_rebuild_framebuffer( gfx_framebuffer_t* fb, int w, int h ) {
  assert( fb );
  if ( w <= 0 || h <= 0 ) { return; } // can happen during alt+tab
  assert( fb->handle_gl > 0 );
  assert( fb->has_cubemap || fb->output_texture.handle_gl > 0 );
  assert( fb->depth_texture.handle_gl > 0 );

  fb->built = false; // invalidate
  fb->w     = w;
  fb->h     = h;

  glBindFramebuffer( GL_FRAMEBUFFER, fb->handle_gl );
  {
    if ( !fb->has_cubemap ) {
      gfx_update_texture( &fb->output_texture, NULL, fb->w, fb->h, fb->output_texture.n_channels );
      glFramebufferTexture2D( GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, fb->output_texture.handle_gl, 0 );
    }
    gfx_update_texture( &fb->depth_texture, NULL, fb->w, fb->h, fb->depth_texture.n_channels );
    glFramebufferTexture2D( GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, fb->depth_texture.handle_gl, 0 );

    GLenum draw_buf = GL_COLOR_ATTACHMENT0;
    glDrawBuffers( 1, &draw_buf );
  }
  glBindFramebuffer( GL_FRAMEBUFFER, 0 );
  if ( !gfx_framebuffer_status( *fb ) ) {
    fprintf( stderr, "ERROR : rebuilding framebuffer\n" );
    return;
  }
  fb->built = true; // validate
}

// NOTE(Anton) maybe later need some srgb settings in this function
gfx_framebuffer_t gfx_create_framebuffer( int w, int h, bool has_cubemap ) {
  gfx_framebuffer_t fb = ( gfx_framebuffer_t ){ .w = w, .h = h, .has_cubemap = true };

  glGenFramebuffers( 1, &fb.handle_gl );
  if ( !has_cubemap ) {
    fb.output_texture = ( gfx_texture_t ){ .w = w, .h = h, .n_channels = 4 };
    glGenTextures( 1, &fb.output_texture.handle_gl );
  }
  fb.depth_texture = ( gfx_texture_t ){ .w = w, .h = h, .n_channels = 4, .properties.is_depth = true };
  glGenTextures( 1, &fb.depth_texture.handle_gl );

  gfx_rebuild_framebuffer( &fb, w, h );
  return fb;
}

void gfx_bind_framebuffer( const gfx_framebuffer_t* fb ) {
  if ( !fb ) {
    glBindFramebuffer( GL_FRAMEBUFFER, 0 );
    return;
  }
  glBindFramebuffer( GL_FRAMEBUFFER, fb->handle_gl );
}

void gfx_framebuffer_bind_cube_face( gfx_framebuffer_t fb, gfx_texture_t tex, int face_idx, int mip_level ) {
  assert( fb.handle_gl && fb.built );

  gfx_bind_framebuffer( &fb );
  glFramebufferTexture2D( GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + face_idx, tex.handle_gl, mip_level );
}

void gfx_framebuffer_bind_texture( gfx_framebuffer_t fb, gfx_texture_t tex ) {
  assert( fb.handle_gl && fb.built );

  gfx_bind_framebuffer( &fb );
  glFramebufferTexture2D( GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tex.handle_gl, 0 );
}

void gfx_framebuffer_update_depth_texture_dims( gfx_framebuffer_t fb, int w, int h ) {
  gfx_update_texture( &fb.depth_texture, NULL, w, h, fb.depth_texture.n_channels );
  glFramebufferTexture2D( GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, fb.depth_texture.handle_gl, 0 );
}

void gfx_read_pixels( int x, int y, int w, int h, int n_channels, uint8_t* data ) {
  GLenum format = GL_RGB;
  if ( 4 == n_channels ) { format = GL_RGBA; }
  if ( 2 == n_channels ) {
    glPixelStorei( GL_PACK_ALIGNMENT, 1 );
    format = GL_RG;
  }

  glPixelStorei( GL_PACK_ALIGNMENT, 1 );   // for irregular display sizes in RGB
  glPixelStorei( GL_UNPACK_ALIGNMENT, 1 ); // affects glReadPixels and subsequent texture calls alignment format

  glReadBuffer( GL_COLOR_ATTACHMENT0 );
  glReadPixels( x, y, w, h, format, GL_UNSIGNED_BYTE, data ); // note can be eg float
}
