cmake_minimum_required(VERSION 3.25)
project(3_hello_triangle C CXX)

set(CMAKE_CXX_STANDARD 14)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -v")
set(CMAKE_VERBOSE_MAKEFILE ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include_directories(/opt/homebrew/Cellar/glfw/3.4/include)
include_directories(~/Environment/glad/include)

link_libraries(/opt/homebrew/Cellar/glfw/3.4/lib/libglfw.3.dylib)

add_library(glad STATIC ~/Environment/glad/src/glad.c)
link_libraries(glad)

add_executable(3_hello_triangle main.cpp)