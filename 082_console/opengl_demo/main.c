#include "apg_console.h"
#include "gfx.h"
#include "glcontext.h"
#include <assert.h>
#include <ctype.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static bool console_open = true;
static double console_state_time;

void character_callback( GLFWwindow* window, unsigned int codepoint ) {
  if ( '`' == codepoint ) {
    // console_open = false;
    return;
  }

  // hopefully this ignores non-ascii stuff which won't render
  if ( isprint( codepoint ) ) {
    char tmp[16];
    char lower_ascii = tolower( codepoint );
    sprintf( tmp, "%c", lower_ascii );
    apg_c_append_user_entered_text( tmp );
    return;
  }
  return;
}

int main() {
  bool ret = gfx_start( 1920, 1080, true, "console demo", NULL );
  assert( ret );

  // unicode codepoint callback for typing
  glfwSetCharCallback( g_window, character_callback );

  apg_c_print( "first line" );
  apg_c_print( "second line" );
  apg_c_print( "third line" );

  int w                        = 1920 / 2;
  int h                        = 1080 / 2; // 16 * ( APG_C_OUTPUT_LINES_MAX + 1 );
  int n_chans                  = 4;
  uint8_t console_background[] = { 0x22, 0x77, 0x11, 0xAA };
  uint8_t* img_ptr             = calloc( w * h * n_chans, 1 );
  apg_c_draw_to_image_mem( img_ptr, w, h, n_chans, console_background );

  int console_shader_idx        = gfx_create_managed_shader_from_files( "opengl_demo/console_text.vert", "opengl_demo/console_text.frag" );
  gfx_texture_t console_texture = gfx_load_image_mem_to_texture( img_ptr, w, h, n_chans, true, false, false, false, false );

  gfx_buffer_colour( NULL, 0.5f, 0.5f, 0.5f, 1.0f );
  double prev_s = gfx_get_time_s();
  while ( !gfx_should_window_close() ) {
    double curr_s    = gfx_get_time_s();
    double elapsed_s = curr_s - prev_s;
    prev_s           = curr_s;

    console_state_time    = console_state_time + elapsed_s < 0.5 ? console_state_time + elapsed_s : 0.5;
    float console_state_f = (float)console_state_time / 0.5f;

    // TODO(Anton) if apg_c_has_changed()
    apg_c_draw_to_image_mem( img_ptr, w, h, n_chans, console_background );
    gfx_update_texture( &console_texture, img_ptr );

    {
      gfx_clear_colour_and_depth_buffers();

      if ( console_open ) {
        float y = 2.0f - 1.0f * console_state_f;
        gfx_uniform_2f( gfx_managed_shaders[console_shader_idx], gfx_managed_shaders[console_shader_idx].u_pos_loc, 0.0f, y );
        gfx_draw_gfx_mesh_texturedv( gfx_ss_quad_mesh, gfx_managed_shaders[console_shader_idx], &console_texture, 1 );
      } else if ( console_state_f < 1.0f ) {
        float y = 2.0f - 1.0f * ( 1.0f - console_state_f );
        gfx_uniform_2f( gfx_managed_shaders[console_shader_idx], gfx_managed_shaders[console_shader_idx].u_pos_loc, 0.0f, y );
        gfx_draw_gfx_mesh_texturedv( gfx_ss_quad_mesh, gfx_managed_shaders[console_shader_idx], &console_texture, 1 );
      }
      gfx_swap_buffer_and_poll_events();
    }

    if ( console_open ) {
      // check for press of return key to finalise user-entered text
      static bool enter_held = false;
      if ( glfwGetKey( g_window, GLFW_KEY_ENTER ) ) {
        if ( !enter_held ) { apg_c_append_user_entered_text( "\n" ); }
        enter_held = true;
      } else {
        enter_held = false;
      }

      // backspace to delete last char
      static bool backspace_held = false;
      if ( glfwGetKey( g_window, GLFW_KEY_BACKSPACE ) ) {
        if ( !backspace_held ) { apg_c_backspace(); }
        backspace_held = true;
      } else {
        backspace_held = false;
      }
    }

    static bool backtic_held = false;
    if ( glfwGetKey( g_window, '`' ) ) {
      if ( !backtic_held ) {
        console_open       = !console_open;
        console_state_time = 0.0;
        if ( console_open ) {
          glfwSetCharCallback( g_window, character_callback );
        } else {
          glfwSetCharCallback( g_window, NULL );
        }
        backtic_held = true;
      }
    } else {
      backtic_held = false;
    }

    // TODO(Anton) GLFW_KEY_TAB for autocomplete

    // test that a variable called 'anton' exists and if so print its value
    float* anton_ptr = apg_c_get_var( "anton" );
    if ( anton_ptr ) { printf( "anton is now %.2f\n", *anton_ptr ); }

  } // end main loop

  gfx_stop();
  free( img_ptr );

  return 0;
}
