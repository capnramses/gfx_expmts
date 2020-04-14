#include "apg_ply.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define APG_PLY_MAX_STR 1024

unsigned int apg_ply_write( const char* filename, apg_ply_t ply ) {
  if ( !filename ) { return false; }
  if ( !ply.positions_ptr || ply.n_vertices <= 0 ) { return false; }
  if ( ply.n_positions_comps != 3 ) { return false; }

  FILE* fptr = fopen( filename, "w" );
  if ( !fptr ) { return false; }
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
    // vertex
    for ( unsigned int v = 0; v < ply.n_vertices; v++ ) {
      fprintf( fptr, "%f %f %f", ply.positions_ptr[v * 3 + 0], ply.positions_ptr[v * 3 + 1], ply.positions_ptr[v * 3 + 2] );
      if ( 3 == ply.n_normals_comps ) { fprintf( fptr, " %f %f %f", ply.normals_ptr[v * 3 + 0], ply.normals_ptr[v * 3 + 1], ply.normals_ptr[v * 3 + 2] ); }
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
    for ( unsigned int i = 0; i < ply.n_vertices / 3; i++ ) { fprintf( fptr, "3 %i %i %i\n", i * 3, i * 3 + 1, i * 3 + 2 ); }
  } // endblock HEADER
  fclose( fptr );
  return true;
}

#define APG_PLY_N_PROPERTY_NAMES 20

unsigned int apg_ply_read( const char* filename, apg_ply_t* ply ) {
  if ( !filename || !ply ) { return 1; }

  const char property_names[APG_PLY_N_PROPERTY_NAMES][APG_PLY_MAX_STR] = {
    "x", "y", "z",                           //
    "nx", "ny", "nz",                        //
    "red", "green", "blue", "alpha",         //
    "s", "t", "u", "v",                      //
    "n_x", "p_x", "n_y", "p_y", "n_z", "p_z" //
  };
  bool has_properties[APG_PLY_N_PROPERTY_NAMES] = { false };

  apg_ply_t mesh = ( apg_ply_t ){ .n_vertices = 0 };
  {
    FILE* f_ptr = fopen( filename, "r" );
    if ( !f_ptr ) {
      fprintf( stderr, "ERROR loading mesh file from `%s`\n", filename );
      return 1;
    }

    char line[APG_PLY_MAX_STR];
    { // HEADER
      if ( !fgets( line, APG_PLY_MAX_STR, f_ptr ) ) {
        fclose( f_ptr );
        return 1;
      }
      if ( line[0] != 'p' || line[1] != 'l' || line[2] != 'y' ) {
        fprintf( stderr, "ERROR loading mesh file from `%s` - invalid file header or not a PLY.\n", filename );
        fclose( f_ptr );
        return 1;
      }
      if ( !fgets( line, APG_PLY_MAX_STR, f_ptr ) ) {
        fclose( f_ptr );
        return 1;
      }
      if ( strncmp( line, "format ascii", 12 ) != 0 ) {
        fprintf( stderr, "ERROR: loading mesh file from `%s` - format string was not `format ascii`\n", filename );
        fclose( f_ptr );
        return 1;
      }
      int n_verts = 0, n_faces = 0;
      bool has_list = false;
      while ( fgets( line, APG_PLY_MAX_STR, f_ptr ) ) {
        if ( strncmp( line, "comment", 7 ) == 0 ) { continue; }
        if ( strncmp( line, "element", 7 ) == 0 ) {
          char type[APG_PLY_MAX_STR] = { 0 };
          int n_elements             = 0;
          if ( sscanf( line, "element %s %i", type, &n_elements ) != 2 ) {
            fprintf( stderr, "ERROR: element line in `%s` did not match expected number of arguments:\n`%s`\n", filename, line );
            fclose( f_ptr );
            return 1;
          }
          if ( strncmp( type, "vertex", 6 ) == 0 ) {
            n_verts = n_elements;
            continue;
          } else if ( strncmp( type, "face", 4 ) == 0 ) {
            n_faces = n_elements;
            continue;
          }
        }
        if ( strncmp( line, "property", 8 ) == 0 ) {
          char type[APG_PLY_MAX_STR] = { 0 }, name[APG_PLY_MAX_STR] = { 0 };
          int n = sscanf( line, "property %s %s", type, name );
          if ( n >= 1 && ( strncmp( type, "list", 4 ) == 0 ) ) {
            has_list = true;
            continue;
          }

          for ( int i = 0; i < APG_PLY_N_PROPERTY_NAMES; i++ ) {
            if ( n >= 2 && ( strcmp( name, property_names[i] ) == 0 ) ) {
              has_properties[i] = true;
              break;
            }
          } // endfor
        }   // endif property
      }     // endwhile BODY
    }       // endblock HEADER

    fclose( f_ptr );
  } // endblock FILE
  // TODO set mesh stuff
  *ply = mesh;

  return 0;
}
