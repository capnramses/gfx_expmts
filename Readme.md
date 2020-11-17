# Graphics Experiments / Demos

A repository to put play-around ideas (good, bad, and crazy), semi-finished
projects, and volatile software. Mostly OpenGL but some other graphics-related stuff too.
If a project turns into a reusable library it most likely ends up in the [apg](https://github.com/capnramses/apg) repository of mini libraries.

## Contents

| numerus | titulus                     | descriptio                                                                 | status              |
|---------|-----------------------------|----------------------------------------------------------------------------|---------------------|
| 000     | `video_modus`               | find and list all video modes on troublesome hardware                      | working             |
| 001     | `cube_map_depth`            | depth writing to cube maps for omni-direc. shadows                         | abandoned           |
| 002     | `bezier_curve`              | demo from splines and curves lecture last year                             | working             |
| 003     | `cube`                      | spinning cube from .obj as starting point project                          | working             |
| 004     | `outlines`                  | outline rendering prototype                                                | working             |
| 005     | `outlines_post`             | post-processing outline rendering prototype                                | working             |
| 006     | `raytrace_cs`               | path tracer based on compute shaders                                       | working             |
| 007     | `tiny_font`                 | minimal-resolution pixel font rendering                                    | working             |
| 008     | `viewports`                 | making sure I could do one of the assignments                              | working             |
| 009     | `water_shader`              | prototype wave-based vertex animation                                      | working             |
| 010     | `compressed_textures`       | demo of various texture compression options                                | working             |
| 011     | `stencils`                  | playing around with stencil tests                                          | proposed            |
| 012     | `gl3w`                      | alternative to GLEW                                                        | working             |
| 013     | `sdl2`                      | SDL2 opengl start-up                                                       | osx                 |
| 014     | `mirror_plane_fb`           | simple mirror w/ previous frame's rendering flipped                        | working             |
| 015     | `hdr_bloom`                 | high-dynamic range rendering with bloom shader                             | proposed            |
| 016     | `pbr`                       | physically-based rendering                                                 | proposed            |
| 017     | `x11_render_loop`           | basic rendering demo with just X11 (not opengl)                            | working             |
| 018     | `default_texture`           | hard-coded fallback texture if img not found                               | working             |
| 019     | `generic_shader`            | abstraction that reverts to default if shader fails                        | working             |
| 020     | `image_chan_swap`           | tool to swap channels in Unreal exported normal maps                       | working             |
| 021     | `omni_shad_sp`              | single-pass omni-directional shadow mapping                                | abandoned           |
| 022     | `glfw_getkeyname`           | testing new key name localisation for glfw                                 | working             |
| 023     | `webgl_quats`               | webgl demo of quaternion rotation mathematics                              | working             |
| 024     | `hmap_terrain`              | the traditional heightmapped terrain demo                                  | working             |
| 025     | `depth_antioverdraw`        | http://fabiensanglard.net/doom3/renderer.php                               | working             |
| 026     | `x11_cube`                  | software 3d renderer built on X11 (not opengl)                             | working             |
| 027     | `omni_shads_cheating`       | omni-directional shadows with cubemap texture                              | unstable            |
| 028     | `more_cube`                 | second pass at shadow mapping with cubemap textures                        | working             |
| 029     | `more_cube_gl_2_1`          | opengl 2.1 port of omni-directional shadows                                | working             |
| 030     | `clang_vectors`             | using clang vector extension data types                                    | started             |
| 031     | `gcc_vectors`               | using gcc vector extension data types                                      | started             |
| 032     | `vulkan_hw`                 | vulkan skeleton                                                            | started             |
| 033     | `compute_shader`            | compute shader play-around                                                 | working             |
| 034     | `switching_costs`           | measuring opengl state switching costs                                     | working             |
| 035     | `vrdemo`                    | virtual reality framework for Cardboard                                    | started             |
| 036     | `pano2cube`                 | converts Streetview app panorama to cube map textures                      | working             |
| 037     | `bsp`                       | binary space partitioning demo                                             | started             |
| 038     | `SPEW`                      | home-made GLEW replacement. not an acronym, just loud                      | working             |
| 039     | `wad_render`                | render geometry from a DOOM WAD file in OpenGL                             | working             |
| 040     | `compute_shader_neural_net` | a neural network encoded in a compute shader                               | started             |
| 041     | `node_terrain `             | terrain that subdivides and can do LOD                                     | working             |
| 042     | `dissolve`                  | a simple dissolving mesh effect in webgl                                   | working             |
| 043     | `stb_ttf`                   | font atlas generator using [stb_truetype](https://github.com/nothings/stb) | working             |
| 044     | `webgl_skybox`              | WebGL skybox to render one of my `pano2cube` boxes                         | working             |
| 045     | `voxel`                     | Extremely basic voxel rendering of a grid of cubes                         | working             |
| 046     | `async_texture`             | Using a thread pool to load images into textures                           | working             |
| 047     | `bresenhams_line`           | Bitmap-drawing impl. of Bresenham's line algorithm                         | working             |
| 048     | `barycentric_triangle`      | Bitmap-drawing impl. of a triangle fill algorithm                          | working             |
| 049     | `apg_chart`                 | Bitmap-drawing scientific chart plotting prototype.                        | working             |
| 050     | `msaa_fbo`                  | Applying multi-sample anti-aliasing to framebuffer objects.                | working             |
| 051     | `apg_bmp`                   | Functions to read and write BMP files.                                     | moved to `apg` repo |
| 052     | `apg_bmp_v2`                | A more efficient apg_bmp with larger-buffer reads/writes.                  | moved to `apg` repo |
| 053     | `glTF`                      | Load and display glTF file using cgltf library.                            | started             |
| 054     | `msdos_hello`               | Hello World C program for MS-DOS.                                          | working             |
| 055     | `diamond_square`            | Heightmap generation with Diamond-Square algorithm.                        | working             |
| 056     | `2d_to_cubemap`             | Environment map generation from a single image.                            | working             |
| 057     | `sphere_doubler`            | Program to modify the texture coords of a sphere.                          | working             |
| 058     | `tga_reader`                | Read and render a basic TGA file.                                          | working             |
| 059     | `subpixel_pixelfont`        | Sampling pixel fonts to reduce shimmery artifacts.                         | working             |
| 060     | `upscale_pixelfont`         | Upscaling pixel font to compare to previous demo.                          | working             |
| 061     | `pdf_view`                  | PDF parser and rendering an image from PDF in OpenGL.                      | working             |
| 062     | `revoxels`                  | Another voxel generator and renderer.                                      | working             |
| 063     | `vox_pick`                  | Ray-based mouse picking for voxels.                                        | working             |
| 064     | `offcentre_perspective`     | Perspective correct subwindow rendering                                    | working             |
| 065     | `vox_colour_pick`           | Using colour buffer 1 frame later with 3x3 window                          | working             |
| 066     | `voxedit`                   | A quick-to-use voxel mesh creator and exporter                             | working             |
| 067     | `multichunk`                | Voxel world made from several chunks                                       | working             |
| 068     | `winmain`                   | Win32 program with a WinMain (prelude to D3D code)                         | working             |
| 069     | `d3d_device`                | Start a Direct3D 11 Device and Context                                     | removed / teaching  |
| 070     | `d3d_device_ii`             | Fresh start based on MS docs. Also clear the viewport.                     | removed / teaching  |
| 071     | `d3d_hello_triangle`        | Basic demo to display a triangle with D3D11.                               | removed / teaching  |
| 072     | `raytracer_sw`              | Basic demo of coding a raytracer in software.                              | removed / teaching  |
| 073     | `ppm_write`                 | Making sure I can write a P6 PPM off the top of my head.                   | working             |
| 074     | `slabs`                     | ray-OOBB and ray-barycentric intersection tests.                           | removed / teaching  |
| 075     | `triangle_fill`             | Edge Function triangle fill algorithm impl.                                | working             |
| 076     | `sw_rasteriser`             | Software rasteriser for drawing a 3D mesh.                                 | working             |
| 077     | `apg_ply`                   | Rewrite of my PLY mesh reader/writer.                                      | working             |
| 078     | `sw_diffuse`                | Diffuse lighting for software rasteriser.                                  | working             |
| 079     | `win32`                     | Open a Win32 window.                                                       | working             |
| 080     | `d3d_init`                  | Initialise Direct3D 11.                                                    | working             |
| 081     | `gfx_platform`              | "Hello Cube" project starter using a graphics platform wrapper I wrote.    | working             |
| 082     | `console`                   | Quake-style interactive console mini-lib.                                  | working             |
| 083     | `diamond_square`            | Diamond-Square Alg for vox-edit.                                           | working             |
| 084     | `pygl`                      | Python script that launches a GLFW window with OpenGL context.             | working             |
| 085     | `py_c_lib`                  | See if I can use my C tga loader from Python app.                          | proposed            |
| 086     | `vox_array_texture`         | Using OpenGL array texture instead of texture atlas for voxel terrain.     | working             |
| 087     | `vox_paging`                | Paging chunks of voxel terrain to/from disk.                               | started             |
| 088     | `apg_bmp_v3`                | Rewrite and refuzz of BMP reader.                                          | moved to `apg` repo |
| 089     | `voxedit_edges`             | Single-pass outline rendering based on `066_voxedit`.                      | working             |
| 090     | `flourish_scroll`           | Study of a cool menu title animation I saw on Empire of Sin.               | working             |
| 091     | `particle_meshes`           | Particle system demo using instanced meshes for particles.                 | working             |
| 092     | `aframe_hotspots`           | A-Frame WebXR/WebVR demo with information points. Using Sponza.            | started             |
| 093     | `x_converter`               | Converted for old MS .x format mesh files.                                 | proposed            |
| 094     | `video_play`                | MPEG-1 video playback demo using plmpeg.h                                  | working             |
| 095     | `gltf_minimal`              | Example of loading a mesh from glTF files and displaying it.               | working             |
| 096     | `gltf_materials`            | Example of loading a mesh with materials from glTF files.                  | started             |
| 097     | `pbr_direct`                | PBR example using only a direct light source.                  | started             |
| xxx     | `draco`                     | Example of mesh compression using Google Draco.                            | proposed            |
| xxx     | `fire`                      | Shader effect using multi-texturing for fire animation.                    | proposed            |
| xxx     | `dither`                    | Dithering shader effect.                                                   | proposed            |
| xxx     | `sw_texture`                | Basic Texture Mapping for software rasteriser.                             | proposed            |
| xxx     | `msdos_vga`                 | VGA graphics output for MS-DOS.                                            | proposed            |
| xxx     | `geo_mipmap`                | heightmap terrain sampled in vertex shader, scrolling camera mesh          | proposed            |
| xxx     | `fresnel_prism`             | refraction/reflection colour split as in nvidia cg_tutorial_chapter07      | proposed            |
| xxx     | `wu_line`                   | wu's line drawing algorithm (pseudo on wiki)                               | proposed            |
| xxx     | `3ds parse`                 | a 3ds binary format loader                                                 | proposed            |
| xxx     | `widgets`                   | simple slider and text field widgets drop-in for demos                     | proposed            |
| xxx     | `stencil buffer`            | render outlines like in Lindsay Kay's CAD renderer                         | proposed            |

## LICENCE

Some files in this repository may be from third-party libraries.
It is the user's responsibility to determine and respect the licences of these files.
All other files are are dual-licenced and may be used by you under the terms of your
choice of licence:

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

## Notes

* find more test 3D models at [Stanford Computer Graphics Laboratory](http://graphics.stanford.edu/data/3Dscanrep/)

## Cool Thing to Try

* http://www.codersnotes.com/notes/untonemapping/
