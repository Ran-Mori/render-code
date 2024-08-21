cmake_minimum_required(VERSION 3.25)
project(3-hello-triangle C CXX)

set(CMAKE_CXX_STANDARD 14)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -v")
set(CMAKE_VERBOSE_MAKEFILE ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)


# build glad.c.o
add_library(glad ~/Environment/glad/src/glad.c)
target_include_directories(glad PUBLIC ~/Environment/glad/include)

# build 2-hello-triangle.o
add_executable(3-hello-triangle main.cpp)
target_include_directories(3-hello-triangle PUBLIC /opt/homebrew/Cellar/glfw/3.4/include)

# link glad.c.o 2-hello-triangle.o into 3-hello-triangle
target_link_libraries(3-hello-triangle
        glad
        /opt/homebrew/Cellar/glfw/3.4/lib/libglfw.3.dylib
)