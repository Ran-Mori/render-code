cmake_minimum_required(VERSION 3.25)
project(hello_window_clear LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(glfw3 3.3 REQUIRED)
find_package(OpenGL REQUIRED)

add_executable(2-hello-window-clear main.cpp)

target_link_libraries(2-hello-window-clear PRIVATE glfw OpenGL::GL)

if(APPLE)
    target_compile_definitions(2-hello-window-clear PRIVATE GL_SILENCE_DEPRECATION)
endif()
