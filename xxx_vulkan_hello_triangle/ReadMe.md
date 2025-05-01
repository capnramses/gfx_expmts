## Primary Goals of a Vulkan Hello Triangle

* Should be a starting springboard for my hobby projects.
* C, not C++.
* Minimalist, and simple as possible without being useless.
* No silly code app abstractions, templates etc.
  It should be easy to follow and adapt into any code-base.
* No third-party framework cludge dependencies if possible.
* Self-contained. No "../common/framework/..." type thing for the starter.
* Just 1 or 2 files, if possible.

## Secondary Goals

* Should be able to sit next to a similar D3D12 hello triangle.
* Make a vulkan drop-in version of my OpenGL gfx.h used in my other projects.
* Should be available to turn into the basis of
  a no-nonsense anton-brand kind of tutorial to throw on my website.

## References of Note

* https://www.glfw.org/docs/3.3/vulkan_guide.html -- functions that simplify other tutorials.
* Sergey's book that I edited had the best Vulkan starter demo I've seen so far.
* This tutorial looks pretty good https://vulkan-tutorial.com/Drawing_a_triangle/Setup/Base_code

## ToDo

* try using integrated device on new PC
* try no-graphics vulkan, with a view to ML or generic GPGPU workloads in compute shaders.
  * also why doesn't pytorch support that instead of 'cuda' rocm? it would work on more devices?