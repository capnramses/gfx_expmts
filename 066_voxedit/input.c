// Author: Anton Gerdelan
#include "input.h"
#include "glcontext.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

#define APG_UNUSED( x ) (void)( x ) // to suppress compiler warnings
#define INPUT_N_MBUTTONS 3

// =================================================================================================------------------
// EXTERNAL GLOBALS
// =================================================================================================------------------
double g_mouse_x_win, g_mouse_y_win, g_scroll_delta_x, g_scroll_delta_y, g_mouse_delta_x_win, g_mouse_delta_y_win;
bool g_mouse_inside_window;

// locking
int g_pause_key                                    = GLFW_KEY_SPACE;
int g_toggle_display_all_layers_key                = GLFW_KEY_HOME;
int g_current_layer_up_key                         = GLFW_KEY_PAGE_UP;
int g_current_layer_down_key                       = GLFW_KEY_PAGE_DOWN;
int g_export_mesh_key                              = GLFW_KEY_F2;
int g_reset_key                                    = GLFW_KEY_F9;
int g_toggle_frustum_culling_key                   = GLFW_KEY_1;
int g_toggle_frustum_culling_individual_layers_key = GLFW_KEY_2;
int g_toggle_freeze_frustum_key                    = GLFW_KEY_3;
int g_toggle_post_processing_key                   = GLFW_KEY_4;
int g_toggle_sobel_key                             = GLFW_KEY_5;
int g_toggle_fxaa_key                              = GLFW_KEY_6;
int g_reload_all_shaders_key                       = GLFW_KEY_7;
int g_reload_all_textures_key                      = GLFW_KEY_8;
int g_reload_all_meshes_key                        = GLFW_KEY_9;
int g_toggle_wireframe_key                         = GLFW_KEY_0;
int g_screenshot_key                               = GLFW_KEY_F11;
int g_rotate_prop_ccw_key                          = GLFW_KEY_LEFT_BRACKET;
int g_rotate_prop_cw_key                           = GLFW_KEY_RIGHT_BRACKET;
int g_game_speed_increase_key                      = GLFW_KEY_EQUAL;
int g_game_speed_decrease_key                      = GLFW_KEY_MINUS;

int g_palette_1_key = GLFW_KEY_1;
int g_palette_2_key = GLFW_KEY_2;
int g_palette_3_key = GLFW_KEY_3;
int g_palette_4_key = GLFW_KEY_4;
int g_palette_5_key = GLFW_KEY_5;
int g_palette_6_key = GLFW_KEY_6;
int g_palette_7_key = GLFW_KEY_7;
int g_palette_8_key = GLFW_KEY_8;
int g_palette_9_key = GLFW_KEY_9;

// continuous
int g_forwards_key    = GLFW_KEY_W;
int g_backwards_key   = GLFW_KEY_S;
int g_slide_left_key  = GLFW_KEY_A;
int g_slide_right_key = GLFW_KEY_D;
int g_fly_up_key      = GLFW_KEY_E;
int g_fly_down_key    = GLFW_KEY_Q;
int g_turn_left_key   = GLFW_KEY_LEFT;
int g_turn_right_key  = GLFW_KEY_RIGHT;
int g_turn_up_key     = GLFW_KEY_UP;
int g_turn_down_key   = GLFW_KEY_DOWN;

// =================================================================================================------------------
// INTERNAL GLOBALS
// =================================================================================================------------------
static bool _g_locked_keys[GLFW_KEY_LAST + 1];

static bool _g_mouse_held_state[INPUT_N_MBUTTONS];
static bool _g_mouse_locked_state[INPUT_N_MBUTTONS];

// =================================================================================================------------------
// CALLBACKS
// =================================================================================================------------------
static void _key_cb( GLFWwindow* window, int key, int scancode, int action, int mods ) {
  assert( window );
  APG_UNUSED( scancode );
  APG_UNUSED( mods );

  if ( GLFW_RELEASE == action && key >= 0 && key <= GLFW_KEY_LAST ) {
    if ( _g_locked_keys[key] ) { _g_locked_keys[key] = false; }
  }
}

static void _mouse_button_cb( GLFWwindow* window, int button, int action, int mods ) {
  assert( window );
  APG_UNUSED( mods );

  int idx = button - GLFW_MOUSE_BUTTON_LEFT;
  assert( idx >= 0 && idx < INPUT_N_MBUTTONS );
  _g_mouse_locked_state[idx] = GLFW_RELEASE == action ? false : _g_mouse_locked_state[idx];
  _g_mouse_held_state[idx]   = (bool)action; // GLFW_PRESS 1, GLFW_RELEASE 0
}

static void _cusror_pos_cb( GLFWwindow* window, double x, double y ) {
  assert( window );

  g_mouse_delta_x_win = x - g_mouse_x_win;
  g_mouse_delta_y_win = y - g_mouse_y_win;
  g_mouse_x_win       = x;
  g_mouse_y_win       = y;
}

static void _cursor_enter_cb( GLFWwindow* window, int entered ) {
  assert( window );

  if ( GLFW_TRUE == entered ) {
    g_mouse_inside_window = true;
  } else {
    g_mouse_inside_window = false;
  }
}

static void _scroll_cb( GLFWwindow* window, double x_offset, double y_offset ) {
  assert( window );

  g_scroll_delta_x = x_offset;
  g_scroll_delta_y = y_offset;
}

// =================================================================================================------------------
// MAIN INTERFACE
// =================================================================================================------------------
void init_input() {
  assert( g_window );

  glfwSetKeyCallback( g_window, _key_cb );
  glfwSetMouseButtonCallback( g_window, _mouse_button_cb );
  glfwSetCursorPosCallback( g_window, _cusror_pos_cb );
  glfwSetCursorEnterCallback( g_window, _cursor_enter_cb );
  glfwSetScrollCallback( g_window, _scroll_cb );

  memset( _g_mouse_held_state, 0, sizeof( bool ) * INPUT_N_MBUTTONS );
  memset( _g_mouse_locked_state, 0, sizeof( bool ) * INPUT_N_MBUTTONS );
}

void reset_last_polled_input_states() {
  g_scroll_delta_x = g_scroll_delta_y = false;
  g_mouse_delta_x_win = g_mouse_delta_y_win = 0;
}

// =================================================================================================------------------
// KEYBOARD KEYS
// =================================================================================================------------------
bool is_key_held( int keycode ) {
  assert( g_window && keycode >= 32 && keycode <= GLFW_KEY_LAST );
  // NOTE(Anton) get check lock here to prevent fall-through
  return glfwGetKey( g_window, keycode );
}

bool was_key_pressed( int keycode ) {
  assert( g_window && keycode >= 32 && keycode <= GLFW_KEY_LAST );

  if ( glfwGetKey( g_window, keycode ) ) {
    if ( _g_locked_keys[keycode] ) { return false; } // pressed but still held from last time
    _g_locked_keys[keycode] = true;
    return true;
  }
  _g_locked_keys[keycode] = false;
  return false;
}

// =================================================================================================------------------
// MOUSE BUTTONS
// =================================================================================================------------------

bool lmb_clicked() {
  if ( _g_mouse_held_state[0] && !_g_mouse_locked_state[0] ) {
    _g_mouse_locked_state[0] = true;
    return true;
  }
  return false;
}

bool mmb_clicked() {
  int idx = GLFW_MOUSE_BUTTON_MIDDLE - GLFW_MOUSE_BUTTON_LEFT;
  if ( _g_mouse_held_state[idx] && !_g_mouse_locked_state[idx] ) {
    _g_mouse_locked_state[idx] = true;
    return true;
  }
  return false;
}

bool rmb_clicked() {
  int idx = GLFW_MOUSE_BUTTON_RIGHT - GLFW_MOUSE_BUTTON_LEFT;
  if ( _g_mouse_held_state[idx] && !_g_mouse_locked_state[idx] ) {
    _g_mouse_locked_state[idx] = true;
    return true;
  }
  return false;
}

bool lmb_held() { return _g_mouse_held_state[0]; }

bool mmb_held() {
  int idx = GLFW_MOUSE_BUTTON_MIDDLE - GLFW_MOUSE_BUTTON_LEFT;
  return _g_mouse_held_state[idx];
}

bool rmb_held() {
  int idx = GLFW_MOUSE_BUTTON_RIGHT - GLFW_MOUSE_BUTTON_LEFT;
  return _g_mouse_held_state[idx];
}

void mouse_pos_win( double* x_pos, double* y_pos ) {
  assert( x_pos && y_pos );

  glfwGetCursorPos( g_window, x_pos, y_pos );
}
