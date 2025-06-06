/* TODO(Anton)
 * add other solids
 * array of normals for each solid
   NOTE that because faces all point away from origin both the face normal and the per-vertex normal are intrinsic to the solid
   and .: can probably just be derived in the shaders w/o a buffer
 * array of texture coordinates for each solid
 * dice textures!
 * simplify representation somehow
 * ~ maybe a mesh struct and a draw_elements function or so.
 * D&D background image
 */

#include "gfx.h"
#include "glcontext.h"
#include "platonic.h"

int main( void ) {
  if ( !gfx_start( "The Platonic Solids", NULL, false ) ) { return 1; }

  GLuint cube_vbo, cube_vao, cube_index_buffer;
  { // make a cube
    glGenBuffers( 1, &cube_vbo );
    glGenVertexArrays( 1, &cube_vao );
    glBindVertexArray( cube_vao );
    glEnableVertexAttribArray( 0 );
    glBindBuffer( GL_ARRAY_BUFFER, cube_vbo );
    glBufferData( GL_ARRAY_BUFFER, 8 * 3 * sizeof( float ), platonic_cube_vertices_xyz, GL_STATIC_DRAW );
    glVertexAttribPointer( 0, 3, GL_FLOAT, GL_FALSE, 0, NULL );
    glBindBuffer( GL_ARRAY_BUFFER, 0 );
    glBindVertexArray( 0 );

    glGenBuffers( 1, &cube_index_buffer );
    glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, cube_index_buffer );
    glBufferData( GL_ELEMENT_ARRAY_BUFFER, 36 * sizeof( uint32_t ), platonic_cube_indices_ccw_triangles, GL_STATIC_DRAW );
    glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, 0 );
  }
  GLuint d4_vbo, d4_vao, d4_index_buffer;
  { // make a d4
    glGenBuffers( 1, &d4_vbo );
    glGenVertexArrays( 1, &d4_vao );
    glBindVertexArray( d4_vao );
    glEnableVertexAttribArray( 0 );
    glBindBuffer( GL_ARRAY_BUFFER, d4_vbo );
    glBufferData( GL_ARRAY_BUFFER, 4 * 3 * sizeof( float ), platonic_tetrahedron_vertices_xyz, GL_STATIC_DRAW );
    glVertexAttribPointer( 0, 3, GL_FLOAT, GL_FALSE, 0, NULL );
    glBindBuffer( GL_ARRAY_BUFFER, 0 );
    glBindVertexArray( 0 );

    glGenBuffers( 1, &d4_index_buffer );
    glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, d4_index_buffer );
    glBufferData( GL_ELEMENT_ARRAY_BUFFER, 4 * 3 * sizeof( uint32_t ), platonic_tetrahedron_indices_ccw_triangles, GL_STATIC_DRAW );
    glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, 0 );
  }
  GLuint d8_vbo, d8_vao, d8_index_buffer;
  { // make a d8
    glGenBuffers( 1, &d8_vbo );
    glGenVertexArrays( 1, &d8_vao );
    glBindVertexArray( d8_vao );
    glEnableVertexAttribArray( 0 );
    glBindBuffer( GL_ARRAY_BUFFER, d8_vbo );
    glBufferData( GL_ARRAY_BUFFER, 6 * 3 * sizeof( float ), platonic_octahedron_vertices_xyz, GL_STATIC_DRAW );
    glVertexAttribPointer( 0, 3, GL_FLOAT, GL_FALSE, 0, NULL );
    glBindBuffer( GL_ARRAY_BUFFER, 0 );
    glBindVertexArray( 0 );

    glGenBuffers( 1, &d8_index_buffer );
    glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, d8_index_buffer );
    glBufferData( GL_ELEMENT_ARRAY_BUFFER, 8 * 3 * sizeof( uint32_t ), platonic_octahedron_indices_ccw_triangles, GL_STATIC_DRAW );
    glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, 0 );
  }
  GLuint d12_vbo, d12_vao, d12_index_buffer;
  { // make a d12
    glGenBuffers( 1, &d12_vbo );
    glGenVertexArrays( 1, &d12_vao );
    glBindVertexArray( d12_vao );
    glEnableVertexAttribArray( 0 );
    glBindBuffer( GL_ARRAY_BUFFER, d12_vbo );
    glBufferData( GL_ARRAY_BUFFER, 20 * 3 * sizeof( float ), platonic_dodecahedron_vertices_xyz, GL_STATIC_DRAW );
    glVertexAttribPointer( 0, 3, GL_FLOAT, GL_FALSE, 0, NULL );
    glBindBuffer( GL_ARRAY_BUFFER, 0 );
    glBindVertexArray( 0 );

    glGenBuffers( 1, &d12_index_buffer );
    glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, d12_index_buffer );
    glBufferData( GL_ELEMENT_ARRAY_BUFFER, 36 * 3 * sizeof( uint32_t ), platonic_dodecahedron_indices_ccw_triangles, GL_STATIC_DRAW );
    glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, 0 );
  }
  GLuint d20_vbo, d20_vao, d20_index_buffer;
  { // make a d20
    glGenBuffers( 1, &d20_vbo );
    glGenVertexArrays( 1, &d20_vao );
    glBindVertexArray( d20_vao );
    glEnableVertexAttribArray( 0 );
    glBindBuffer( GL_ARRAY_BUFFER, d20_vbo );
    glBufferData( GL_ARRAY_BUFFER, 12 * 3 * sizeof( float ), platonic_isocahedron_vertices_xyz, GL_STATIC_DRAW );
    glVertexAttribPointer( 0, 3, GL_FLOAT, GL_FALSE, 0, NULL );
    glBindBuffer( GL_ARRAY_BUFFER, 0 );
    glBindVertexArray( 0 );

    glGenBuffers( 1, &d20_index_buffer );
    glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, d20_index_buffer );
    glBufferData( GL_ELEMENT_ARRAY_BUFFER, 20 * 3 * sizeof( uint32_t ), platonic_isocahedron_indices_ccw_triangles, GL_STATIC_DRAW );
    glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, 0 );
  }

  while ( !gfx_should_window_close() ) {
    double curr_s = gfx_get_time_s();
    int fb_w = 0, fb_h = 0;
    gfx_framebuffer_dims( &fb_w, &fb_h );
    gfx_viewport( 0, 0, fb_w, fb_h );
    gfx_clear_colour_and_depth_buffers( 0.2f, 0.2f, 0.2f, 1.0f );

    mat4 P = perspective( 67.0f, fb_w / (float)fb_h, 0.1f, 100.0f );
    mat4 V = look_at( ( vec3 ){ 0, 0, 10 }, ( vec3 ){ 0, 0, 0 }, ( vec3 ){ 0, 1, 0 } );
    mat4 R = rot_y_deg_mat4( curr_s * 10 );

    gfx_backface_culling( false );
    gfx_wireframe_mode();

    glUseProgram( gfx_dice_shader.program_gl );
    glUniformMatrix4fv( gfx_dice_shader.u_P, 1, GL_FALSE, P.m );
    glUniformMatrix4fv( gfx_dice_shader.u_V, 1, GL_FALSE, V.m );

    mat4 T, M, S;

    // D4
    T = translate_mat4( ( vec3 ){ -4, 4.0, 0.0 } );
    M = mult_mat4_mat4( T, R );
    glUniformMatrix4fv( gfx_dice_shader.u_M, 1, GL_FALSE, M.m );
    glBindVertexArray( d4_vao );
    glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, d4_index_buffer );
    // NB can also just give the array here instead of binding index buffer
    glDrawElements( GL_TRIANGLES, 4 * 3, GL_UNSIGNED_INT, NULL );
    glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, 0 );
    glBindVertexArray( 0 );

    // D6
    T = translate_mat4( ( vec3 ){ 4, 4.0, 0.0 } );
    M = mult_mat4_mat4( T, R );
    glUniformMatrix4fv( gfx_dice_shader.u_M, 1, GL_FALSE, M.m );
    glBindVertexArray( cube_vao );
    glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, cube_index_buffer );
    // NB can also just give the array here instead of binding index buffer
    glDrawElements( GL_TRIANGLES, 36, GL_UNSIGNED_INT, NULL );
    glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, 0 );
    glBindVertexArray( 0 );

    // D8
    T = translate_mat4( ( vec3 ){ -4, 0.0, 0.0 } );
    M = mult_mat4_mat4( T, R );
    glUniformMatrix4fv( gfx_dice_shader.u_M, 1, GL_FALSE, M.m );
    glBindVertexArray( d8_vao );
    glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, d8_index_buffer );
    // NB can also just give the array here instead of binding index buffer
    glDrawElements( GL_TRIANGLES, 8 * 3, GL_UNSIGNED_INT, NULL );
    glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, 0 );
    glBindVertexArray( 0 );

    // D12
    T = translate_mat4( ( vec3 ){ 4, 0.0, 0.0 } );
    M = mult_mat4_mat4( T, R );
    glUniformMatrix4fv( gfx_dice_shader.u_M, 1, GL_FALSE, M.m );
    glBindVertexArray( d12_vao );
    glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, d12_index_buffer );
    // NB can also just give the array here instead of binding index buffer
    glDrawElements( GL_TRIANGLES, 36 * 3, GL_UNSIGNED_INT, NULL );
    glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, 0 );
    glBindVertexArray( 0 );

    // D20
    S = scale_mat4( ( vec3 ){ 1.0f / 1.618034f, 1.0f / 1.618034f, 1.0f / 1.618034f } );
    T = translate_mat4( ( vec3 ){ -4, -4.0, 0.0 } );
    M = mult_mat4_mat4( T, mult_mat4_mat4( R, S ) );
    glUniformMatrix4fv( gfx_dice_shader.u_M, 1, GL_FALSE, M.m );
    glBindVertexArray( d20_vao );
    glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, d20_index_buffer );
    // NB can also just give the array here instead of binding index buffer
    glDrawElements( GL_TRIANGLES, 20 * 3, GL_UNSIGNED_INT, NULL );
    glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, 0 );
    glBindVertexArray( 0 );

    glUseProgram( 0 );

    gfx_poll_events();
    gfx_swap_buffer();
  }

  gfx_stop();
  return 0;
}
