/* Drop-in Single Header PLY loader
C89

Todo:
* quads->tris supporting function or mode
* indices->flat buffers supporting function
* respect actual order of properties when building vertex buffers
* support >1 sub-mesh and respect ordering of elements
* malloc/free function pointer override
* fuzzing

History:
v0.1 - 27 Aug 2019 - Initial Code (only supports default Blender export with single submesh)
*/
#ifndef APG_PLY_IMPLEMENTATION_H_
#define APG_PLY_IMPLEMENTATION_H_

struct apg_ply_mesh_t {
  float* vertices;
  unsigned int* indices;
  unsigned int n_vertices;
  unsigned int n_vertex_comps; /* XYZ = 3, XYZ NxNyNz ST = 8 */
  unsigned int n_indices;
  int loaded; /* 1 indicates success */
};

struct apg_ply_mesh_t apg_ply_from_mem( void* ptr, unsigned long sz );
struct apg_ply_mesh_t apg_ply_from_file( const char* filename );
void apg_ply_free( struct apg_ply_mesh_t* mesh );

#endif

#ifdef APG_PLY_IMPLEMENTATION
#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define _APG_PLY_MAX_LINE_LEN 2048
#define _APG_PLY_MAX_TOKEN_LEN 128

struct apg_ply_file_t {
  void* data;
  size_t sz;  /* in bytes */
  int loaded; /* 1 indicates success */
};

struct apg_ply_hdr_t {
  unsigned int vertex_count;
  unsigned int vertex_property_count;
  unsigned int face_count;
  size_t end_offset; /* byte offset where body starts */
  int loaded;
};

static struct apg_ply_file_t _apg_ply_file_to_mem( const char* filename ) {
  struct apg_ply_file_t record;
  size_t nr = 0;

  memset( &record, 0, sizeof( struct apg_ply_file_t ) );
  if ( !filename ) { return record; }
  {
    FILE* fptr = fopen( filename, "rb" );
    if ( !fptr ) { return record; }
    fseek( fptr, 0L, SEEK_END );
    record.sz   = (size_t)ftell( fptr );
    record.data = malloc( record.sz );
    if ( !record.data ) {
      fclose( fptr );
      return record;
    }
    rewind( fptr );
    nr = fread( record.data, record.sz, 1, fptr );
    fclose( fptr );
  }
  if ( 1 != nr ) {
    free( record.data );
    return record;
  }
  record.loaded = 1;
  return record;
}

static size_t _apg_ply_skip_ws( const char* ptr, int curr_offset, size_t sz ) {
  size_t i;
  for ( i = curr_offset; i < sz; i++ ) {
    if ( !isspace( ptr[i] ) ) { break; }
  }
  return i;
}

static size_t _apg_ply_line_len( const char* ptr, int curr_offset, size_t sz ) {
  size_t i = curr_offset;
  for ( i = curr_offset; i < sz; i++ ) {
    if ( '\n' == ptr[i] ) { break; }
  }
  return i - curr_offset;
}

static size_t _apg_ply_read_line( const char* ptr, int curr_offset, size_t sz, char* line, size_t* len ) {
  line[0] = '\0';
  *len    = _apg_ply_line_len( ptr, curr_offset, sz );
  *len    = *len < _APG_PLY_MAX_LINE_LEN ? *len : _APG_PLY_MAX_LINE_LEN - 1;
  strncat( line, &ptr[curr_offset], *len );
  return _apg_ply_skip_ws( ptr, curr_offset + *len, sz );
}

static size_t _apg_ply_strtok( const char* ptr, int curr_offset, size_t sz, char* tok, size_t max_tok ) {
  size_t i;
  curr_offset = _apg_ply_skip_ws( ptr, curr_offset, sz );
  for ( i = 0; i + curr_offset < sz && i < max_tok; i++ ) {
    if ( isspace( ptr[curr_offset + i] ) ) { break; }
    tok[i] = ptr[curr_offset + i];
  }
  tok[i] = '\0';
  return curr_offset + i;
}

static struct apg_ply_hdr_t _apg_ply_parse_header( void* ptr, size_t sz ) {
  struct apg_ply_hdr_t hdr;
  char* ascii_ptr         = (char*)ptr;
  const size_t min_hdr_sz = 100;
  size_t offset           = 0;
  size_t line_len         = 0;
  char line[_APG_PLY_MAX_LINE_LEN];

  memset( &hdr, 0, sizeof( struct apg_ply_hdr_t ) );
  line[0] = '\0';

  if ( sz < min_hdr_sz ) { return hdr; }
  /* magic number */
  offset = _apg_ply_read_line( ascii_ptr, offset, sz, line, &line_len );
  if ( 'p' != line[0] || 'l' != line[1] || 'y' != line[2] ) { return hdr; }
  /* binary formats not supported yet */
  offset = _apg_ply_read_line( ascii_ptr, offset, sz, line, &line_len );
  if ( 0 != strncmp( line, "format ascii 1.0", strlen( "format ascii 1.0" ) ) ) { return hdr; }
  { /* read remaining header lines */
    char token_b[_APG_PLY_MAX_TOKEN_LEN], token_c[_APG_PLY_MAX_TOKEN_LEN];
    int end_header_len    = strlen( "end_header" );
    int comment_len       = strlen( "comment" );
    int element_len       = strlen( "element" );
    int property_len      = strlen( "property" );
    int vertex_len        = strlen( "vertex" );
    int face_len          = strlen( "face" );
    int in_vertex_element = 0;

    while ( offset < sz ) {
      offset = _apg_ply_read_line( ascii_ptr, offset, sz, line, &line_len );
      /* end_header */
      if ( 0 == strncmp( line, "end_header", end_header_len ) ) {
        hdr.end_offset = offset;
        hdr.loaded     = 1;
        return hdr;
      }
      /* comment */
      if ( 0 == strncmp( line, "comment", comment_len ) ) { continue; }
      /* element */
      if ( 0 == strncmp( line, "element", element_len ) ) {
        int n = sscanf( line, "element %s %s", token_b, token_c );
        if ( n < 2 ) { return hdr; }
        if ( 0 == strncmp( token_b, "vertex", vertex_len ) ) {
          hdr.vertex_count          = atoi( token_c );
          hdr.vertex_property_count = 0;
          in_vertex_element         = 1;
          /* TODO(Anton) reset vertex properties or start new submesh */
        } else if ( 0 == strncmp( token_b, "face", face_len ) ) {
          hdr.face_count    = atoi( token_c );
          in_vertex_element = 0;
          /* TODO(Anton) reset face properties or start new submesh */
        }
        continue;
      }
      /* property */
      if ( 0 == strncmp( line, "property", property_len ) ) {
        if ( 0 == strncmp( token_b, "list", _APG_PLY_MAX_TOKEN_LEN ) ) {
        } else {
          if ( in_vertex_element ) { hdr.vertex_property_count++; }
        }
      }
    } /* endwhile */
  }   /* endblock */
  return hdr;
}

void _apg_ply_parse_body( void* ptr, unsigned long sz, struct apg_ply_hdr_t hdr, struct apg_ply_mesh_t* mesh ) {
  size_t line_len = 0;
  char line[_APG_PLY_MAX_LINE_LEN];
  char token[_APG_PLY_MAX_TOKEN_LEN];
  char* ascii_ptr      = (char*)ptr;
  size_t offset        = hdr.end_offset;
  size_t curr_vert_cmp = 0, curr_face_idx = 0;
  const int indices_per_face = 3;
  int i, j;

  /* assume for now: always floats */
  mesh->n_vertices     = hdr.vertex_count;
  mesh->n_vertex_comps = hdr.vertex_property_count;
  mesh->vertices       = (float*)malloc( sizeof( float ) * mesh->n_vertices * mesh->n_vertex_comps );

  mesh->n_indices = hdr.face_count * indices_per_face;
  mesh->indices   = (unsigned int*)malloc( sizeof( unsigned int ) * mesh->n_indices );

  /* assume for now: verts first, faces next */
  for ( i = 0; i < hdr.vertex_count; i++ ) {
    int line_i = 0;
    offset     = _apg_ply_read_line( ascii_ptr, offset, sz, line, &line_len );
    for ( j = 0; j < hdr.vertex_property_count; j++ ) {
      line_i = _apg_ply_strtok( line, line_i, line_len, token, _APG_PLY_MAX_TOKEN_LEN );

      mesh->vertices[curr_vert_cmp++] = atof( token );
    }
  }
  for ( i = 0; i < hdr.face_count; i++ ) {
    int line_i = 0, n = 0;
    offset = _apg_ply_read_line( ascii_ptr, offset, sz, line, &line_len );
    line_i = _apg_ply_strtok( line, line_i, line_len, token, _APG_PLY_MAX_TOKEN_LEN );
    n      = atoi( token );
    /* TODO(Anton) handle quads -> tris here */
    if ( indices_per_face != n ) { return; }

    for ( j = 0; j < n; j++ ) {
      line_i = _apg_ply_strtok( line, line_i, line_len, token, _APG_PLY_MAX_TOKEN_LEN );

      mesh->indices[curr_face_idx++] = atoi( token );
    }
  }
  mesh->loaded = 1;
}

struct apg_ply_mesh_t apg_ply_from_mem( void* ptr, unsigned long sz ) {
  struct apg_ply_mesh_t mesh;
  struct apg_ply_hdr_t hdr;

  memset( &mesh, 0, sizeof( struct apg_ply_mesh_t ) );
  if ( !ptr || 0 == sz ) { return mesh; }

  hdr = _apg_ply_parse_header( ptr, sz );
  if ( !hdr.loaded ) { return mesh; }

  _apg_ply_parse_body( ptr, sz, hdr, &mesh );

  return mesh;
}

struct apg_ply_mesh_t apg_ply_from_file( const char* filename ) {
  struct apg_ply_file_t record;
  struct apg_ply_mesh_t mesh;

  memset( &mesh, 0, sizeof( struct apg_ply_mesh_t ) );
  record = _apg_ply_file_to_mem( filename );
  if ( !record.loaded ) { return mesh; }
  mesh = apg_ply_from_mem( record.data, record.sz );
  free( record.data );

  return mesh;
}

void apg_ply_free( struct apg_ply_mesh_t* mesh ) {
  if ( !mesh ) { return; }
  if ( mesh->vertices ) { free( mesh->vertices ); }
  if ( mesh->indices ) { free( mesh->indices ); }
  memset( mesh, 0, sizeof( struct apg_ply_mesh_t ) );
}

#endif
