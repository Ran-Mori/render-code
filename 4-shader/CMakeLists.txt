cmake_minimum_required(VERSION 3.25)
project(hello_triangle LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(glfw3 3.3 REQUIRED)
find_package(OpenGL REQUIRED)

set(GLAD_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../third_party/glad")

add_library(glad STATIC "${GLAD_ROOT}/src/glad.c")
target_include_directories(glad PUBLIC "${GLAD_ROOT}/include")

add_executable(4-shader main.cpp)
target_include_directories(4-shader PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
target_link_libraries(4-shader PRIVATE glad glfw OpenGL::GL)
