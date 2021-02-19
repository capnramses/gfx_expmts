/*
Keyboard/mouse
Copyright: Anton Gerdelan <antonofnote@gmail.com>
C99
*/

#pragma once
#include <stdbool.h>

// =================================================================================================------------------
// MAIN INTERFACE
// =================================================================================================------------------
// call once to set up callbacks
void input_init();

// call BEFORE polling events to reset stored states
void input_reset_last_polled_input_states();

// =================================================================================================------------------
// KEYBOARD KEYS
// =================================================================================================------------------
// repeating input
bool input_is_key_held( int keycode );

bool input_is_key_shift_held();

// do not repeat whilst held keys
bool input_was_key_pressed( int keycode );

// remember to call again with 'false' to disable
// RETURNS false if could not start typing callback (error)
bool input_enable_typing_mode( bool enable );

// n_codepoints - must not be NULL
// RETURN ptr to array of codepoints
unsigned int* input_get_typed_codepoints( unsigned int* n_codepoints );

void input_clear_typed_codepoints();

// CTRL+V or APPL+V shortcut to eg clear a command in the internal console. CTRL+V also returns true on Apple.
bool input_paste_shortcut_pressed();

// CTRL+C shortcut to eg clear a command in the internal console
bool input_interrupt_shortcut_pressed();

// =================================================================================================------------------
// MOUSE BUTTONS
// =================================================================================================------------------
bool input_lmb_held();
bool input_lmb_clicked();
bool input_mmb_held();
bool input_mmb_clicked();
bool input_rmb_held();
bool input_rmb_clicked();
bool input_mwheel_scroll_up();
bool input_mwheel_scroll_down();
void input_mouse_pos_win( double* x_pos, double* y_pos );
void input_set_mouse_pos_win( double x_pos, double y_pos );

// =================================================================================================------------------
// EXTERNAL GLOBALS
// =================================================================================================------------------
extern double input_mouse_x_win, input_mouse_y_win, input_scroll_delta_x, input_scroll_delta_y, input_mouse_delta_x_win, input_mouse_delta_y_win;
extern bool input_mouse_inside_window;

// locking
extern int input_escape_key;
extern int input_pause_key;
extern int input_game_speed_increase_key;
extern int input_game_speed_decrease_key;
extern int input_toggle_display_all_layers_key;
extern int input_current_layer_up_key;
extern int input_current_layer_down_key;
extern int input_export_mesh_key;
extern int input_reset_key;
extern int input_toggle_frustum_culling_key;
extern int input_toggle_frustum_culling_individual_layers_key;
extern int input_toggle_freeze_frustum_key;
extern int input_toggle_post_processing_key;
extern int input_toggle_sobel_key;
extern int input_toggle_fxaa_key;
extern int input_reload_all_shaders_key;
extern int input_reload_all_textures_key;
extern int input_reload_all_meshes_key;
extern int input_screenshot_key;
extern int input_console_key;
extern int input_autocomplete_key;
extern int input_prev_hist_command_key;
extern int input_next_hist_command_key;
extern int input_backspace_key;
extern int input_enter_console_command_key;
extern int input_toggle_wireframe_key;
extern int input_save_key;
extern int input_load_key;
extern int input_quicksave_key;


// continuous
extern int input_left_shift_key;
extern int input_forwards_key;
extern int input_backwards_key;
extern int input_slide_left_key;
extern int input_slide_right_key;
extern int input_fly_up_key;
extern int input_fly_down_key;
extern int input_turn_left_key;
extern int input_turn_right_key;
extern int input_turn_up_key;
extern int input_turn_down_key;

extern int input_palette_1_key;
extern int input_palette_2_key;
extern int input_palette_3_key;
extern int input_palette_4_key;
extern int input_palette_5_key;
extern int input_palette_6_key;
extern int input_palette_7_key;
extern int input_palette_8_key;
extern int input_palette_9_key;
extern int input_palette_0_key;
