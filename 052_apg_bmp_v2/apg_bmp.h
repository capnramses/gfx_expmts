/*
BMP File Reader/Writer Interface
Licence: see bottom of file.
C89 ( impl is C99 )

Contributors:
- Anton Gerdelan - initial code
- Saija Sorsa - fuzz testing

To Do:
- (maybe) PERF different loops for different bpp options to remove inner-loop branching
- (maybe) PERF ifdef intrinsics/asm for bitscan
- (maybe) FEATURE add parameter for padding to eg 4-byte alignment or n channels
- (maybe) FEATURE improved apps support in alpha channel writing (using v5 header)

Current Limitations:
- 16-bit images not supported (yet). 1,4,8,24, and 32-bit images can be read.
- no support for interleaved channel bit layouts eg RGB101010 RGB555 RGB565
- no hooks provided yet to override internal malloc calls
- no support for compressed images
- output images with alpha channel are written in BITMAPINFOHEADER format.
  for better alpha support in other apps the 124-bit v5 header could be used instead,
	at the cost of some backward compatibility and bloat.
*/

#ifndef _APG_BMP_H_
#define _APG_BMP_H_

/*
Reads a Microsoft BMP from the given filename, and allocates and fills a buffer of pixels.
Width w, height h, and number of colour channels n_chans will all be set by the function based on the file's properties.
If vertical_flip is 0 (false) then memory order will be from the bottom row to the top row (upwards).
If set to 1 (true) the image will be flipped so that memory starts at the top-left pixel.
RETURNS:
An RGBA-ordered tightly-packed buffer, or NULL on error. 
It is the calling program's responsibility to call free() on the returned memory.
CAVEATS:
In order to keep the code free of system-specific calls the function does not check if filename is a valid file
- the calling application code must not pass eg a directory name here.
*/
unsigned char* apg_read_bmp( const char* filename, int* w, int* h, int* n_chans, int vertical_flip );

/*
Write a Microsoft BMP to the given filename, from tightly packed pixel_data with the given width w and height h in pixels.
The number of channels may be 3 or 4 for RGB or RGBA, respectively.
If given 1 or 2 channels then a 3-channel RGB is currently written, for simplicity.
RETURNS:
0 (false) on any failure, or 1 (true) on success.
*/
int apg_write_bmp( const char* filename, const unsigned char* pixel_data, int w, int h, int n_chans );

#endif /*_APG_BMP_H_ */

/*
-------------------------------------------------------------------------------------
This software is available under two licences - you may use it under either licence.
-------------------------------------------------------------------------------------
FIRST LICENCE OPTION

>                                  Apache License
>                            Version 2.0, January 2004
>                         http://www.apache.org/licenses/
>    Copyright 2019 Anton Gerdelan.
>    Licensed under the Apache License, Version 2.0 (the "License");
>    you may not use this file except in compliance with the License.
>    You may obtain a copy of the License at
>        http://www.apache.org/licenses/LICENSE-2.0
>    Unless required by applicable law or agreed to in writing, software
>    distributed under the License is distributed on an "AS IS" BASIS,
>    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
>    See the License for the specific language governing permissions and
>    limitations under the License.
-------------------------------------------------------------------------------------
SECOND LICENCE OPTION

> This is free and unencumbered software released into the public domain.
>
> Anyone is free to copy, modify, publish, use, compile, sell, or
> distribute this software, either in source code form or as a compiled
> binary, for any purpose, commercial or non-commercial, and by any
> means.
>
> In jurisdictions that recognize copyright laws, the author or authors
> of this software dedicate any and all copyright interest in the
> software to the public domain. We make this dedication for the benefit
> of the public at large and to the detriment of our heirs and
> successors. We intend this dedication to be an overt act of
> relinquishment in perpetuity of all present and future rights to this
> software under copyright law.
>
> THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
> EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
> MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
> IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR
> OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
> ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
> OTHER DEALINGS IN THE SOFTWARE.
>
> For more information, please refer to <http://unlicense.org>
-------------------------------------------------------------------------------------
*/
