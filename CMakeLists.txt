cmake_minimum_required(VERSION 4.4)
project(HelloPhysicsWorld)

set(CMAKE_CXX_STANDARD 20)

find_package(glfw3 CONFIG REQUIRED)
find_package(OpenGL REQUIRED)

add_subdirectory(Chapters/01_Hello_Triangle)