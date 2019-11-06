#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>

/* Limitations
* binary_little_endian and binary_big_endian are not supported.
* Only the first submesh is considered. Others are ignored.
* All vertex components are assumed to be float.
* The following vertex components are supported:
  x,y,z,nx,ny,nz,s,t,red,green,blue,alpha
* Components are optional, but the order is fixed.
  The following is valid:
  x, y, s, t, red, green, blue.
  But the following is not valid:
  s, t, x, y, z
* Edges are ignored.
* Custom material sections are ignored.
* Comments are discarded.
* Only triangular and quad faces are read.
* Quad faces are always converted to triangles.
*/

enum ply_vertex_comp_format_t {
  PLY_VERTEX_XYZ = 0,
  PLY_VERTEX_XYZ_NXNYNZ_ST_RGBA,
  PLY_VERTEX_XYZ_NXNYNZ_ST_RGB,
  PLY_VERTEX_XYZ_NXNYNZ_RGBA,
  PLY_VERTEX_XYZ_NXNYNZ_RGB,
  PLY_VERTEX_XYZ_NXNYNZ_ST,
  PLY_VERTEX_XYZ_NXNYNZ,
  PLY_VERTEX_XYZ_ST_RGBA,
  PLY_VERTEX_XYZ_ST_RGB,
  PLY_VERTEX_XYZ_ST,
  PLY_VERTEX_XYZ_RGBA,
  PLY_VERTEX_XYZ_RGB,
  PLY_VERTEX_MAX
};

struct ply_t {
  enum ply_vertex_comp_format_t vertex_format;
  float* vertex_data;
  int vertex_count;
  int n_vertex_attribs;
  int loaded; // 1 if there were no errors
};

struct ply_t ply_read( const char* filename ) {
  struct ply_t ply;
  memset( &ply, 0, sizeof( struct ply_t ) );

  FILE* fptr = fopen( filename, "r" );
  {
    char line[512];
    int n = 0;

    // -- header stuff --
    char magic_number[4]; // "ply" lowercase
    char format_type[16];
    float format_version;
    int vertex_list_count = 0, face_count = 0;
    { // parse header. lots of holes to explode with ascii string parsing in headers
      // for now only handle the first submesh and ignore the rest
      bool got_vertices = false, got_faces = false;
      bool has_xyz = false, has_nxnynz = false, has_st = false, has_rgb = false, has_rgba = false;

      // magic number and format ascii/binary assumed fixed in first and second line
      n = fscanf( fptr, "%s format %s %f\n", magic_number, format_type, &format_version );
      if ( 3 != n || 0 != strncmp( "ply", magic_number, 3 ) || 0 != strncmp( "ascii", format_type, 5 ) ) {
        fclose( fptr );
        return ply;
      }

      bool in_face_block = false, in_vert_block = false;
      while ( fgets( line, 512, fptr ) ) {
        if ( 0 == strncmp( "comment", line, 7 ) ) { continue; }
        if ( 0 == strncmp( "element", line, 7 ) ) {
          char element_type[32];
          int element_count;
          int n = sscanf( line, "element %s %i\n", element_type, &element_count );
          if ( 2 != n ) {
            fclose( fptr );
            return ply;
          }
          in_vert_block = false;
          in_face_block = false;
          printf( "%s el line\n", element_type );
          if ( !got_vertices && 0 == strncmp( "vertex", element_type, 6 ) ) {
            in_vert_block     = true;
            vertex_list_count = element_count;
            got_vertices      = true;
          } else if ( !got_faces && 0 == strncmp( "face", element_type, 4 ) ) {
            // finalise vertex properties
            {
              if ( !has_xyz ) {
                fclose( fptr );
                return ply;
              }
              ply.vertex_format = PLY_VERTEX_XYZ;
              if ( has_nxnynz ) {
                if ( has_st ) {
                  if ( has_rgba ) {
                    ply.vertex_format    = PLY_VERTEX_XYZ_NXNYNZ_ST_RGBA;
                    ply.n_vertex_attribs = 12;
                  } else if ( has_rgb ) {
                    ply.vertex_format    = PLY_VERTEX_XYZ_NXNYNZ_ST_RGB;
                    ply.n_vertex_attribs = 11;
                  } else {
                    ply.vertex_format    = PLY_VERTEX_XYZ_NXNYNZ_ST;
                    ply.n_vertex_attribs = 8;
                  }
                } else {
                  if ( has_rgba ) {
                    ply.vertex_format    = PLY_VERTEX_XYZ_NXNYNZ_RGBA;
                    ply.n_vertex_attribs = 10;
                  } else if ( has_rgb ) {
                    ply.vertex_format    = PLY_VERTEX_XYZ_NXNYNZ_RGB;
                    ply.n_vertex_attribs = 9;
                  } else {
                    ply.vertex_format    = PLY_VERTEX_XYZ_NXNYNZ;
                    ply.n_vertex_attribs = 6;
                  }
                }
              } else if ( has_st ) {
                if ( has_rgba ) {
                  ply.vertex_format    = PLY_VERTEX_XYZ_ST_RGBA;
                  ply.n_vertex_attribs = 9;
                } else if ( has_rgb ) {
                  ply.vertex_format    = PLY_VERTEX_XYZ_ST_RGB;
                  ply.n_vertex_attribs = 8;
                } else {
                  ply.vertex_format    = PLY_VERTEX_XYZ_ST;
                  ply.n_vertex_attribs = 5;
                }
              } else if ( has_rgba ) {
                ply.vertex_format    = PLY_VERTEX_XYZ_RGBA;
                ply.n_vertex_attribs = 7;
              } else if ( has_rgb ) {
                ply.vertex_format    = PLY_VERTEX_XYZ_RGB;
                ply.n_vertex_attribs = 6;
              }
            } // endblock vertex format
            in_face_block = true;
            face_count    = element_count;
            got_faces     = true;
          }
          // ignore 'element edge'
          continue;
        }
        if ( 0 == strncmp( "property", line, 8 ) ) {
          if ( !in_vert_block ) { continue; }
          { // vertex property
            char data_type[16];
            char element_name[64];
            int n = sscanf( line, "property %s %s\n", data_type, element_name );
            if ( 2 != n ) {
              fclose( fptr );
              return ply;
            }
            if ( 'x' == element_name[0] || 'y' == element_name[0] || 'z' == element_name[0] ) {
              has_xyz = true;
            } else if ( 's' == element_name[0] || 't' == element_name[0] ) {
              has_st = true;
            } else if ( 0 == strncmp( "nx", element_name, 2 ) || 0 == strncmp( "ny", element_name, 2 ) || 0 == strncmp( "nz", element_name, 2 ) ) {
              has_nxnynz = true;
            } else if ( 0 == strncmp( "red", element_name, 3 ) || 0 == strncmp( "green", element_name, 5 ) || 0 == strncmp( "blue", element_name, 4 ) ) {
              has_rgb = true;
            } else if ( 0 == strncmp( "alpha", element_name, 5 ) ) {
              has_rgba = true;
              continue;
            }
          }
          continue;
        }
        if ( 0 == strncmp( "end_header", line, 10 ) ) { break; }
      } // endwhile header variables

    } // endblock header
    { // ASCII body - vertex list then face list
      float* vertex_list = malloc( sizeof( float ) * vertex_list_count * ply.n_vertex_attribs );
      int* index_list    = malloc( sizeof( int ) * face_count * 3 * 2 ); // x2 in case we get all quads
      float comps[PLY_VERTEX_MAX];
      int face_comps[5];
      int triangle_count = 0;
      for ( int i = 0; i < vertex_list_count; i++ ) {
        if ( !fgets( line, 512, fptr ) ) {
          free( vertex_list );
          fclose( fptr );
          return ply;
        }
        n = sscanf( line, "%f %f %f %f %f %f %f %f %f %f %f %f\n", &comps[0], &comps[1], &comps[2], &comps[3], &comps[4], &comps[5], &comps[6], &comps[7],
          &comps[8], &comps[9], &comps[10], &comps[11] );
        if ( n != ply.n_vertex_attribs ) {
          free( vertex_list );
          free( index_list );
          fclose( fptr );
          return ply;
        }
        memcpy( &vertex_list[i * ply.n_vertex_attribs], comps, ply.n_vertex_attribs * sizeof( float ) );
      } // endfor

      for ( int i = 0; i < face_count; i++ ) {
        if ( !fgets( line, 512, fptr ) ) {
          free( vertex_list );
          free( index_list );
          fclose( fptr );
          return ply;
        }
        n = sscanf( line, "%i %i %i %i %i\n", &face_comps[0], &face_comps[1], &face_comps[2], &face_comps[3], &face_comps[4] );
        if ( ( face_comps[0] != 3 && face_comps[0] != 4 ) || ( face_comps[0] + 1 != n ) ) {
          free( vertex_list );
          free( index_list );
          fclose( fptr );
          return ply;
        }
        /*
        a +--+ b
          |\ |
          | \|
        d +--+ c
        */
        index_list[triangle_count * 3 + 0] = face_comps[1]; // a
        index_list[triangle_count * 3 + 1] = face_comps[2]; // b
        index_list[triangle_count * 3 + 2] = face_comps[3]; // c
        if ( 3 == face_comps[0] ) {
          triangle_count++;
        } else {
          index_list[triangle_count * 3 + 3] = face_comps[3]; // c
          index_list[triangle_count * 3 + 4] = face_comps[4]; // d
          index_list[triangle_count * 3 + 5] = face_comps[1]; // a
          triangle_count += 2;
        }
      }
      ply.vertex_count = triangle_count * 3;
      ply.vertex_data  = malloc( ply.vertex_count * ply.n_vertex_attribs * sizeof( float ) );
      for ( int i = 0; i < ply.vertex_count; i++ ) {
        int v        = index_list[i];
        int v_offset = v * ply.n_vertex_attribs;
        memcpy( &ply.vertex_data[i * ply.n_vertex_attribs], &vertex_list[v_offset], sizeof( float ) * ply.n_vertex_attribs );
      }

      free( vertex_list );
      free( index_list );
    }
  }
  fclose( fptr );

  ply.loaded = 1;
  return ply;
}

void ply_write( const char* filename, struct ply_t ply ) {
  FILE* fptr = fopen( filename, "w" );
  fprintf( fptr, "ply\nformat ascii 1.0\ncomment Created by Anton's ply writer\n" );
  fprintf( fptr, "element vertex %i\n", ply.vertex_count );
  fprintf( fptr,
    "property float x\nproperty float y\nproperty float z\nproperty float nx\nproperty float ny\nproperty float nz\nproperty float s\nproperty float t\n" );
  fprintf( fptr, "element face %i\nproperty list uchar uint vertex_indices\nend_header\n", ply.vertex_count / 3 );
  for ( int i = 0; i < ply.vertex_count; i++ ) {
    int idx = i * ply.n_vertex_attribs;
    fprintf( fptr, "%f %f %f %f %f %f %f %f\n", ply.vertex_data[idx], ply.vertex_data[idx + 1], ply.vertex_data[idx + 2], ply.vertex_data[idx + 3],
      ply.vertex_data[idx + 4], ply.vertex_data[idx + 5], ply.vertex_data[idx + 6], ply.vertex_data[idx + 7] );
  }
  for ( int i = 0; i < ply.vertex_count / 3; i++ ) { fprintf( fptr, "3 %i %i %i\n", i * 3, i * 3 + 1, i * 3 + 2 ); }
  fclose( fptr );
}

int main() {
  // read PLY
  struct ply_t ply = ply_read( "uv_sphere.ply" );
  if ( !ply.loaded ) {
    fprintf( stderr, "ERROR parsing PLY\n" );
    return 1;
  }
  printf( "vertex format: %i\n", ply.vertex_format );
  printf( "vertex count: %i\n", ply.vertex_count );

  // modify UVs
  for ( int i = 0; i < ply.vertex_count; i++ ) {
    int s_offs          = i * ply.n_vertex_attribs + 6;
    ply.vertex_data[s_offs] = ply.vertex_data[s_offs] < 0.5f ? 2.0f * ply.vertex_data[s_offs] : 1.0f - 2.0f * ( ply.vertex_data[s_offs] - 0.5f );
  }

  // write PLY
  ply_write( "out.ply", ply );

  return 0;
}
