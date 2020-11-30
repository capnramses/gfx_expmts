#include "gltf.h"
#define CGLTF_IMPLEMENTATION
#include "cgltf/cgltf.h"
// NB assuming already included
#include "stb/stb_image.h"

bool gltf_load( const char* filename, gltf_scene_t* gltf_scene_ptr ) {
  if ( !filename || !gltf_scene_ptr ) { return false; }

  char asset_path[2048];
  { // work out subfolder to find relative path to images etc
    asset_path[0]  = '\0';
    int last_slash = -1;
    int len        = strlen( filename );
    for ( int i = 0; i < len; i++ ) {
      if ( filename[i] == '\\' || filename[i] == '/' ) { last_slash = i; }
    }
    if ( last_slash > -1 ) {
      strncpy( asset_path, filename, last_slash + 1 );
      asset_path[last_slash + 1] = '\0';
      printf( "using asset path: `%s`\n", asset_path );
      for ( int i = 0; i < last_slash + 1; i++ ) {
        if ( asset_path[i] == '\\' ) { asset_path[i] = '/'; }
      }
    }
  }

  // format mirrors glTF spec https://github.com/KhronosGroup/glTF/tree/master/specification/2.0
  cgltf_data* data_ptr  = NULL;
  cgltf_options options = { 0 };

  // parses both gltf and glb according to docs in header
  // must free data pointer if succeeds
  // contents of external files for buffers and images are not loaded. URIs are in `cgltf_data`.
  // cgltf_options - zero initialised structure to trigger default behaviour
  cgltf_result res = cgltf_parse_file( &options, filename, &data_ptr );
  if ( cgltf_result_success != res ) {
    fprintf( stderr, "ERROR: loading GLTF file `%s`\n", filename );
    return false;
  }

  // populate data buffers with external files using FILE* API
  res = cgltf_load_buffers( &options, data_ptr, filename );
  if ( cgltf_result_success != res ) {
    fprintf( stderr, "ERROR: loading external resources for GLTF file `%s`\n", filename );
    cgltf_free( data_ptr );
    return false;
  }

  { // load images into a master list of scene textures. materials are just indices into this
    gltf_scene_ptr->n_textures = (uint32_t)data_ptr->images_count;
    printf( "%u textures\n", gltf_scene_ptr->n_textures );
    gltf_scene_ptr->textures_ptr = calloc( sizeof( gfx_texture_t ), gltf_scene_ptr->n_textures );

    for ( uint32_t i = 0; i < (uint32_t)gltf_scene_ptr->n_textures; i++ ) {
      if ( data_ptr->images[i].uri != NULL ) {
        char full_path[2048];
        strcpy( full_path, asset_path );
        strcat( full_path, data_ptr->images[i].uri );
        printf( "loading from %s\n", full_path );
        int x = 0, y = 0, comp = 0;
        unsigned char* img_ptr = stbi_load( full_path, &x, &y, &comp, 0 );
        if ( !img_ptr ) {
          fprintf( stderr, "ERROR loading image: `%s`\n", full_path );
          cgltf_free( data_ptr );
          return 1;
        }
      } else {
        printf( "UNHANDLED: image %i is embedded\n", i );
      }
    }
  }

  { // create gfx resources from gltf data
    gltf_scene_ptr->n_meshes = (uint32_t)data_ptr->meshes_count;
    printf( "allocating %u meshes\n", gltf_scene_ptr->n_meshes );
    gltf_scene_ptr->meshes_ptr = calloc( gltf_scene_ptr->n_meshes, sizeof( gfx_mesh_t ) );
    // In glTF, meshes are defined as arrays of primitives
    for ( uint32_t i = 0; i < gltf_scene_ptr->n_meshes; i++ ) {
      if ( data_ptr->meshes[i].primitives_count > 1 ) {
        fprintf( stderr, "ERROR: mesh %u has %u primitives and I assume 1 primitive == 1 mesh\n", i, (uint32_t)data_ptr->meshes[i].primitives_count );
        cgltf_free( data_ptr );
        return false;
      }

      // From spec:
      // Primitives specify one or more vertex attributes. Indexed primitives also define an indices property.
      // Each primitive also specifies a material and a primitive type that corresponds to the GPU primitive type (e.g., triangle set).
      // Implementation note: Splitting one mesh into primitives could be useful to limit number of indices per draw call.
      // If material is not specified, then a default material is used (base colour 1,1,1,1, metal and roughness set to 1).
      for ( uint32_t j = 0; j < (uint32_t)data_ptr->meshes[i].primitives_count; j++ ) {
        // get indices here
        cgltf_accessor* indices_acc_ptr = data_ptr->meshes[i].primitives[j].indices;
        uint32_t n_indices              = (uint32_t)indices_acc_ptr->count;
        size_t indices_sz               = n_indices * sizeof( uint16_t );
        gfx_indices_type_t indices_type = GFX_INDICES_TYPE_UINT16; // 1==uint16_t
        /* https://github.com/KhronosGroup/glTF-Tutorials/blob/master/gltfTutorial/gltfTutorial_005_BuffersBufferViewsAccessors.md
        The first accessor refers to the bufferView with index 0, which defines the part of the buffer data that contains the indices.
        Its type is "SCALAR", and its componentType is 5123 (UNSIGNED_SHORT). This means that the indices are stored as scalar unsigned short values.
        */
        uint8_t* indices_bytes_ptr = (uint8_t*)indices_acc_ptr->buffer_view->buffer->data;
        uint16_t* indices_ptr      = (uint16_t*)&indices_bytes_ptr[indices_acc_ptr->buffer_view->offset];
        float *points_ptr = NULL, *texcoords_ptr = NULL, *colours_ptr = NULL, *normals_ptr = NULL;
        uint32_t n_vertices = 0;

        for ( int k = 0; k < (int)data_ptr->meshes[i].primitives[j].attributes_count; k++ ) {
          printf( "mesh %i) primitive %i) `%s`::%i\n", i, j, data_ptr->meshes[i].primitives[j].attributes[k].name,
            data_ptr->meshes[i].primitives[j].attributes[k].index );

          if ( data_ptr->meshes[i].primitives[j].attributes[k].type == cgltf_attribute_type_position ) {
            cgltf_accessor* acc = data_ptr->meshes[i].primitives[j].attributes[k].data;
            n_vertices          = (int)acc->count; // note gltf position is a vec3 so this is number of vertices, not number of floats
            printf( "n verts = %u\n", n_vertices );
            uint8_t* bytes_ptr = (uint8_t*)acc->buffer_view->buffer->data;
            points_ptr         = (float*)&bytes_ptr[acc->buffer_view->offset];

// TODO first validate hasmin hasmax
#if 0
              float min_x   = acc->min[0];
              float min_y   = acc->min[1];
              float min_z   = acc->min[2];
              float max_x   = acc->max[0];
              float max_y   = acc->max[1];
              float max_z   = acc->max[2];
              float biggest = fabs( min_x );
              if ( fabs( min_y ) > biggest ) { biggest = fabs( min_y ); }
              if ( fabs( min_z ) > biggest ) { biggest = fabs( min_z ); }
              if ( fabs( max_x ) > biggest ) { biggest = fabs( max_x ); }
              if ( fabs( max_y ) > biggest ) { biggest = fabs( max_y ); }
              if ( fabs( max_z ) > biggest ) { biggest = fabs( max_z ); }
              scale_to_fit = 1.0 / biggest;
#endif

          } else if ( data_ptr->meshes[i].primitives[j].attributes[k].type == cgltf_attribute_type_texcoord ) {
            cgltf_accessor* acc = data_ptr->meshes[i].primitives[j].attributes[k].data;
            uint8_t* bytes_ptr  = (uint8_t*)acc->buffer_view->buffer->data;
            texcoords_ptr       = (float*)&bytes_ptr[acc->buffer_view->offset];
          } else if ( data_ptr->meshes[i].primitives[j].attributes[k].type == cgltf_attribute_type_color ) {
            cgltf_accessor* acc = data_ptr->meshes[i].primitives[j].attributes[k].data;
            uint8_t* bytes_ptr  = (uint8_t*)acc->buffer_view->buffer->data;
            colours_ptr         = (float*)&bytes_ptr[acc->buffer_view->offset];
          } else if ( data_ptr->meshes[i].primitives[j].attributes[k].type == cgltf_attribute_type_normal ) {
            cgltf_accessor* acc = data_ptr->meshes[i].primitives[j].attributes[k].data;
            uint8_t* bytes_ptr  = (uint8_t*)acc->buffer_view->buffer->data;
            normals_ptr         = (float*)&bytes_ptr[acc->buffer_view->offset];
          } // endif

        } // endfor k vertex attributes
        gltf_scene_ptr->meshes_ptr[i] =
          gfx_create_mesh_from_mem( points_ptr, 3, texcoords_ptr, 2, normals_ptr, 3, colours_ptr, 3, indices_ptr, indices_sz, indices_type, n_vertices, false );

        // set up material for mesh (link texture 'views' to loaded images as an index)
        gltf_scene_ptr->material = gltf_default_material();
        if ( data_ptr->meshes[i].primitives[j].material ) {
          if ( data_ptr->meshes[i].primitives[j].material->name ) {
            printf( "creating material `%s` for primitive\n", data_ptr->meshes[i].primitives[j].material->name );
            strncat( gltf_scene_ptr->material.name, data_ptr->meshes[i].primitives[j].material->name, 1023 );
          }

          // m_r
          if ( data_ptr->meshes[i].primitives[j].material->has_pbr_metallic_roughness ) {
            cgltf_pbr_metallic_roughness m_r = data_ptr->meshes[i].primitives[j].material->pbr_metallic_roughness;

            memcpy( &gltf_scene_ptr->material.base_colour_rgba, m_r.base_color_factor, sizeof( float ) * 4 );
            gltf_scene_ptr->material.metallic_f  = m_r.metallic_factor;
            gltf_scene_ptr->material.roughness_f = m_r.roughness_factor;

            cgltf_texture_view albedo_view      = m_r.base_color_texture;
            cgltf_texture_view metal_rough_view = m_r.metallic_roughness_texture;
            // NOTE(Anton) maybe there is a LUT for this to help speed up large scene loads
            for ( uint32_t i = 0; i < (uint32_t)gltf_scene_ptr->n_textures; i++ ) {
              if ( albedo_view.texture && albedo_view.texture->image ) {
                if ( &data_ptr->images[i] == albedo_view.texture->image ) { gltf_scene_ptr->material.base_colour_texture_idx = i; }
              }
              if ( metal_rough_view.texture && metal_rough_view.texture->image ) {
                if ( &data_ptr->images[i] == metal_rough_view.texture->image ) { gltf_scene_ptr->material.metal_roughness_texture_idx = i; }
              }
            }
          } // endif m_r

          cgltf_texture_view normal_view    = data_ptr->meshes[i].primitives[j].material->normal_texture;
          cgltf_texture_view occlusion_view = data_ptr->meshes[i].primitives[j].material->occlusion_texture;
          cgltf_texture_view emissive_view  = data_ptr->meshes[i].primitives[j].material->emissive_texture;
          for ( uint32_t i = 0; i < (uint32_t)gltf_scene_ptr->n_textures; i++ ) {
            if ( normal_view.texture && normal_view.texture->image ) {
              if ( &data_ptr->images[i] == normal_view.texture->image ) { gltf_scene_ptr->material.normal_texture_idx = i; }
            }
            if ( occlusion_view.texture && occlusion_view.texture->image ) {
              if ( &data_ptr->images[i] == occlusion_view.texture->image ) { gltf_scene_ptr->material.ambient_occlusion_texture_idx = i; }
            }
            if ( emissive_view.texture && emissive_view.texture->image ) {
              if ( &data_ptr->images[i] == emissive_view.texture->image ) { gltf_scene_ptr->material.emissive_texture_idx = i; }
            }
          }
          memcpy( &gltf_scene_ptr->material.emissive_rgb, data_ptr->meshes[i].primitives[j].material->emissive_factor, sizeof( float ) * 3 );
          gltf_scene_ptr->material.unlit = data_ptr->meshes[i].primitives[j].material->unlit;
          // NB(Anton) extensions can be here too

        } // endif material

      } // endfor j primitives
    }   // endfor i meshes
  }     // endblock gltf->gfx meshes

  // NOTE(Anton) assuming here that cgltf doesn't load images into buffers
  // TODO(Anton) doesn't work with embedded binary images -- no idea how this works yet.

  cgltf_free( data_ptr );
  return true;
}

bool gltf_free( gltf_scene_t* gltf_scene_ptr ) {
  if ( !gltf_scene_ptr || !gltf_scene_ptr->meshes_ptr ) { return false; }
  for ( uint32_t i = 0; i < gltf_scene_ptr->n_meshes; i++ ) { gfx_delete_mesh( &gltf_scene_ptr->meshes_ptr[i] ); }
  for ( uint32_t i = 0; i < gltf_scene_ptr->n_textures; i++ ) { gfx_delete_texture( &gltf_scene_ptr->textures_ptr[i] ); }
  free( gltf_scene_ptr->meshes_ptr );
  free( gltf_scene_ptr->textures_ptr );
  *gltf_scene_ptr = ( gltf_scene_t ){ .n_meshes = 0 };
  return true;
}

gltf_material_t gltf_default_material() {
  return ( gltf_material_t ){ // as defined in spec. texture index -1 means 'use the constant factors instead'
    .name[0]                       = '\0',
    .base_colour_texture_idx       = -1,
    .metal_roughness_texture_idx   = -1,
    .base_colour_rgba              = { 1.0f },
    .metallic_f                    = 1.0f,
    .roughness_f                   = 1.0f,
    .emissive_texture_idx          = -1,
    .emissive_rgb                  = { 0.0f },
    .ambient_occlusion_texture_idx = -1,
    .normal_texture_idx            = -1,
    .unlit                         = false
  };
}

void gltf_print_material_summary( gltf_material_t mat ) {
  printf( "material name:\t\t\t%s\n", mat.name );
  printf( "base_colour_texture_idx:\t%i\n", mat.base_colour_texture_idx );
  printf( "metal_roughness_texture_idx:\t%i\n", mat.metal_roughness_texture_idx );
  printf( "emissive_texture_idx:\t\t%i\n", mat.emissive_texture_idx );
  printf( "ambient_occlusion_texture_idx:\t%i\n", mat.ambient_occlusion_texture_idx );
  printf( "normal_texture_idx:\t\t%i\n", mat.normal_texture_idx );
  printf( "base_colour_rgba:\t\t%.2f %.2f %.2f %.2f\n", mat.base_colour_rgba[0], mat.base_colour_rgba[1], mat.base_colour_rgba[2], mat.base_colour_rgba[3] );
  printf( "metallic_f:\t\t\t%.2f\n", mat.metallic_f );
  printf( "roughness_f:\t\t\t%.2f\n", mat.roughness_f );
  printf( "emissive_rgb:\t\t\t%.2f %.2f %.2f\n", mat.emissive_rgb[0], mat.emissive_rgb[1], mat.emissive_rgb[2] );
  printf( "unlit:\t\t\t\t%i\n", (int)mat.unlit );
}
