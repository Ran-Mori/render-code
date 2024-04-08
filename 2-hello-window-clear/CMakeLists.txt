cmake_minimum_required(VERSION 3.25)
project(create_a_window)

set(CMAKE_CXX_STANDARD 14)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -v")

include_directories(/opt/homebrew/Cellar/glfw/3.4/include)

link_libraries(/opt/homebrew/Cellar/glfw/3.4/lib/libglfw.3.dylib)
link_libraries(/Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX14.4.sdk/System/Library/Frameworks/OpenGL.framework)

add_executable(2_hello_window_clear main.cpp)
