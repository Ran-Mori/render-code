cmake_minimum_required(VERSION 3.25)
project(2-hello-window-clear)

set(CMAKE_CXX_STANDARD 14)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -v")

add_executable(2-hello-window-clear main.cpp)

target_include_directories(2-hello-window-clear PUBLIC /opt/homebrew/Cellar/glfw/3.4/include)

target_link_libraries(2-hello-window-clear
        /opt/homebrew/Cellar/glfw/3.4/lib/libglfw.3.dylib
        /Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX.sdk/System/Library/Frameworks/OpenGL.framework
)