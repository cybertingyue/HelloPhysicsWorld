set(target_name 03_Hello_Collision)

add_executable(${target_name}
        main.cpp
)

target_link_libraries(${target_name}
        PRIVATE
        glfw
        OpenGL::GL
        Jolt::Jolt
)
