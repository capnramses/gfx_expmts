// Author: Anton Gerdelan
#pragma once
#include <stdbool.h>

// =================================================================================================------------------
// MAIN INTERFACE
// =================================================================================================------------------
// call once to set up callbacks
void init_input();

// call BEFORE polling events to reset stored states
void reset_last_polled_input_states();

// =================================================================================================------------------
// KEYBOARD KEYS
// =================================================================================================------------------
// repeating input
bool is_key_held( int keycode );

// do not repeat whilst held keys
bool was_key_pressed( int keycode );

// =================================================================================================------------------
// MOUSE BUTTONS
// =================================================================================================------------------
bool lmb_held();
bool lmb_clicked();
bool mmb_held();
bool mmb_clicked();
bool rmb_held();
bool rmb_clicked();
void mouse_pos_win( double* x_pos, double* y_pos );

// =================================================================================================------------------
// EXTERNAL GLOBALS
// =================================================================================================------------------
extern double g_mouse_x_win, g_mouse_y_win, g_scroll_delta_x, g_scroll_delta_y, g_mouse_delta_x_win, g_mouse_delta_y_win;
extern bool g_mouse_inside_window;

// locking
extern int g_pause_key;
extern int g_game_speed_increase_key;
extern int g_game_speed_decrease_key;
extern int g_toggle_display_all_layers_key;
extern int g_current_layer_up_key;
extern int g_current_layer_down_key;
extern int g_export_mesh_key;
extern int g_reset_key;
extern int g_toggle_frustum_culling_key;
extern int g_toggle_frustum_culling_individual_layers_key;
extern int g_toggle_freeze_frustum_key;
extern int g_toggle_post_processing_key;
extern int g_toggle_sobel_key;
extern int g_toggle_fxaa_key;
extern int g_reload_all_shaders_key;
extern int g_reload_all_textures_key;
extern int g_reload_all_meshes_key;
extern int g_screenshot_key;
extern int g_toggle_wireframe_key;
extern int g_rotate_prop_ccw_key;
extern int g_rotate_prop_cw_key;

// continuous
extern int g_forwards_key;
extern int g_backwards_key;
extern int g_slide_left_key;
extern int g_slide_right_key;
extern int g_fly_up_key;
extern int g_fly_down_key;
extern int g_turn_left_key;
extern int g_turn_right_key;
extern int g_turn_up_key;
extern int g_turn_down_key;

extern int g_palette_1_key;
extern int g_palette_2_key;
extern int g_palette_3_key;
extern int g_palette_4_key;
extern int g_palette_5_key;
extern int g_palette_6_key;
extern int g_palette_7_key;
extern int g_palette_8_key;
extern int g_palette_9_key;
