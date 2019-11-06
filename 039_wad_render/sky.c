// WAD Rend - Copyright 2017 Anton Gerdelan <antonofnote@gmail.com>
// C99

// TODO -- cube isn't right as the height != width

#include "sky.h"
#include "wad.h"
#include <alloca.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>

#define SKY_VERTEX_SHADER_FILE "sky.vert"
#define SKY_FRAGMENT_SHADER_FILE "sky.frag"

typedef unsigned char byte_t;

static GLuint sky_vao, sky_program, sky_cube_tex;
static GLint sky_program_view_mat_location = -1, sky_program_proj_mat_location = -1,
             my_cube_texture_loc = -1;

static GLuint make_big_cube() {
  const float dim = 100.0f;
  const float height_dim = 100.0f;
  float points[] = { -dim, height_dim,  -dim, -dim, -height_dim, -dim, dim,  -height_dim, -dim,
                     dim,  -height_dim, -dim, dim,  height_dim,  -dim, -dim, height_dim,  -dim,

                     -dim, -height_dim, dim,  -dim, -height_dim, -dim, -dim, height_dim,  -dim,
                     -dim, height_dim,  -dim, -dim, height_dim,  dim,  -dim, -height_dim, dim,

                     dim,  -height_dim, -dim, dim,  -height_dim, dim,  dim,  height_dim,  dim,
                     dim,  height_dim,  dim,  dim,  height_dim,  -dim, dim,  -height_dim, -dim,

                     -dim, -height_dim, dim,  -dim, height_dim,  dim,  dim,  height_dim,  dim,
                     dim,  height_dim,  dim,  dim,  -height_dim, dim,  -dim, -height_dim, dim,

                     -dim, height_dim,  -dim, dim,  height_dim,  -dim, dim,  height_dim,  dim,
                     dim,  height_dim,  dim,  -dim, height_dim,  dim,  -dim, height_dim,  -dim,

                     -dim, -height_dim, -dim, -dim, -height_dim, dim,  dim,  -height_dim, -dim,
                     dim,  -height_dim, -dim, -dim, -height_dim, dim,  dim,  -height_dim, dim };
  GLuint vbo;
  glGenBuffers( 1, &vbo );
  glBindBuffer( GL_ARRAY_BUFFER, vbo );
  glBufferData( GL_ARRAY_BUFFER, 3 * 36 * sizeof( GLfloat ), &points,
                GL_STATIC_DRAW );

  GLuint vao;
  glGenVertexArrays( 1, &vao );
  glBindVertexArray( vao );
  glEnableVertexAttribArray( 0 );
  glBindBuffer( GL_ARRAY_BUFFER, vbo );
  glVertexAttribPointer( 0, 3, GL_FLOAT, GL_FALSE, 0, NULL );
  return vao;
}

static GLuint create_cube_map( byte_t *img_data, int x, int y ) {
  byte_t *white = alloca( x * y * 4 );
  memset(white, 0, x * y * 4);

  // generate a cube-map texture to hold all the sides
  GLuint tex_cube;
  glActiveTexture( GL_TEXTURE0 );
  glGenTextures( 1, &tex_cube );
  glBindTexture( GL_TEXTURE_CUBE_MAP, tex_cube );
  glTexImage2D( GL_TEXTURE_CUBE_MAP_POSITIVE_X, 0, GL_RGBA, x, y, 0, GL_RGBA,
                GL_UNSIGNED_BYTE, img_data );
  glTexImage2D( GL_TEXTURE_CUBE_MAP_NEGATIVE_X, 0, GL_RGBA, x, y, 0, GL_RGBA,
                GL_UNSIGNED_BYTE, img_data );
  glTexImage2D( GL_TEXTURE_CUBE_MAP_POSITIVE_Z, 0, GL_RGBA, x, y, 0, GL_RGBA,
                GL_UNSIGNED_BYTE, img_data );
  glTexImage2D( GL_TEXTURE_CUBE_MAP_NEGATIVE_Z, 0, GL_RGBA, x, y, 0, GL_RGBA,
                GL_UNSIGNED_BYTE, img_data );
  glTexImage2D( GL_TEXTURE_CUBE_MAP_POSITIVE_Y, 0, GL_RGBA, x, y, 0, GL_RGBA,
                GL_UNSIGNED_BYTE, white );
  glTexImage2D( GL_TEXTURE_CUBE_MAP_NEGATIVE_Y, 0, GL_RGBA, x, y, 0, GL_RGBA,
                GL_UNSIGNED_BYTE, white );
  // format cube map texture
  glTexParameteri( GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
  glTexParameteri( GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
  glTexParameteri( GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE );
  glTexParameteri( GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
  glTexParameteri( GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );
  glBindTexture( GL_TEXTURE_CUBE_MAP, 0 );

  return tex_cube;
}

static void square_picture( byte_t *input, int inwidth, int inheight,
                            byte_t *output ) {
  int row_multiple = inwidth / inheight;
  printf( "row mult = %i/%i = %i\n", inwidth, inheight, row_multiple );
  int rowbytes = inwidth * 4;
  for ( int i = 0; i < inheight; i++ ) {
    for ( int j = 0; j < row_multiple; j++ ) {
      for ( int x = 0; x < rowbytes; x++ ) {
        output[i * rowbytes * j + x] = input[i * rowbytes + x];
      }
    }
  }
}

void init_sky( const char* map_name ) {
  sky_vao = make_big_cube();
  sky_program =
    create_programme_from_files( SKY_VERTEX_SHADER_FILE, SKY_FRAGMENT_SHADER_FILE );
  sky_program_view_mat_location = glGetUniformLocation( sky_program, "view" );
  sky_program_proj_mat_location = glGetUniformLocation( sky_program, "proj" );
  my_cube_texture_loc = glGetUniformLocation( sky_program, "my_cube_texture" );
  glUseProgram( sky_program );
  glUniform1i( my_cube_texture_loc, 0 );

  { // sky texture
    byte_t *img_data = NULL;

    int width, height;
    char sky_name[32] = "SKY1";

    // TODO heretic has different sky res so the square thing won't work
    /*
     better idea - just load a 2d texture on a quad
     BUT
     scroll its s-coord based on the y rotation of the cam
     no need for perspective etc

     should always fit to screen vertically
     and horizonatal should scale based on aspect ratio vs image aspect

    */



    if (map_name[0] == 'E') {
      if (map_name[1] == '2') {
        strcpy(sky_name, "SKY2");
      } else if (map_name[1] == '3') {
        strcpy(sky_name, "SKY3");
      } else if (map_name[1] == '4') {
        strcpy(sky_name, "SKY4");
      }
    } else {
      int lev = 0;
      sscanf(map_name, "MAP%i", &lev);
      if (lev >= 21) {
        strcpy(sky_name, "RSKY3");
      } else if (lev >= 12) {
        strcpy(sky_name, "RSKY2");
      } else {
        strcpy(sky_name, "RSKY1");
      }
    }

    bool ret = get_wad_picture_dims( sky_name, &width, &height );
    printf( "sky %s w= %i h = %i\n", sky_name, width, height );
    if( !ret ) {
      fprintf(stderr, "ERROR: loading sky texture\n");
      return;
    } 

    img_data = (byte_t *)calloc( 1, width * height * 4 );
    assert( img_data );

    ret = load_picture_data( sky_name, img_data, width, height );
    assert( ret );

    byte_t *output_img = (byte_t *)calloc( 1, width * width * 4 );
    assert( output_img );

    square_picture( img_data, width, height, output_img );

    sky_cube_tex = create_cube_map( output_img, width, width );
    free( img_data );
    free( output_img );
    // assert (0 );
  }
}

void draw_sky( mat4 view_mat, mat4 proj_mat ) {
  glDepthMask( GL_FALSE );

  glUseProgram( sky_program );
  glUniformMatrix4fv( sky_program_view_mat_location, 1, GL_FALSE, view_mat.m );
  glUniformMatrix4fv( sky_program_proj_mat_location, 1, GL_FALSE, proj_mat.m );
  glActiveTexture( GL_TEXTURE0 );
  glBindTexture( GL_TEXTURE_CUBE_MAP, sky_cube_tex );
  glBindVertexArray( sky_vao );
  glDrawArrays( GL_TRIANGLES, 0, 36 );

  glDepthMask( GL_TRUE );
}
