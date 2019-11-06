#include "apg_ply.h"
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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
      fprintf( fptr, "property float red\nproperty float green\nproperty float blue\nalpha\n" );
    } else if ( 3 == ply.n_colours_comps ) {
      fprintf( fptr, "property float red\nproperty float green\nproperty float blue\n" );
    }
    if ( 2 == ply.n_texcoords_comps ) { fprintf( fptr, "property float s\nproperty float t\n" ); }
    fprintf( fptr, "element face %i\nproperty list uchar uint vertex_indices\nend_header\n", ply.n_vertices / 3 );
  }
  { // BODY
    // vertices
    for ( int v = 0; v < ply.n_vertices; v++ ) {
      fprintf( fptr, "%f %f %f", ply.positions_ptr[v * 3 + 0], ply.positions_ptr[v * 3 + 1], ply.positions_ptr[v * 3 + 2] );
      if ( 3 == ply.n_normals_comps ) { fprintf( fptr, " %f %f %f", ply.normals_ptr[v * 3 + 0], ply.normals_ptr[v * 3 + 1], ply.normals_ptr[v * 3 + 2] ); }
      if ( 4 == ply.n_colours_comps ) {
        fprintf( fptr, " %f %f %f %f", ply.colours_ptr[v * 3 + 0], ply.colours_ptr[v * 3 + 1], ply.colours_ptr[v * 3 + 2], ply.colours_ptr[v * 3 + 3] );
      } else if ( 3 == ply.n_colours_comps ) {
        fprintf( fptr, " %f %f %f", ply.colours_ptr[v * 3 + 0], ply.colours_ptr[v * 3 + 1], ply.colours_ptr[v * 3 + 2] );
      }
      if ( 2 == ply.n_texcoords_comps ) { fprintf( fptr, " %f %f", ply.texcoords_ptr[v * 2 + 0], ply.texcoords_ptr[v * 2 + 1] ); }
      fprintf( fptr, "\n" );
    }
    // faces
    for ( int i = 0; i < ply.n_vertices / 3; i++ ) { fprintf( fptr, "3 %i %i %i\n", i * 3, i * 3 + 1, i * 3 + 2 ); }
  }
  fclose( fptr );
  return true;
}

apg_ply_t apg_ply_read( const char* filename ) {
  assert( filename );
  apg_ply_t ply = ( apg_ply_t ){ .loaded = 0 };

  float *v_list = NULL, *v_finals = NULL;
  int n_finals = 0;
  int v_count = 0, f_count = 0;

  FILE* fptr = fopen( filename, "r" );
  if ( !fptr ) {
    fprintf( stderr, "ERROR: couldn't open ply file `%s` - is path correct?\n", filename );
    return ply;
  }
  char line[1024];
  { // hdr
    if ( !fgets( line, 1024, fptr ) ) {
      fprintf( stderr, "ERROR: 'fgets' failed reading file `%s`\n", filename );
      goto free_and_return_ply;
    }
    if ( line[0] != 'p' || line[1] != 'l' || line[2] != 'y' ) {
      fprintf( stderr, "ERROR: 'ply' magic number missing in file `%s`\n", filename );
      goto free_and_return_ply;
    }
    if ( !fgets( line, 1024, fptr ) ) {
      fprintf( stderr, "ERROR: 'fgets' failed reading file `%s`\n", filename );
      goto free_and_return_ply;
    }
    if ( 0 != strncmp( line, "format ascii", strlen( "format ascii" ) ) ) {
      fprintf( stderr, "ERROR: 'format ascii' magic number missing in file `%s`\n", filename );
      goto free_and_return_ply;
    }
    while ( fgets( line, 1024, fptr ) ) {
      if ( 0 == strncmp( line, "comment", strlen( "comment" ) ) ) { continue; }
      if ( 0 == strncmp( line, "element vertex", strlen( "element vertex" ) ) ) {
        if ( v_count ) { fprintf( stderr, "WARNING: more than 1 vertex section in ply file `%s`. Only 1 supported\n", filename ); }
        if ( 1 != sscanf( line, "element vertex %i", &v_count ) ) {
          fprintf( stderr, "ERROR: 'sscanf' got wrong number of params reading file `%s`\n", filename );
          goto free_and_return_ply;
        }
        continue;
      }
      if ( 0 == strncmp( line, "property float", strlen( "property float" ) ) || 0 == strncmp( line, "property uchar", strlen( "property uchar" ) ) ) {
        char term[64] = { 0 }, data_type[64] = { 0 };
        if ( 2 != sscanf( line, "property %s %s", data_type, term ) ) {
          fprintf( stderr, "ERROR: 'sscanf' got wrong number of params reading file `%s`\n", filename );
          goto free_and_return_ply;
        }
        if ( strcmp( term, "x" ) == 0 || strcmp( term, "y" ) == 0 || strcmp( term, "z" ) == 0 ) { ply.n_positions_comps++; }
        if ( strcmp( term, "nx" ) == 0 || strcmp( term, "ny" ) == 0 || strcmp( term, "nz" ) == 0 ) { ply.n_normals_comps++; }
        if ( strcmp( term, "s" ) == 0 || strcmp( term, "t" ) == 0 ) { ply.n_texcoords_comps++; }
        if ( strcmp( term, "red" ) == 0 || strcmp( term, "blue" ) == 0 || strcmp( term, "green" ) == 0 || strcmp( term, "alpha" ) == 0 ) {
          ply.n_colours_comps++;
        }
        continue;
      }

      if ( 0 == strncmp( line, "element face", strlen( "element face" ) ) ) {
        if ( f_count ) { fprintf( stderr, "WARNING: more than 1 face section. Only 1 supported\n" ); }
        if ( 1 != sscanf( line, "element face %i", &f_count ) ) {
          fprintf( stderr, "ERROR: 'sscanf' got wrong number of params reading file `%s`\n", filename );
          goto free_and_return_ply;
        }
        continue;
      }
      if ( 0 == strncmp( line, "property list", strlen( "property list" ) ) ) { continue; }
      if ( 0 == strncmp( line, "end_header", strlen( "end_header" ) ) ) { break; }
    } // endwhile
    if ( ( ply.n_positions_comps != 0 && ply.n_positions_comps != 3 ) || ( ply.n_texcoords_comps != 0 && ply.n_texcoords_comps != 2 ) ||
         ( ply.n_normals_comps != 0 && ply.n_normals_comps != 3 ) || ( ply.n_colours_comps != 0 && ply.n_colours_comps != 3 && ply.n_colours_comps != 4 ) ) {
      fprintf( stderr, "ERROR: unsupported count of vertex components\n" );
      goto free_and_return_ply;
    }
  }
  int total_n_comps = ply.n_positions_comps + ply.n_texcoords_comps + ply.n_normals_comps + ply.n_colours_comps;
  v_list            = malloc( v_count * total_n_comps * sizeof( float ) );
  v_finals          = malloc( 6 * f_count * total_n_comps * sizeof( float ) );
  { // BODY
    for ( int i = 0; i < v_count; i++ ) {
      if ( !fgets( line, 1024, fptr ) ) {
        fprintf( stderr, "ERROR: 'fgets' failed reading file `%s`\n", filename );
        goto free_and_return_ply;
      }
      float comps[12]; // x y z nx ny nz s t r g b a
      int n = sscanf( line, "%f %f %f %f %f %f %f %f %f %f %f %f", &comps[0], &comps[1], &comps[2], &comps[3], &comps[4], &comps[5], &comps[6], &comps[7],
        &comps[8], &comps[9], &comps[10], &comps[11] );
      if ( n != total_n_comps ) {
        fprintf( stderr, "ERROR: expected %i vertex components, got %i\n", total_n_comps, n );
        goto free_and_return_ply;
      }
      for ( int col_idx = 8; col_idx < n; col_idx++ ) { comps[col_idx] /= 255.0f; } // rgba are really uchars and should be converted to floats
      memcpy( &v_list[i * n], comps, n * sizeof( float ) );
    }
    // faces list
    for ( int i = 0; i < f_count; i++ ) {
      if ( !fgets( line, 1024, fptr ) ) {
        fprintf( stderr, "ERROR: 'fgets' failed reading file `%s`\n", filename );
        goto free_and_return_ply;
      }
      // NOTE(Anton) assumes this format: property list uchar uint vertex_indices
      uint32_t n_poly_verts = 0, a = 0, b = 0, c = 0, d = 0;
      int n = sscanf( line, "%u %u %u %u %u", &n_poly_verts, &a, &b, &c, &d ); // 4 0 1 2 3
      if ( ( n < 2 ) || ( 4 == n_poly_verts && n != 5 ) || ( 3 == n_poly_verts && n != 4 ) ) {
        fprintf( stderr, "ERROR: wrong number of components (%i) scanned in face line\n", n );
        goto free_and_return_ply;
      }
      // TODO(Anton) check winding order for quad/tri
      if ( 4 == n_poly_verts || 3 == n_poly_verts ) {
        int count          = 4 == n_poly_verts ? 6 : 3;
        uint32_t indices[] = { a, b, c, c, d, a };
        for ( int j = 0; j < count; j++ ) {
          int vert_idx = indices[j];
          memcpy( &v_finals[n_finals++ * total_n_comps], &v_list[vert_idx * total_n_comps], total_n_comps * sizeof( float ) );
        }
      } else {
        fprintf( stderr, "ERROR: unsupported number of vertices per polygon in a face. only 3 and 4 supported\n" );
        goto free_and_return_ply;
      }
    }
  }
  { // split finals into groups and allocate correct sizes
    // NOTE(Anton) could just use indexed rendering but would need a quads->tris split anyway
    if ( ply.n_positions_comps > 0 ) {
      ply.positions_ptr = malloc( sizeof( float ) * ply.n_positions_comps * n_finals );
      assert( ply.positions_ptr );
    }
    if ( ply.n_normals_comps > 0 ) {
      ply.normals_ptr = malloc( sizeof( float ) * ply.n_normals_comps * n_finals );
      assert( ply.normals_ptr );
    }
    if ( ply.n_texcoords_comps > 0 ) {
      ply.texcoords_ptr = malloc( sizeof( float ) * ply.n_texcoords_comps * n_finals );
      assert( ply.texcoords_ptr );
    }
    if ( ply.n_colours_comps > 0 ) {
      ply.colours_ptr = malloc( sizeof( float ) * ply.n_colours_comps * n_finals );
      assert( ply.colours_ptr );
    }
    for ( int i = 0; i < n_finals; i++ ) {
      int idx = 0;
      if ( ply.n_positions_comps > 0 ) {
        memcpy( &ply.positions_ptr[ply.n_positions_comps * i], &v_finals[i * total_n_comps + idx], sizeof( float ) * ply.n_positions_comps );
        idx += ply.n_positions_comps;
      }
      if ( ply.n_normals_comps > 0 ) {
        memcpy( &ply.normals_ptr[ply.n_normals_comps * i], &v_finals[i * total_n_comps + idx], sizeof( float ) * ply.n_normals_comps );
        idx += ply.n_normals_comps;
      }
      if ( ply.n_texcoords_comps > 0 ) {
        memcpy( &ply.texcoords_ptr[ply.n_texcoords_comps * i], &v_finals[i * total_n_comps + idx], sizeof( float ) * ply.n_texcoords_comps );
        idx += ply.n_texcoords_comps;
      }
      if ( ply.n_colours_comps > 0 ) {
        memcpy( &ply.colours_ptr[ply.n_colours_comps * i], &v_finals[i * total_n_comps + idx], sizeof( float ) * ply.n_colours_comps );
        idx += ply.n_colours_comps;
      }
    }
  }
  ply.n_vertices = n_finals;
  ply.loaded     = 1;
free_and_return_ply:
  fclose( fptr );
  if ( v_list ) { free( v_list ); }
  if ( v_finals ) { free( v_finals ); }
  return ply;
}

void apg_ply_delete( apg_ply_t* ply ) {
  assert( ply );
  if ( ply->positions_ptr ) { free( ply->positions_ptr ); }
  if ( ply->normals_ptr ) { free( ply->normals_ptr ); }
  if ( ply->texcoords_ptr ) { free( ply->texcoords_ptr ); }
  if ( ply->colours_ptr ) { free( ply->colours_ptr ); }
  *ply = ( apg_ply_t ){ .loaded = 0 };
}
