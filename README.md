# study-opengl
a repo for storing the code when studying opengl.

***

## glfw

### relation with opengl

1. OpenGL itself doesn't handle things like creating windows, handling user input (mouse, keyboard), or managing contexts across different platforms, but glfw does.
2. glfw is a helper library that sits on top of OpenGL. It provides a simpler API for handling the tasks OpenGL doesn't directly address.
3. GLFW provides the foundation and utilities for building your OpenGL application. It takes care of the lower-level details that differ between operating systems, allowing you to focus on the core graphics logic using OpenGL.
4. glfw target does not depend on OpenGL, as GLFW loads any OpenGL, OpenGL ES or Vulkan libraries it needs at runtime.

***

## glad

### what

* Vulkan/GL/GLES/EGL/GLX/WGL Loader-Generator based on the official specifications for multiple languages.

### why

* question

  > The OpenGL API itself is designed to be consistent across different systems. In theory, developers only need to care about the API functions and not the underlying implementation details. So why do I need glad to load functions?

*  While the core OpenGL API defines a set of functions, not all functions are available on all systems. There are two main reasons for this

  1. Newer versions of OpenGL introduce new functions. If you're targeting a wider range of systems with potentially older graphics cards, some functions you might want to use might not be supported everywhere.
  2. Hardware vendors can create optional extensions to the core OpenGL functionality for specific features. These extensions also come with their own set of functions.

### features

1. GLAD helps you by dynamically checking which functions are actually available on the current system at runtime. This ensures your code doesn't try to use functions that aren't supported, preventing errors and crashes.
2. It is a little similar to the androidx library.

### example

```c++
#include <glad/glad.h>

// ... other OpenGL code

// Function to check if a specific OpenGL function is available
bool isFunctionAvailable(const char* functionName) {
  void* functionPointer = glad[functionName];
  return functionPointer != NULL;
}

int main() {
  // ... OpenGL initialization code

  // Check if a specific function (e.g., glDrawElements) is available
  if (isFunctionAvailable("glDrawElements")) {
    // Use the glDrawElements function as usual
    // ... your OpenGL drawing code
  } else {
    // Handle the case where the function is not supported
    std::cout << "Error: glDrawElements function not available." << std::endl;
    // Implement fallback logic or error handling here
  }

  // ... rest of your application code

  return 0;
}
```

### add order

* background

  > It is required that `glad.h` must be included before `glfw.h`.
  >
  > Now I want to call `glClear()` function, it will be different over the including order.

*  `glad.h` is included before `glfw.h`

  ```c++
  //glad.h
  #ifdef __gl_h_
  #error OpenGL header already included, remove this include, glad already provides it
  #endif
  #define __gl_h_ // __gl_h_ is defined by glad.h
  
  GLAPI PFNGLCLEARPROC glad_glClear;
  #define glClear glad_glClear // this is function pointer of glClear
  
  //glad.c
  #include <glad/glad.h>
  static void load_GL_VERSION_1_0(GLADloadproc load) {
    glad_glClear = (PFNGLCLEARPROC)load("glClear"); // glClear is loaded dynamicly
  }
  ```

* only include `glfw.h`

  ```c++
  //glfw.h
  #if !defined(__gl_h_)
  	#if defined(__APPLE__)
    #include <OpenGL/gl.h> // here <OpenGL/gl.h> is included
  	#endif
  #endif
  
  //gl.h
  #ifndef __gl_h_
  #define __gl_h_ // __gl_h_ is defined by glad.h
  
  extern void glClear (GLbitfield mask) OPENGL_DEPRECATED(10.0, 10.14);
  ```

***

## skia

### ref

1. [深入理解Flutter的图形图像绘制原理——图形库skia剖析](https://juejin.cn/post/6914188284126035981)

### what

* It is an open source 2D graphics library which provides common APIs that work across a variety of hardware and software platforms. It serves as the graphics engine for Google Chrome and ChromeOS, Android, Flutter, and many other products.

### features

1. Skia offers multiple backends for rendering, and one of them is OpenGL. This allows Skia to leverage the power of OpenGL for hardware-accelerated rendering on platforms where it's available.

2. Skia also has backends like Vulkan, Metal and its own software renderer.

3. hierarchy

   ```bash
   # from top to down
   mobile applications
   Android app, Flutter app, iOS app
   Skia, CoreAnimation
   Vulkan, OpenGL, Metal
   ARM cpu, Adreno gpu, ARM Mali gpu, iPhone gpu, iPhone ARM cpu

### android

* Skia is an external dependency of AOSP .

***
