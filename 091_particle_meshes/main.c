// Particle System demo using instanced meshes for particles
// Anton Gerdelan <antongdl@protonmail.com> 9 July 2020

#include "apg/apg_gfx.h"
#include "apg/apg_ply.h"
#include <stdio.h>

#define PARTICLES_MAX 128

// instanced particle system properties
typedef struct particle_system_t {
  gfx_mesh_t particle_mesh;
  gfx_shader_t shader;
  vec3 particles_pos_loc[PARTICLES_MAX];
  vec3 particles_vel_loc[PARTICLES_MAX];
  mat4 emitter_M;
  double system_elapsed_s;
  double system_duration_s;
  float accel_loc;
  int n_particles;
  bool is_running;
  bool loops;
  // TODO(Anton) system type
} particle_system_t;

particle_system_t particle_system_create( gfx_shader_t shader, gfx_mesh_t particle_mesh, int n_particles, vec3 emitter_pos_wor, double duration_s, bool loops ) {
  if ( n_particles >= PARTICLES_MAX ) {
    fprintf( stderr, "WARNING: particle_system_create() %i particles requested but max %i. Using max instead.\n", n_particles, PARTICLES_MAX );
  }

  particle_system_t ps = ( particle_system_t ){ .particle_mesh = particle_mesh,
    .shader                                                    = shader,
    .emitter_M                                                 = translate_mat4( emitter_pos_wor ),
    .system_elapsed_s                                          = 0.0,
    .system_duration_s                                         = duration_s,
    .accel_loc                                                 = -10.0f,
    .n_particles                                               = APG_M_MIN( n_particles, PARTICLES_MAX ),
    .is_running                                                = false,
    .loops                                                     = loops };

  for ( int i = 0; i < ps.n_particles; i++ ) {
    ps.particles_pos_loc[i] = ( vec3 ){ 0, 0, 0 };
    ps.particles_vel_loc[i] = ( vec3 ){ 0, 10, 0 };
  }

  return ps;
}

void particle_system_start( particle_system_t* particle_system ) {
  assert( particle_system );

  particle_system->system_elapsed_s = 0.0;
  particle_system->is_running       = true;

  // TODO(Anton) reset positions and velocities based on emitter type
}

void particle_system_stop( particle_system_t* particle_system ) {
  assert( particle_system );

  particle_system->is_running = false;
}

/*
void particle_system_update( particle_system_t* particle_system, double elapsed_s ) {
  assert( particle_system );

  for ( int part_idx = 0; part_idx < particle_system->n_particles; part_idx++ ) {
    particle_system->particles_vel_loc[i] += particle_system->accel_loc * elapsed_s;
    particle_system->particles_pos_loc[i] += particle_system->particles_vel_loc[i] * elapsed_s;
  }
  particle_system->system_elapsed_s += elapsed_s;
  if ( particle_system->system_elapsed_s >= system_duration_s ) {
    if (particle_system->loops) {
      particle_system_start( particle_system );
    } else {
      particle_system->is_running = false;
    }
  }
}
*/

void particle_system_draw( particle_system_t* particle_system, mat4 P, mat4 V ) {
  assert( particle_system );

  if ( !particle_system->is_running ) { return; }

  // TODO(Anton) bounding radius and distance check for system

  // TODO(Anton) number of instances should exclude particles not launched yet etc.
   gfx_draw_mesh_instanced( particle_system->shader, P, V, particle_system->emitter_M, particle_system->particle_mesh.vao,
    particle_system->particle_mesh.n_vertices, particle_system->n_particles, NULL, 0 );

  //gfx_draw_mesh( particle_system->shader, P, V, particle_system->emitter_M, particle_system->particle_mesh.vao, particle_system->particle_mesh.n_vertices, NULL, 0 );
}

int main() {
  printf( "Anton's particle thing\n" );
  gfx_start( "Anton's particle thing", false );

  apg_ply_t particle_ply = ( apg_ply_t ){ .n_vertices = 0 };
  if ( !apg_ply_read( "particle.ply", &particle_ply ) ) {
    printf( "ERROR: reading mesh file `particle.ply`\n" );
    gfx_stop();
    return 1;
  }

  gfx_shader_t ps_shader = gfx_create_shader_program_from_files( "particle.vert", "particle.frag" );
  gfx_mesh_t ps_mesh     = gfx_create_mesh_from_mem( particle_ply.positions_ptr, particle_ply.n_positions_comps, NULL, 0, NULL, 0, NULL, 0,
    particle_ply.normals_ptr, particle_ply.n_normals_comps, NULL, 0, NULL, 0, particle_ply.n_vertices, false );
  apg_ply_free( &particle_ply );

  particle_system_t ps = particle_system_create( ps_shader, ps_mesh, 1, ( vec3 ){ 0, 0, 0 }, 10.0, true );
  // TODO(anton) gfx_mesh_gen_instanced_buffer(

  particle_system_start( &ps );

  while ( !gfx_should_window_close() ) {
    gfx_poll_events();

    int w = 0, h = 0;
    gfx_framebuffer_dims( &w, &h );
    gfx_viewport( 0, 0, w, h );
    gfx_clear_colour_and_depth_buffers( 0.2, 0.2, 0.2, 1.0 );

    mat4 P = perspective( 67.0f, (float)w / (float)h, 0.01f, 1000.0f );
    mat4 V = look_at( ( vec3 ){ 0, 0, 10 }, ( vec3 ){ 0 }, ( vec3 ){ 0, 1, 0 } );

    particle_system_draw( &ps, P, V );

    gfx_swap_buffer();
  }

  particle_system_stop( &ps );

  gfx_delete_mesh( &ps_mesh );
  gfx_delete_shader_program( &ps_shader );
  gfx_stop();
  printf( "HALT\n" );
  return 0;
}
