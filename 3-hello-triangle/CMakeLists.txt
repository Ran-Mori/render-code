cmake_minimum_required(VERSION 3.25)
project(create_a_window C CXX)

set(CMAKE_CXX_STANDARD 14)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -v")

include_directories(/opt/homebrew/Cellar/glfw/3.4/include)
include_directories(~/Software/glad/include)

link_libraries(/opt/homebrew/Cellar/glfw/3.4/lib/libglfw.3.dylib)

add_executable(3_hello_triangle main.cpp
        ~/Software/glad/src/glad.c)