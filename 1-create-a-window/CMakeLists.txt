cmake_minimum_required(VERSION 3.25)
project(create_a_window LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(glfw3 3.3 REQUIRED)
find_package(OpenGL REQUIRED)

add_executable(1-create-a-window main.cpp)

target_link_libraries(1-create-a-window PRIVATE glfw OpenGL::GL)

if(APPLE)
    target_compile_definitions(1-create-a-window PRIVATE GL_SILENCE_DEPRECATION)
endif()
