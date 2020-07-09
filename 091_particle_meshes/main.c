// Particle System demo using instanced meshes for particles
// Anton Gerdelan <antongdl@protonmail.com> 9 July 2020

#include "apg/apg_gfx.h"

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

// particle_system_t particle_system_create( gfx_shader_t shader, gfx_mesh_t particle_mesh, int n_particles ) { return p; }

void particle_system_start( particle_system_t* particle_system ) {
  assert( particle_system );

  particle_system->system_elapsed_s = 0.0;

  // TODO(Anton) reset positions and velocities based on emitter type
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
  gfx_draw_mesh_instanced( particle_system->shader, P, V, particle_system->emitter_M, particle_system->mesh.vao, particle_system->mesh.n_vertices,
    particle_system->n_particles, NULL, 0 );
}

int main() {
  // TODO gfx_mesh_t particle_mesh = gfx_create_mesh_from_mem();
  // TODO gfx_mesh_gen_instanced_buffer(
  // TODO particle_system_t p = ( particle_system_t ){ .particle_mesh = particle_mesh, .n_particles = n_particles };
  return 0;
}
