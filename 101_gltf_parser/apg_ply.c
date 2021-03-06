#include "apg_ply.h"
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define APG_PLY_MAX_STR 1024

unsigned int apg_ply_write( const char* filename, apg_ply_t ply ) {
  if ( !filename ) { return 0; }
  if ( !ply.positions_ptr || ply.n_vertices <= 0 ) { return 0; }
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
    // vertex
    for ( uint32_t v = 0; v < ply.n_vertices; v++ ) {
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
    for ( uint32_t i = 0; i < ply.n_vertices / 3; i++ ) { fprintf( fptr, "3 %i %i %i\n", i * 3, i * 3 + 1, i * 3 + 2 ); }
  } // endblock HEADER
  fclose( fptr );
  return 1;
}

#define APG_PLY_N_PROPERTY_NAMES 20

typedef struct _vertex_t {
  float pos[3];
  uint8_t colour[4];
  float edge[6];
  float nor[3];
  float st[2];
} _vertex_t;

typedef struct _face_t {
  uint32_t vert_idxs[4];
  int n_verts;
} _face_t;

unsigned int apg_ply_read( const char* filename, apg_ply_t* ply ) {
  if ( !filename || !ply ) { return 0; }

  const char property_names[APG_PLY_N_PROPERTY_NAMES][APG_PLY_MAX_STR] = {
    "x", "y", "z",                           // 0,1,2
    "nx", "ny", "nz",                        // 3,4,5
    "red", "green", "blue", "alpha",         // 6,7,8,9
    "s", "t", "u", "v",                      // 10,11,12,13
    "n_x", "p_x", "n_y", "p_y", "n_z", "p_z" // 14,15,16,17,18.19
  };
  apg_ply_t mesh = ( apg_ply_t ){ .n_vertices = 0 };
  {
    FILE* f_ptr = fopen( filename, "r" );
    if ( !f_ptr ) {
      fprintf( stderr, "ERROR loading mesh file from `%s`\n", filename );
      return 0;
    }

    char line[APG_PLY_MAX_STR];
    int n_verts = 0, n_faces = 0;
    int n_vertex_properties = 0;
    { // HEADER
      if ( !fgets( line, APG_PLY_MAX_STR, f_ptr ) ) {
        fclose( f_ptr );
        return 0;
      }
      if ( line[0] != 'p' || line[1] != 'l' || line[2] != 'y' ) {
        fprintf( stderr, "ERROR loading mesh file from `%s` - invalid file header or not a PLY.\n", filename );
        fclose( f_ptr );
        return 0;
      }
      if ( !fgets( line, APG_PLY_MAX_STR, f_ptr ) ) {
        fclose( f_ptr );
        return 0;
      }
      if ( strncmp( line, "format ascii", 12 ) != 0 ) {
        fprintf( stderr, "ERROR: loading mesh file from `%s` - format string was not `format ascii`\n", filename );
        fclose( f_ptr );
        return 0;
      }
      bool has_list = false;
      while ( fgets( line, APG_PLY_MAX_STR, f_ptr ) ) {
        if ( strncmp( line, "comment", 7 ) == 0 ) { continue; }
        if ( strncmp( line, "element", 7 ) == 0 ) {
          char type[APG_PLY_MAX_STR] = { 0 };
          int n_elements             = 0;
          if ( sscanf( line, "element %s %i", type, &n_elements ) != 2 ) {
            fprintf( stderr, "ERROR: element line in `%s` did not match expected number of arguments:\n`%s`\n", filename, line );
            fclose( f_ptr );
            return 0;
          }
          if ( strncmp( type, "vertex", 6 ) == 0 ) {
            n_vertex_properties = 0; // reset counter between sections
            n_verts             = n_elements;
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
              n_vertex_properties++;
              break;
            }
          } // endfor
          continue;
        } // endif property
        if ( strncmp( line, "end_header", 10 ) == 0 ) { break; }
      } // endwhile HEADER
      if ( !has_list ) {
        fprintf( stderr, "ERROR: no property list found in headers\n" );
        fclose( f_ptr );
        return 0;
      }
    } // endblock HEADER
    if ( n_faces < 1 || n_verts < 3 ) {
      fprintf( stderr, "ERROR: not enough face/vertex data for 1 triangle\n" );
      fclose( f_ptr );
      return 0;
    }
    { // BODY
      if ( n_vertex_properties != 8 && n_vertex_properties != 10 && n_vertex_properties != 11 && n_vertex_properties != 12 ) {
        fprintf( stderr, "WARNING: n_vertex_properties=%i\n", n_vertex_properties );
      }
      assert( n_vertex_properties == 8 || n_vertex_properties == 10 || n_vertex_properties == 11 || n_vertex_properties == 12 ); // only xyz rgb -x+x-y+y
                                                                                                                                 // implemented so far
      _vertex_t* v_list = malloc( sizeof( _vertex_t ) * n_verts );
      if ( !v_list ) {
        fprintf( stderr, "ERROR malloc returned NULL\n" );
        fclose( f_ptr );
        return 0;
      }
      _face_t* f_list = malloc( sizeof( _face_t ) * n_faces );
      if ( !f_list ) {
        fprintf( stderr, "ERROR malloc returned NULL\n" );
        fclose( f_ptr );
        free( v_list );
        return 0;
      }
      for ( int v = 0; v < n_verts; v++ ) {
        if ( !fgets( line, APG_PLY_MAX_STR, f_ptr ) ) {
          fprintf( stderr, "ERROR corrupted file: could not read expected vertex line\n" );
          fclose( f_ptr );
          free( v_list );
          free( f_list );
        }
        float x = 0, y = 0, z = 0, nx = 0, ny = 0, nz = 0, s = 0, t = 0, edge_n_x = 0, edge_p_x = 0, edge_n_y = 0, edge_p_y = 0;
        uint32_t red = 0, green = 0, blue = 0, alpha = 0;
        if ( 8 == n_vertex_properties ) { // xyz nnn st
          int n = sscanf( line, "%f %f %f %f %f %f %f %f", &x, &y, &z, &nx, &ny, &nz, &s, &t );
          assert( n == 8 );

          v_list[v].pos[0] = x;
          v_list[v].pos[1] = y;
          v_list[v].pos[2] = z;
          v_list[v].nor[0] = nx;
          v_list[v].nor[1] = ny;
          v_list[v].nor[2] = nz;
          v_list[v].st[0]  = s;
          v_list[v].st[1]  = t;
        } else if ( 10 == n_vertex_properties ) {
          int n = sscanf( line, "%f %f %f %u %u %u %f %f %f %f", &x, &y, &z, &red, &green, &blue, &edge_n_x, &edge_p_x, &edge_n_y, &edge_p_y );
          assert( n == 10 );

          v_list[v].pos[0]    = x;
          v_list[v].pos[1]    = y;
          v_list[v].pos[2]    = z;
          v_list[v].colour[0] = (uint8_t)red;
          v_list[v].colour[1] = (uint8_t)green;
          v_list[v].colour[2] = (uint8_t)blue;
          v_list[v].edge[0]   = edge_n_x;
          v_list[v].edge[1]   = edge_p_x;
          v_list[v].edge[2]   = edge_n_y;
          v_list[v].edge[3]   = edge_p_y;
        } else if ( 11 == n_vertex_properties ) {
          // x y z nx ny nz s t red green blue
          int n = sscanf( line, "%f %f %f %f %f %f %f %f %u %u %u", &x, &y, &z, &nx, &ny, &nz, &s, &t, &red, &green, &blue );
          assert( n == 11 );

          v_list[v].pos[0]    = x;
          v_list[v].pos[1]    = y;
          v_list[v].pos[2]    = z;
          v_list[v].nor[0]    = nx;
          v_list[v].nor[1]    = ny;
          v_list[v].nor[2]    = nz;
          v_list[v].st[0]     = s;
          v_list[v].st[1]     = t;
          v_list[v].colour[0] = (uint8_t)red;
          v_list[v].colour[1] = (uint8_t)green;
          v_list[v].colour[2] = (uint8_t)blue;
        } else if ( 12 == n_vertex_properties ) { // blender exports this
          // x y z nx ny nz s t red green blue alpha
          int n = sscanf( line, "%f %f %f %f %f %f %f %f %u %u %u %u", &x, &y, &z, &nx, &ny, &nz, &s, &t, &red, &green, &blue, &alpha );
          assert( n == 12 );

          v_list[v].pos[0]    = x;
          v_list[v].pos[1]    = y;
          v_list[v].pos[2]    = z;
          v_list[v].nor[0]    = nx;
          v_list[v].nor[1]    = ny;
          v_list[v].nor[2]    = nz;
          v_list[v].st[0]     = s;
          v_list[v].st[1]     = t;
          v_list[v].colour[0] = (uint8_t)red;
          v_list[v].colour[1] = (uint8_t)green;
          v_list[v].colour[2] = (uint8_t)blue;
          v_list[v].colour[3] = (uint8_t)alpha;
        }
      } // endfor vertices
      int final_vertex_count = 0;
      for ( int f = 0; f < n_faces; f++ ) {
        if ( !fgets( line, APG_PLY_MAX_STR, f_ptr ) ) {
          fprintf( stderr, "ERROR corrupted file: could not read expected face line\n" );
          fclose( f_ptr );
          free( v_list );
          free( f_list );
        }

        int nv = 0, v0 = -1, v1 = -1, v2 = -1, v3 = -1;
        int n = sscanf( line, "%i %i %i %i %i", &nv, &v0, &v1, &v2, &v3 );
        if ( nv == 3 ) {
          if ( n - 1 != nv ) {
            fprintf( stderr, "ERROR mismatched vertex count %i and vertices scanned %i in polygon\n", nv, n );
            fclose( f_ptr );
            free( v_list );
            free( f_list );
          }
          final_vertex_count += 3;
        } else if ( nv == 4 ) {
          if ( n - 1 != nv ) {
            fprintf( stderr, "ERROR mismatched vertex count %i and vertices scanned %iin polygon\n", nv, n );
            fclose( f_ptr );
            free( v_list );
            free( f_list );
          }
          final_vertex_count += 6; // split into 2 triangles
        } else {
          fprintf( stderr, "ERROR unhandled vertex count %i in polygon\n", nv );
          fclose( f_ptr );
          free( v_list );
          free( f_list );
        }
        f_list[f].n_verts      = nv;
        f_list[f].vert_idxs[0] = v0;
        f_list[f].vert_idxs[1] = v1;
        f_list[f].vert_idxs[2] = v2;
        f_list[f].vert_idxs[3] = v3;
      } // endfor faces

      // TODO actually count components to support alpha etc
      int hack               = 2; // HACK *2. this is a screw-up for 4-vert faces
      mesh.n_vertices        = final_vertex_count;
      mesh.n_positions_comps = 3;
      mesh.n_normals_comps   = 0;
      mesh.n_texcoords_comps = 0;
      mesh.n_edges_comps     = 4;
      mesh.n_colours_comps   = 3;
      if ( n_vertex_properties == 8 ) {
        mesh.n_normals_comps   = 3;
        mesh.n_texcoords_comps = 2;
        mesh.n_edges_comps     = 0;
        mesh.n_colours_comps   = 0;
      } else if ( n_vertex_properties == 11 ) {
        mesh.n_normals_comps   = 3;
        mesh.n_texcoords_comps = 2;
        mesh.n_edges_comps     = 0;
      } else if ( n_vertex_properties == 12 ) {
        mesh.n_normals_comps   = 3;
        mesh.n_texcoords_comps = 2;
        mesh.n_edges_comps     = 0;
        mesh.n_colours_comps   = 4;
      }
      mesh.positions_ptr = malloc( sizeof( float ) * mesh.n_positions_comps * mesh.n_vertices * hack );
      assert( mesh.positions_ptr );
      if ( mesh.n_normals_comps > 0 ) {
        mesh.normals_ptr = malloc( sizeof( float ) * mesh.n_normals_comps * mesh.n_vertices * hack );
        assert( mesh.normals_ptr );
      }
      if ( mesh.n_colours_comps > 0 ) {
        mesh.colours_ptr = malloc( sizeof( unsigned char ) * mesh.n_colours_comps * mesh.n_vertices * hack );
        assert( mesh.colours_ptr );
      }

      if ( mesh.n_texcoords_comps > 0 ) {
        mesh.texcoords_ptr = malloc( sizeof( float ) * mesh.n_texcoords_comps * mesh.n_vertices * hack );
        assert( mesh.texcoords_ptr );
      }
      if ( mesh.n_edges_comps > 0 ) {
        mesh.edges_ptr = malloc( sizeof( float ) * mesh.n_edges_comps * mesh.n_vertices * hack );
        assert( mesh.edges_ptr );
      }
      int v_counter = 0;
      for ( int f = 0; f < n_faces; f++ ) {
        for ( int v = 0; v < 3; v++ ) {
          int i = f_list[f].vert_idxs[v];
          for ( uint32_t c = 0; c < mesh.n_positions_comps; c++ ) { mesh.positions_ptr[( v_counter + v ) * mesh.n_positions_comps + c] = v_list[i].pos[c]; }
          for ( uint32_t c = 0; c < mesh.n_normals_comps; c++ ) { mesh.normals_ptr[( v_counter + v ) * mesh.n_normals_comps + c] = v_list[i].nor[c]; }
          for ( uint32_t c = 0; c < mesh.n_colours_comps; c++ ) { mesh.colours_ptr[( v_counter + v ) * mesh.n_colours_comps + c] = v_list[i].colour[c]; }
          for ( uint32_t c = 0; c < mesh.n_texcoords_comps; c++ ) { mesh.texcoords_ptr[( v_counter + v ) * mesh.n_texcoords_comps + c] = v_list[i].st[c]; }
          for ( uint32_t c = 0; c < mesh.n_edges_comps; c++ ) { mesh.edges_ptr[( v_counter + v ) * mesh.n_edges_comps + c] = v_list[i].edge[c]; }
        } // endfor v
        v_counter += 3;
        // TODO(Anton) untested order -- should be ABC, CDA
        if ( f_list[f].n_verts == 4 ) {
          for ( int v = 0; v < 4; v++ ) {
            int order = ( v + 2 ) % 4; // CDA
            int i     = f_list[f].vert_idxs[order];
            for ( uint32_t c = 0; c < mesh.n_positions_comps; c++ ) { mesh.positions_ptr[( v_counter + v ) * mesh.n_positions_comps + c] = v_list[i].pos[c]; }
            for ( uint32_t c = 0; c < mesh.n_normals_comps; c++ ) { mesh.normals_ptr[( v_counter + v ) * mesh.n_normals_comps + c] = v_list[i].nor[c]; }
            for ( uint32_t c = 0; c < mesh.n_colours_comps; c++ ) { mesh.colours_ptr[( v_counter + v ) * mesh.n_colours_comps + c] = v_list[i].colour[c]; }
            for ( uint32_t c = 0; c < mesh.n_texcoords_comps; c++ ) { mesh.texcoords_ptr[( v_counter + v ) * mesh.n_texcoords_comps + c] = v_list[i].st[c]; }
            for ( uint32_t c = 0; c < mesh.n_edges_comps; c++ ) { mesh.edges_ptr[( v_counter + v ) * mesh.n_edges_comps + c] = v_list[i].edge[c]; }
          }
          v_counter += 3;
        } // endfor v
      }   // endfor f

      free( v_list );
      free( f_list );
    } // endblock BODY
    fclose( f_ptr );
  } // endblock FILE

  // TODO set mesh stuff
  *ply = mesh;

  return 1;
}

void apg_ply_free( apg_ply_t* ply ) {
  assert( ply );
  if ( !ply ) { return; }

  if ( ply->positions_ptr ) { free( ply->positions_ptr ); }
  if ( ply->normals_ptr ) { free( ply->normals_ptr ); }
  if ( ply->colours_ptr ) { free( ply->colours_ptr ); }
  if ( ply->texcoords_ptr ) { free( ply->texcoords_ptr ); }
  if ( ply->edges_ptr ) { free( ply->edges_ptr ); }
  memset( ply, 0, sizeof( apg_ply_t ) );
}
