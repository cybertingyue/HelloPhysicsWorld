set(target_name 02_Hello_Heightmap)

add_executable(${target_name}
        main.cpp
)

target_link_libraries(${target_name}
        PRIVATE
        glfw
        OpenGL::GL
        Jolt::Jolt
)