/*
Keyboard/mouse
Copyright: Anton Gerdelan <antonofnote@gmail.com>
C99
*/

#include "input.h"
#include "glcontext.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

#define APG_UNUSED( x ) (void)( x ) /* to suppress compiler warnings */
#define INPUT_N_MBUTTONS 3
#define INPUT_MAX_CODEPOINTS 256

// =================================================================================================
// EXTERNAL GLOBALS
// =================================================================================================
double input_mouse_x_win, input_mouse_y_win, input_scroll_delta_x, input_scroll_delta_y, input_mouse_delta_x_win, input_mouse_delta_y_win;
bool input_mouse_inside_window;

// locking
int input_escape_key                                   = GLFW_KEY_ESCAPE;
int input_pause_key                                    = GLFW_KEY_SPACE;
int input_toggle_display_all_layers_key                = GLFW_KEY_HOME;
int input_current_layer_up_key                         = GLFW_KEY_PAGE_UP;
int input_current_layer_down_key                       = GLFW_KEY_PAGE_DOWN;
int input_export_mesh_key                              = GLFW_KEY_F2;
int input_reset_key                                    = GLFW_KEY_F9;
int input_toggle_frustum_culling_key                   = GLFW_KEY_1;
int input_toggle_frustum_culling_individual_layers_key = GLFW_KEY_2;
int input_toggle_freeze_frustum_key                    = GLFW_KEY_3;
int input_toggle_post_processing_key                   = GLFW_KEY_4;
int input_toggle_sobel_key                             = GLFW_KEY_5;
int input_toggle_fxaa_key                              = GLFW_KEY_6;
int input_reload_all_shaders_key                       = GLFW_KEY_7;
int input_reload_all_textures_key                      = GLFW_KEY_8;
int input_reload_all_meshes_key                        = GLFW_KEY_9;
int input_toggle_wireframe_key                         = GLFW_KEY_0;
int input_screenshot_key                               = GLFW_KEY_F11;
int input_console_key                                  = '`';
int input_prev_hist_command_key                        = GLFW_KEY_UP;
int input_next_hist_command_key                        = GLFW_KEY_DOWN;
int input_autocomplete_key                             = GLFW_KEY_TAB;
int input_backspace_key                                = GLFW_KEY_BACKSPACE;
int input_enter_console_command_key                    = GLFW_KEY_ENTER;
int input_save_key                                     = GLFW_KEY_F2;
int input_load_key                                     = GLFW_KEY_F3;
int input_game_speed_increase_key                      = GLFW_KEY_EQUAL;
int input_game_speed_decrease_key                      = GLFW_KEY_MINUS;

int input_palette_0_key = GLFW_KEY_0;
int input_palette_1_key = GLFW_KEY_1;
int input_palette_2_key = GLFW_KEY_2;
int input_palette_3_key = GLFW_KEY_3;
int input_palette_4_key = GLFW_KEY_4;
int input_palette_5_key = GLFW_KEY_5;
int input_palette_6_key = GLFW_KEY_6;
int input_palette_7_key = GLFW_KEY_7;
int input_palette_8_key = GLFW_KEY_8;
int input_palette_9_key = GLFW_KEY_9;

// continuous
int input_left_shift_key  = GLFW_KEY_LEFT_SHIFT;
int input_forwards_key    = GLFW_KEY_W;
int input_backwards_key   = GLFW_KEY_S;
int input_slide_left_key  = GLFW_KEY_A;
int input_slide_right_key = GLFW_KEY_D;
int input_fly_up_key      = GLFW_KEY_E;
int input_fly_down_key    = GLFW_KEY_Q;
int input_turn_left_key   = GLFW_KEY_LEFT;
int input_turn_right_key  = GLFW_KEY_RIGHT;
int input_turn_up_key     = GLFW_KEY_UP;
int input_turn_down_key   = GLFW_KEY_DOWN;

// =================================================================================================
// INTERNAL GLOBALS
// =================================================================================================
static bool _locked_keys[GLFW_KEY_LAST + 1];

static bool _mouse_held_state[INPUT_N_MBUTTONS];
static bool _mouse_locked_state[INPUT_N_MBUTTONS];
static unsigned int _user_typing_accum_array[INPUT_MAX_CODEPOINTS];
static unsigned int _n_user_typing_accum_array;
static int _input_modifier_keys_held_bitfield;

// =================================================================================================
// CALLBACKS
// =================================================================================================
static void _key_cb( GLFWwindow* window, int key, int scancode, int action, int mods ) {
  assert( window );
  APG_UNUSED( scancode );

  _input_modifier_keys_held_bitfield = mods;

  if ( GLFW_RELEASE == action && key >= 0 && key <= GLFW_KEY_LAST ) {
    if ( _locked_keys[key] ) { _locked_keys[key] = false; }
  }
}

static void _mouse_button_cb( GLFWwindow* window, int button, int action, int mods ) {
  assert( window );
  APG_UNUSED( mods );

  int idx = button - GLFW_MOUSE_BUTTON_LEFT;
  if ( idx < 0 || idx >= INPUT_N_MBUTTONS ) { return; }
  _mouse_locked_state[idx] = GLFW_RELEASE == action ? false : _mouse_locked_state[idx];
  _mouse_held_state[idx]   = (bool)action; // GLFW_PRESS 1, GLFW_RELEASE 0
}

static void _cusror_pos_cb( GLFWwindow* window, double x, double y ) {
  assert( window );

  input_mouse_delta_x_win = x - input_mouse_x_win;
  input_mouse_delta_y_win = y - input_mouse_y_win;
  input_mouse_x_win       = x;
  input_mouse_y_win       = y;
}

static void _cursor_enter_cb( GLFWwindow* window, int entered ) {
  assert( window );

  if ( GLFW_TRUE == entered ) {
    input_mouse_inside_window = true;
  } else {
    input_mouse_inside_window = false;
  }
}

static void _scroll_cb( GLFWwindow* window, double x_offset, double y_offset ) {
  assert( window );

  input_scroll_delta_x = x_offset;
  input_scroll_delta_y = y_offset;
}

// =================================================================================================
// MAIN INTERFACE
// =================================================================================================
void input_init() {
  assert( gfx_window_ptr );

  glfwSetKeyCallback( gfx_window_ptr, _key_cb );
  glfwSetMouseButtonCallback( gfx_window_ptr, _mouse_button_cb );
  glfwSetCursorPosCallback( gfx_window_ptr, _cusror_pos_cb );
  glfwSetCursorEnterCallback( gfx_window_ptr, _cursor_enter_cb );
  glfwSetScrollCallback( gfx_window_ptr, _scroll_cb );

  memset( _mouse_held_state, 0, sizeof( bool ) * INPUT_N_MBUTTONS );
  memset( _mouse_locked_state, 0, sizeof( bool ) * INPUT_N_MBUTTONS );
}

void input_reset_last_polled_input_states() {
  input_scroll_delta_x = input_scroll_delta_y = false;
  input_mouse_delta_x_win = input_mouse_delta_y_win = 0;
}

// =================================================================================================
// KEYBOARD KEYS
// =================================================================================================
bool input_is_key_held( int keycode ) {
  assert( gfx_window_ptr && keycode >= 32 && keycode <= GLFW_KEY_LAST );
  // NOTE(Anton) get check lock here to prevent fall-through
  return glfwGetKey( gfx_window_ptr, keycode );
}

bool input_was_key_pressed( int keycode ) {
  assert( gfx_window_ptr && keycode >= 32 && keycode <= GLFW_KEY_LAST );

  if ( glfwGetKey( gfx_window_ptr, keycode ) ) {
    if ( _locked_keys[keycode] ) { return false; } // pressed but still held from last time
    _locked_keys[keycode] = true;
    return true;
  }
  _locked_keys[keycode] = false;
  return false;
}

static void _character_callback( GLFWwindow* window, unsigned int codepoint ) {
  APG_UNUSED( window );
  if ( _n_user_typing_accum_array < INPUT_MAX_CODEPOINTS ) { _user_typing_accum_array[_n_user_typing_accum_array++] = codepoint; }
}

bool input_enable_typing_mode( bool enable ) {
  assert( gfx_window_ptr );

  if ( !enable ) {
    glfwSetCharCallback( gfx_window_ptr, NULL );
    return true;
  }

  _n_user_typing_accum_array = 0; // clear the previous buffer
  bool ret                   = glfwSetCharCallback( gfx_window_ptr, _character_callback );
  return ret;
}

unsigned int* input_get_typed_codepoints( unsigned int* n_codepoints ) {
  assert( n_codepoints );

  *n_codepoints = _n_user_typing_accum_array;
  return _user_typing_accum_array;
}

void input_clear_typed_codepoints() {
  _n_user_typing_accum_array = 0; // clear the previous buffer
}

bool input_paste_shortcut_pressed() {
#ifdef __APPLE__ // on OSX this should be SUPER i guess
  if ( ( _input_modifier_keys_held_bitfield & GLFW_MOD_SUPER ) && ( input_was_key_pressed( 'V' ) ) ) { return true; }
#endif
  if ( ( _input_modifier_keys_held_bitfield & GLFW_MOD_CONTROL ) && ( input_was_key_pressed( 'V' ) ) ) { return true; }

  return false;
}

bool input_interrupt_shortcut_pressed() {
  if ( ( _input_modifier_keys_held_bitfield & GLFW_MOD_CONTROL ) && ( input_was_key_pressed( 'C' ) ) ) { return true; }

  return false;
}

// =================================================================================================
// MOUSE BUTTONS
// =================================================================================================

bool input_lmb_clicked() {
  if ( _mouse_held_state[0] && !_mouse_locked_state[0] ) {
    _mouse_locked_state[0] = true;
    return true;
  }
  return false;
}

bool input_mmb_clicked() {
  int idx = GLFW_MOUSE_BUTTON_MIDDLE - GLFW_MOUSE_BUTTON_LEFT;
  if ( _mouse_held_state[idx] && !_mouse_locked_state[idx] ) {
    _mouse_locked_state[idx] = true;
    return true;
  }
  return false;
}

bool input_rmb_clicked() {
  int idx = GLFW_MOUSE_BUTTON_RIGHT - GLFW_MOUSE_BUTTON_LEFT;
  if ( _mouse_held_state[idx] && !_mouse_locked_state[idx] ) {
    _mouse_locked_state[idx] = true;
    return true;
  }
  return false;
}

bool input_lmb_held() { return _mouse_held_state[0]; }

bool input_mmb_held() {
  int idx = GLFW_MOUSE_BUTTON_MIDDLE - GLFW_MOUSE_BUTTON_LEFT;
  return _mouse_held_state[idx];
}

bool input_rmb_held() {
  int idx = GLFW_MOUSE_BUTTON_RIGHT - GLFW_MOUSE_BUTTON_LEFT;
  return _mouse_held_state[idx];
}

bool input_mwheel_scroll_up() { return input_scroll_delta_y > 0; }

bool input_mwheel_scroll_down() { return input_scroll_delta_y < 0; }

void input_mouse_pos_win( double* x_pos, double* y_pos ) {
  assert( x_pos && y_pos );

  glfwGetCursorPos( gfx_window_ptr, x_pos, y_pos );
}

void input_set_mouse_pos_win( double x_pos, double y_pos ) { glfwSetCursorPos( gfx_window_ptr, x_pos, y_pos ); }
