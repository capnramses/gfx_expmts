#include "apg_ply.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

unsigned int apg_ply_write( const char* filename, apg_ply_t ply ) {
  if ( !filename ) { return 0; }
  if ( !ply.positions_ptr || ply.n_vertices == 0 ) { return 0; }
  if ( ply.n_positions_comps != 3 ) { return 0; }

  FILE* fptr = fopen( filename, "w" );
  if ( !fptr ) { return 0; }
  { // HEADER
    fprintf( fptr, "ply\nformat ascii 1.0\ncomment Exported with apg_ply by @capnramses\n" );
    fprintf( fptr, "element vertex %i\n", ply.n_vertices );

    fprintf( fptr, "property float x\nproperty float y\nproperty float z\n" );
    if ( 3 == ply.n_normals_comps ) { fprintf( fptr, "property float nx\nproperty float ny\nproperty float nz\n" ); }
    if ( 4 == ply.n_colours_comps ) {
      fprintf( fptr, "property uchar red\nproperty uchar green\nproperty uchar blue\nproperty uchar alpha\n" );
    } else if ( 3 == ply.n_colours_comps ) {
      fprintf( fptr, "property uchar red\nproperty uchar green\nproperty uchar blue\n" );
    }
    if ( 2 == ply.n_texcoords_comps ) { fprintf( fptr, "property float s\nproperty float t\n" ); }
    if ( 4 == ply.n_edges_comps ) { fprintf( fptr, "property float n_x\nproperty float p_x\nproperty float n_y\nproperty float p_y\n" ); }
    fprintf( fptr, "element face %i\nproperty list uchar uint vertex_indices\nend_header\n", ply.n_vertices / 3 );
  }
  { // BODY
    // vertices
    for ( int v = 0; v < ply.n_vertices; v++ ) {
      fprintf( fptr, "%.2f %.2f %.2f", ply.positions_ptr[v * 3 + 0], ply.positions_ptr[v * 3 + 1], ply.positions_ptr[v * 3 + 2] );
      if ( 3 == ply.n_normals_comps ) { fprintf( fptr, " %.2f %.2f %.2f", ply.normals_ptr[v * 3 + 0], ply.normals_ptr[v * 3 + 1], ply.normals_ptr[v * 3 + 2] ); }
      if ( 4 == ply.n_colours_comps ) {
        fprintf( fptr, " %u %u %u %u", ply.colours_ptr[v * 3 + 0], ply.colours_ptr[v * 3 + 1], ply.colours_ptr[v * 3 + 2], ply.colours_ptr[v * 3 + 3] );
      } else if ( 3 == ply.n_colours_comps ) {
        fprintf( fptr, " %u %u %u", ply.colours_ptr[v * 3 + 0], ply.colours_ptr[v * 3 + 1], ply.colours_ptr[v * 3 + 2] );
      }
      if ( 2 == ply.n_texcoords_comps ) { fprintf( fptr, " %f %f", ply.texcoords_ptr[v * 2 + 0], ply.texcoords_ptr[v * 2 + 1] ); }
      if ( 4 == ply.n_edges_comps ) {
        fprintf( fptr, " %.2f %.2f %.2f %.2f", ply.edges_ptr[v * 4 + 0], ply.edges_ptr[v * 4 + 1], ply.edges_ptr[v * 4 + 2], ply.edges_ptr[v * 4 + 3] );
      }
      fprintf( fptr, "\n" );
    }
    // faces
    for ( int i = 0; i < ply.n_vertices / 3; i++ ) { fprintf( fptr, "3 %i %i %i\n", i * 3, i * 3 + 1, i * 3 + 2 ); }
  }
  fclose( fptr );
  return 1;
}
