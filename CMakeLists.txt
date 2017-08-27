cmake_minimum_required (VERSION 2.6)
cmake_policy(SET CMP0042 OLD)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
set(CMAKE_CXX_STANDARD 14)


project (BOUNCING_PARTICLES2D)

#include (common.cmake)

# don't build in the source directory
if ("${CMAKE_BINARY_DIR}" STREQUAL "${CMAKE_SOURCE_DIR}")
  message (SEND_ERROR "Do not build in the source directory.")
  message (FATAL_ERROR "Remove the created \"CMakeCache.txt\" file and the \"CMakeFiles\" directory, then create a build directory and call \"${CMAKE_COMMAND} <path to the sources>\".")
endif ("${CMAKE_BINARY_DIR}" STREQUAL "${CMAKE_SOURCE_DIR}")

# finds all files with a given extension
macro (append_files files ext)
  foreach (dir ${ARGN})
    file (GLOB _files "${dir}/*.${ext}")
    list (APPEND ${files} ${_files})
  endforeach (dir)
endmacro (append_files)


append_files (Headers "h" ./bouncing_particles2D)
append_files (Sources "cpp" ./bouncing_particles2D)

find_package(OpenGL REQUIRED)
include_directories(${OPENGL_INCLUDE_DIR})
#include_directories(/usr/local/include/eigen3)

include_directories(/usr/local/include/)
link_directories(/usr/local/lib)

#add_executable(bouncing_particles bouncing_particles2D/bouncing_particles2D.cpp)

add_executable (bouncing_particles ${Headers} ${Sources})
target_link_libraries(bouncing_particles ${OPENGL_LIBRARIES} libglfw.3.dylib libGLEW.dylib)
