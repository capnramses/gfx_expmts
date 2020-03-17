/*
BMP File Reader/Writer Implementation
Anton Gerdelan
Version: 3. 17 March 2020.
Licence: see bottom of file.
C89 ( Implementation is C99 )

Contributors:
- Anton Gerdelan - Initial code.
- Saija Sorsa    - Fuzz testing.

Instructions:
- Just drop this header, and the matching .c file into your project.
- to get debug printouts during parsing define APG_BMP_DEBUG_OUTPUT.

Current Limitations:
- 16-bit images not supported (yet). 1,4,8,24, and 32-bit images can be read.
- No support for interleaved channel bit layouts eg RGB101010 RGB555 RGB565.
- No hooks provided yet to override internal malloc calls.
- No support for compressed images, although in practice these are not used.
- Output images with alpha channel are written in BITMAPINFOHEADER format.
  For better alpha support in other apps the 124-bit v5 header could be used instead,
	at the cost of some backward compatibility and bloat.

To Do:
- Re-instate bmp writer for v3
- FUZZING
  - create a fuzz test set for each BPP (32,24,8,4,1).
	- fuzz each BPP separately.
- (maybe) FEATURE Flipping the image based on negative width and height in header, and/or function arguments. 
- (maybe) PERF ifdef intrinsics/asm for bitscan. Platform-specific code so won't include unless necessary.
- (maybe) FEATURE Add parameter for padding output memory to eg 4-byte alignment or n channels.
- (maybe) FEATURE Improved apps support in alpha channel writing (using v5 header).
*/

#ifndef APG_BMP_H_
#define APG_BMP_H_

#ifdef __cplusplus
extern "C" {
#endif /* CPP */

/*
Reads a bitmap from a file, allocates memory for the raw image data, and returns it.
PARAMS
  * w,h,     - Retrieves the width and height of the BMP in pixels.
  * n_chans  - Retrieves the number of channels in the BMP.
RETURNS
  * Tightly-packed pixel memory in RGBA order. The caller must call free() on the memory.
  * NULL on any error. Any allocated memory is freed before returning NULL.
*/
unsigned char* apg_bmp_read( const char* filename, int* w, int* h, int* n_chans );

#ifdef __cplusplus
}
#endif /* CPP */

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
