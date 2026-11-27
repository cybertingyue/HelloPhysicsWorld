#include <iostream>
#ifdef __APPLE__
#include <OpenGL/gl3.h>
#else
#define GL_GLEXT_PROTOTYPES
#include <GL/glcorearb.h>
#endif

#include <GLFW/glfw3.h>

#include <Jolt/Jolt.h>
#include <Jolt/Core/Factory.h>
#include <Jolt/Core/Memory.h>
#include <Jolt/Math/Float4.h>
#include <Jolt/Physics/Collision/Shape/HeightFieldShape.h>
#include <Jolt/RegisterTypes.h>
#include <Jolt/Renderer/DebugRendererSimple.h>

#include <array>
#include <cmath>
#include <vector>


class WireframeCollector final : public JPH::DebugRendererSimple
{
public:
    std::vector<float> positions;

    void DrawLine(JPH::RVec3Arg from, JPH::RVec3Arg to, JPH::ColorArg) override
    {
        AddVertex(from);
        AddVertex(to);
    }

    void DrawText3D(JPH::RVec3Arg, const JPH::string_view&, JPH::ColorArg, float) override
    {
    }

private:
    void AddVertex(JPH::RVec3Arg point)
    {
        positions.push_back(static_cast<float>(point.GetX()));
        positions.push_back(static_cast<float>(point.GetY()));
        positions.push_back(static_cast<float>(point.GetZ()));
    }
};

std::vector<float> BuildTerrainWireframe()
{
    JPH::RegisterDefaultAllocator();
    JPH::Factory factory;
    JPH::Factory::sInstance = &factory;
    JPH::RegisterTypes();

    std::vector<float> positions;
    {
        constexpr JPH::uint32 side = 32;
        constexpr float center = (side - 1) * 0.5f;
        std::array<float, side * side> heights{};

        for (JPH::uint32 row = 0; row < side; ++row)
        {
            for (JPH::uint32 col = 0; col < side; ++col)
            {
                const float dx = (static_cast<float>(col) - center) / 6.0f;
                const float dz = (static_cast<float>(row) - center) / 6.0f;
                heights[row * side + col] = 7.0f * std::exp(-(dx * dx + dz * dz));
            }
        }

        WireframeCollector renderer;

        JPH::HeightFieldShapeSettings settings(
            heights.data(), JPH::Vec3(-center, 0.0f, -center),
            JPH::Vec3::sOne(), side);

        const auto shape = settings.Create();

        if (shape.HasError())
        {
            std::cerr << "HeightField shape creation failed: " << shape.GetError().c_str() << std::endl;
        }
        else
        {
            shape.Get()->Draw(&renderer, JPH::RMat44::sIdentity(),
                              JPH::Vec3::sOne(), JPH::Color::sWhite,
                              false, true);

            positions.swap(renderer.positions);
        }
    }

    JPH::UnregisterTypes();
    JPH::Factory::sInstance = nullptr;

    return positions;
}


GLuint CompileShader(GLenum type, const char* source)
{
    const GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);

    GLint compiled = GL_FALSE;

    glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
    if (compiled == GL_TRUE)
        return shader;

    char log[2048]{};
    glGetShaderInfoLog(shader, sizeof(log), nullptr, log);
    std::cerr << "Shader compilation failed: " << log << std::endl;

    glDeleteShader(shader);

    return 0;
}

GLuint CreateProgram()
{
    const GLuint vertex = CompileShader(GL_VERTEX_SHADER, R"(
        #version 330 core
        layout(location = 0) in vec3 position;

        uniform mat4 view;
        uniform vec2 clipScale;

        void main()
        {
            vec3 cameraPosition = (view * vec4(position, 1.0)).xyz;
            gl_Position = vec4(cameraPosition.xy * clipScale, 0.0, 1.0);
        }
    )");

    const GLuint fragment = CompileShader(GL_FRAGMENT_SHADER, R"(
        #version 330 core
        out vec4 color;

        void main()
        {
            color = vec4(1.0, 0.45, 0.15, 1.0);
        }
    )");

    if (vertex == 0 || fragment == 0)
    {
        glDeleteShader(vertex);
        glDeleteShader(fragment);

        return 0;
    }

    const GLuint program = glCreateProgram();

    glAttachShader(program, vertex);
    glAttachShader(program, fragment);

    glLinkProgram(program);

    glDeleteShader(vertex);
    glDeleteShader(fragment);

    GLint linked = GL_FALSE;
    glGetProgramiv(program, GL_LINK_STATUS, &linked);
    if (linked == GL_TRUE)
        return program;

    char log[2048]{};
    glGetProgramInfoLog(program, sizeof(log), nullptr, log);
    std::cerr << "Program linking failed: " << log << std::endl;

    glDeleteProgram(program);

    return 0;
}

int main()
{
    glfwSetErrorCallback([](int code, const char* message)
    {
        std::cerr << "GLFW Error: " << code << ": " << message << std::endl;
    });

    if (!glfwInit())
        return 1;

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);

    GLFWwindow* window = glfwCreateWindow(800, 600, "Hello HeightMap", nullptr, nullptr);
    if (!window)
    {
        glfwTerminate();
        return 1;
    }

    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    const GLuint program = CreateProgram();
    if (program == 0)
    {
        glfwDestroyWindow(window);
        glfwTerminate();

        return 1;
    }

    const auto vertices = BuildTerrainWireframe();

    GLuint vertexArray = 0;
    GLuint vertexBuffer = 0;

    glGenVertexArrays(1, &vertexArray);
    glBindVertexArray(vertexArray);

    glGenBuffers(1, &vertexBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);

    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, nullptr);
    glEnableVertexAttribArray(0);

    glClearColor(0.08f, 0.10f, 0.14f, 1.0f);
    glUseProgram(program);

    const auto view = JPH::Mat44::sLookAt(
        JPH::Vec3(28.0f, 22.0f, 30.0f),
        JPH::Vec3(0.0f, 2.0f, 0.0f),
        JPH::Vec3::sAxisY());

    JPH::Float4 columns[4];
    view.StoreFloat4x4(columns);

    glUniformMatrix4fv(glGetUniformLocation(program, "view"), 1, GL_FALSE, &columns[0].x);

    const GLint clipScale = glGetUniformLocation(program, "clipScale");

    while (!glfwWindowShouldClose(window))
    {
        int width = 0;
        int height = 0;

        glfwGetFramebufferSize(window, &width, &height);
        glViewport(0, 0, width, height);

        glClear(GL_COLOR_BUFFER_BIT);

        if (width > 0 && height > 0)
        {
            const float aspect = static_cast<float>(width) / static_cast<float>(height);
            constexpr float scale = 1.0f / 24.0f;

            glUniform2f(clipScale,
                        aspect > 1.0f ? scale / aspect : scale,
                        aspect < 1.0f ? scale * aspect : scale);

            glDrawArrays(GL_LINES, 0, static_cast<GLsizei>(vertices.size() / 3));
        }

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glDeleteVertexArrays(1, &vertexArray);
    glDeleteBuffers(1, &vertexBuffer);
    glDeleteProgram(program);
    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}
