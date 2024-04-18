# study-opengl
a repo for storing the code when studying opengl.

## glfw

### relation with opengl

1. OpenGL itself doesn't handle things like creating windows, handling user input (mouse, keyboard), or managing contexts across different platforms, but glfw does.
2. glfw is a helper library that sits on top of OpenGL. It provides a simpler API for handling the tasks OpenGL doesn't directly address.
3. GLFW provides the foundation and utilities for building your OpenGL application. It takes care of the lower-level details that differ between operating systems, allowing you to focus on the core graphics logic using OpenGL.
4. glfw target does not depend on OpenGL, as GLFW loads any OpenGL, OpenGL ES or Vulkan libraries it needs at runtime.

