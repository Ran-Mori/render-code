cmake_minimum_required(VERSION 3.25)
project(opengl_state_machine LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(glfw3 3.3 REQUIRED)
find_package(OpenGL REQUIRED)

set(GLAD_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../third_party/glad")
set(GENERATED_INCLUDE_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated")

configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/learnopengl/root_directory.h.in"
    "${GENERATED_INCLUDE_DIR}/root_directory.h"
    @ONLY
)

add_library(glad STATIC "${GLAD_ROOT}/src/glad.c")
target_include_directories(glad PUBLIC "${GLAD_ROOT}/include")

add_executable(
    opengl-state-machine
    main.cpp
    application.cpp
    geometry.cpp
    state_machine_lab.cpp
)
target_include_directories(
    opengl-state-machine
    PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}"
        "${GENERATED_INCLUDE_DIR}"
)
target_link_libraries(opengl-state-machine PRIVATE glad glfw OpenGL::GL)
