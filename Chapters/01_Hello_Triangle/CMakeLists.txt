set(target_name 01_Hello_Triangle)

add_executable(${target_name}
        main.cpp
)

target_link_libraries(${target_name}
        PRIVATE
        glfw
        OpenGL::GL
)