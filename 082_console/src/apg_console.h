/*==============================================================
Quake-style Console mini-library
Status: Work-In-Progress. Not functional yet.
Language: C99
Author:   Anton Gerdelan - @capnramses
Contact:  <antongdl@protonmail.com>
Website:  https://github.com/capnramses/apg - http://antongerdelan.net/
Licence:  See bottom of this file.

Instructions
============
The primary interface for user-entered text is:

  apg_c_append_user_entered_text( str );

Where `str` can be a whole line or eg one character at a time.
Instructions end and are entered and parsed after a line break `\n` byte.

Autocompletion of user-entered text can be used with:

  apg_c_autocomplete();

Instructions may be of the following forms:

  BUILT-IN-COMMAND [VARIABLE] [VALUE]
  VARIABLE

Built-in commands are:

  "var my_var 2.0" - create a new variable called my_var and initialise its value to 2.0
  "my_var 2.0"     - set the value of a variable 'my_var'
  "my_var"         - print the value of variable 'my_var'
  "clear"          - invoke the 'clear' command

All values are stored as 32-bit floats, but may be cast as boolean or integer values.

Variables may also be created, set, or fetched programmatically.

  apg_c_create_var( str )
  apg_c_set_var( str, val )
  apg_c_get_var( str )

Scrolling output text may be interacted with

  apg_capg_c_output_clear_clear() - Clear the output text.
  apg_c_print( str )              - Adds a line of text to the output such as a debug message.
  apg_c_dump_to_stdout()          - Writes the current console output text to stdout via printf().
  apg_c_count_lines()             - Counts the number of lines in the console output.
  
The console text may also be rendered out to an image for use in graphical applications.
This is API-agnostic so must be converted to a texture to be used with 3D APIs.

  apg_c_draw_to_image_mem()       - Writes current console text on top of a pre-allocated image.

TODO
=====
* Display the user entered text as an additional line, starting with '>' eg "> my text here"
* Remove the big string maker and print line-by-line.
* Store a colour for each line so eg errors can be in red.
==============================================================*/
#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>

#define APG_C_STR_MAX 128         // maximum console string length. commands and variable names must be shorter than this.
#define APG_C_VARS_MAX 128        // maximum number of variables stored in console
#define APG_C_OUTPUT_LINES_MAX 32 // maximum number of lines retained in output

bool apg_c_append_user_entered_text( const char* str );
void apg_c_backspace( void );
void apg_c_clear_user_entered_text( void );

// creates a console variable with name `str` and initial value `val`.
// RETURNS
//   NULL if value with name `str` already exists. The function then does not change the value.
//   Address of the c_var. Since c_vars cannot be deleted this can be used anywhere in your code to get or set the c_var's value.
float* apg_c_create_var( const char* str, float val );

// fetches the value of a console variable with name `str`.
// sets the float pointed to by `val` to the value of the variable.
// RETURNS
//   NULL if the variable does not exist.
//   Address of the c_var. Since c_vars cannot be deleted this can be used anywhere in your code to get or set the c_var's value.
float* apg_c_get_var( const char* str, float* val );

// changes the value of an existing console variable.
// RETURNS
//  false if a variable with name `str` does not already exist.
bool apg_c_set_var( const char* str, float val );

// RETURNS
//   number of potential console variables that could complete `substr` found
//   - no matches - returns 0.
//   - exactly 1 match - match is copied into buffer pointed to by `completed`.
//     `completed` requires a buffer of at least APG_C_STR_MAX bytes
//   - more than 1 match - prints all matches to the console.
// NOTE if being called on a string such as `set mypartialvarname` substr should be set to `mypartialvarname`
// to find matching variables.
int apg_c_autocomplete_var( const char* substr, char* completed );

// TODO(Anton) make `apg_c_autocomplete_cmd()` or a combined one that only looks for variables after `set `

void apg_c_output_clear( void );

// Appends str as an output line to the scrolling output
void apg_c_print( const char* str );

int apg_c_count_lines( void );

// printf everything in console to stdout stream
void apg_c_dump_to_stdout( void );

// Draw the current console text into an image buffer you have allocated with dimensions w, h, and n_channels.
// The destination image does not need to exactly match the console text size - it can be bigger or smaller.
// Text starts drawing at the bottom-left of the provided image, and lines scroll upwards.
// PARAMETERS
//   img_ptr    - pointer to the destination image bytes. must not be NULL
//   w,h        - dimensions of the destination image in pixels
//   n_channels - number of channels in the destination image
// RETURNS
//   false on any failure
bool apg_c_draw_to_image_mem( uint8_t* img_ptr, int w, int h, int n_channels );

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
