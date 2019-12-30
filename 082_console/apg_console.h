/*==============================================================
Quake-style Console mini-library
Status: Work-In-Progress. Not functional yet.
Language: C99
Author:   Anton Gerdelan - @capnramses
Contact:  <antongdl@protonmail.com>
Website:  https://github.com/capnramses/apg - http://antongerdelan.net/
Licence:  See bottom of this file.
==============================================================*/
#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>

#define APG_C_STR_MAX 128
#define APG_C_VARS_MAX 128
#define APG_C_OUTPUT_LINES_MAX 32

// creates a console variable with name `str` and initial value `val`.
// RETURNS false if value with name `str` already exists and does not set the value.
bool apg_c_create_var( const char* str, float val );

// changes the value of an existing console variable.
// RETURNS false if a variable with name `str` does not already exist.
bool apg_c_set_var( const char* str, float val );

// fetches the value of a console variable with name `str`.
// sets the float pointed to by `val` to the value of the variable.
// RETURNS false if the variable does not exist.
bool apg_c_get_var( const char* str, float* val );

// RETURNS number of potential console variables that could complete `substr` found
//   - no matches - returns 0.
//   - exactly 1 match - match is copied into buffer pointed to by `completed`.
//     `completed` requires a buffer of at least APG_C_STR_MAX bytes
//   - more than 1 match - prints all matches to the console.
// NOTE if being called on a string such as `set mypartialvarname` substr should be set to `mypartialvarname`
// to find matching variables.
int apg_c_autocomplete_var( const char* substr, char* completed );

// TODO(Anton) make `apg_c_autocomplete_cmd()` or a combined one that only looks for variables after `set `

// appends str as an output line to the scrolling output
void apg_c_print( const char* str );

// printf everything in console to stdout stream
void apg_c_dump_to_stdout();

// calculate the image pixel dimensions of image required to fit the current console text
// based on the number of lines of printed text in the console and the height of the text in pixels
// RETURNS false on any failure
bool apg_c_get_required_image_dims( int* w, int* h );

// draw the current console text into an image buffer you have allocated.
// you can get the size required to fit the full console text by first calling
// `apg_c_get_required_image_dims()`
// RETURNS false on any failure
bool apg_c_draw_to_image_mem( uint8_t* img_ptr, int w, int h, int n_channels );

int apg_c_count_lines();

#ifdef __cplusplus
}
#endif

/*
-------------------------------------------------------------------------------------
This software is available under two licences - you may use it under either licence.
-------------------------------------------------------------------------------------
FIRST LICENCE OPTION

                                 Apache License
                           Version 2.0, January 2004
                        http://www.apache.org/licenses/
   Copyright 2019 Anton Gerdelan.
   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at
       http://www.apache.org/licenses/LICENSE-2.0
   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
-------------------------------------------------------------------------------------
SECOND LICENCE OPTION

Public Domain (www.unlicense.org)
This is free and unencumbered software released into the public domain.
Anyone is free to copy, modify, publish, use, compile, sell, or distribute this
software, either in source code form or as a compiled binary, for any purpose,
commercial or non-commercial, and by any means.
In jurisdictions that recognize copyright laws, the author or authors of this
software dedicate any and all copyright interest in the software to the public
domain. We make this dedication for the benefit of the public at large and to
the detriment of our heirs and successors. We intend this dedication to be an
overt act of relinquishment in perpetuity of all present and future rights to
this software under copyright law.
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
-------------------------------------------------------------------------------------
*/
