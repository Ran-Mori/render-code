cmake_minimum_required(VERSION 3.25)
project(4-circle C CXX)

set(CMAKE_CXX_STANDARD 14)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -v")
set(CMAKE_VERBOSE_MAKEFILE ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_library(glad ~/Environment/glad/src/glad.c)
target_include_directories(glad PUBLIC ~/Environment/glad/include)

add_executable(4-circle main.cpp)
target_include_directories(4-circle PUBLIC /opt/homebrew/Cellar/glfw/3.4/include)

target_link_libraries(4-circle
        glad
        /opt/homebrew/Cellar/glfw/3.4/lib/libglfw.3.dylib
)