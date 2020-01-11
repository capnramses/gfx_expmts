"""
NOTE(Anton)
Installed Python 3.8 via Windows store (run `python` in cmd and it brings up the store if not installed)
Install pip package manager by downloading the "wheel" for it and running `python get-pip.py` on downloaded file.
pip install glfw
pip install PyOpenGL and PyOpenGL_accelerate
This script is the glfw example from https://pypi.org/project/glfw/ with some extras added
To get access to glClear() etc:
  import OpenGL
  import OpenGL.GL import *
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
