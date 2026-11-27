cmake_minimum_required(VERSION 4.4)
project(HelloPhysicsWorld)

set(CMAKE_CXX_STANDARD 20)

find_package(glfw3 CONFIG REQUIRED)
find_package(OpenGL REQUIRED)

include(scripts/jolt_install_path.cmake)
if(NOT Jolt_DIR)
    set(Jolt_DIR "${PROJECT_SOURCE_DIR}/${jolt_install_dir}/lib/cmake/Jolt")
endif()

find_package(Jolt CONFIG QUIET)

add_subdirectory(Chapters/01_Hello_Triangle)
if(Jolt_FOUND)
    add_subdirectory(Chapters/02_Hello_Heightmap)
else()
    message(STATUS "To enable chapter 2 onward, run cmake -P scripts/setup_jolt_physics.cmake from the project root and reload CMake.")
endif()
