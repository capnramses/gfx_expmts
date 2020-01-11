"""
TODO(Anton)
Idea is to see if I can have a python stub and heavy lifting in existing opengl/C code without modification
* print opengl version running
* create and display triangle with shaders
* load image from file - use my C TGA loader if possible or console lib.
* texture triangle with image

* worth reading - http://pyopengl.sourceforge.net/documentation/opengl_diffs.html
"""

import glfw
import OpenGL
from OpenGL.GL import *

def main():
    # Initialize the library
    if not glfw.init():
        return
    # Create a windowed mode window and its OpenGL context
    window = glfw.create_window(640, 480, "Anton's Python OpenGL Window", None, None)
    if not window:
        glfw.terminate()
        return

    # Make the window's context current
    glfw.make_context_current(window)

    glClearColor( 1.0, 0.2, 0.2, 1.0 )

    # Loop until the user closes the window
    while not glfw.window_should_close(window):
        # Render here, e.g. using pyOpenGL
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT)

        # Swap front and back buffers
        glfw.swap_buffers(window)

        # Poll for and process events
        glfw.poll_events()

    glfw.terminate()

if __name__ == "__main__":
    main()
