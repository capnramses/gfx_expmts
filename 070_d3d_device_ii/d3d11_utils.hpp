#pragma once

#include <stdint.h>

void d3d11_start( uint32_t w, uint32_t h, const wchar_t* window_title, bool debug );

void d3d11_stop();

// returns false if quit was requested by eg clicking on the close window button
bool d3d11_process_events();

// draw a frame
void d3d11_present();
