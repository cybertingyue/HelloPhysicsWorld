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

#include <Jolt/Core/JobSystemSingleThreaded.h>
#include <Jolt/Core/TempAllocator.h>
#include <Jolt/Physics/Body/BodyCreationSettings.h>
#include <Jolt/Physics/Collision/BroadPhase/BroadPhaseLayerInterfaceTable.h>
#include <Jolt/Physics/Collision/BroadPhase/ObjectVsBroadPhaseLayerFilterTable.h>
#include <Jolt/Physics/Collision/ContactListener.h>
#include <Jolt/Physics/Collision/ObjectLayerPairFilterTable.h>
#include <Jolt/Physics/Collision/Shape/SphereShape.h>
#include <Jolt/Physics/PhysicsSystem.h>

#include <algorithm>
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

class TerrainContacts final : public JPH::ContactListener
{
public:
    std::vector<JPH::RVec3> points;

    void OnContactAdded(const JPH::Body& first, const JPH::Body&, const JPH::ContactManifold& contact,
                        JPH::ContactSettings&) override
    {
        Collect(first, contact);
    }

    void OnContactPersisted(const JPH::Body& first, const JPH::Body&, const JPH::ContactManifold& contact,
                            JPH::ContactSettings&) override
    {
        Collect(first, contact);
    }

private:
    void Collect(const JPH::Body& first, const JPH::ContactManifold& contact)
    {
        for (JPH::uint i = 0; i < contact.mRelativeContactPointsOn1.size(); ++i)
        {
            // The hill is the static body. Jolt can pass it on either side.
            points.push_back(first.IsStatic()
                                 ? contact.GetWorldSpaceContactPointOn1(i)
                                 : contact.GetWorldSpaceContactPointOn2(i));
        }
    }
};

JPH::ShapeSettings::ShapeResult CreateTerrainShape()
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

    JPH::HeightFieldShapeSettings settings(heights.data(),
                                           JPH::Vec3(-center, 0.0f, -center), JPH::Vec3::sOne(), side);
    return settings.Create();
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
        uniform mat4 model;
        uniform vec2 clipScale;

        void main()
        {
            vec3 cameraPosition = (view * model * vec4(position, 1.0)).xyz;
            gl_Position = vec4(cameraPosition.xy * clipScale, 0.0, 1.0);
        }
    )");

    const GLuint fragment = CompileShader(GL_FRAGMENT_SHADER, R"(
        #version 330 core
        uniform vec3 lineColor;
        out vec4 color;

        void main()
        {
            color = vec4(lineColor, 1.0);
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

void SetMatrix(GLint location, const JPH::Mat44& matrix)
{
    JPH::Float4 columns[4];
    matrix.StoreFloat4x4(columns);
    glUniformMatrix4fv(location, 1, GL_FALSE, &columns[0].x);
}

int RunScene(GLFWwindow* window, GLuint program)
{
    constexpr JPH::ObjectLayer terrainLayer = 0;
    constexpr JPH::ObjectLayer sphereLayer = 1;

    JPH::ObjectLayerPairFilterTable pairs(2);
    pairs.EnableCollision(terrainLayer, sphereLayer);

    JPH::BroadPhaseLayerInterfaceTable layers(2, 2);
    layers.MapObjectToBroadPhaseLayer(terrainLayer, JPH::BroadPhaseLayer(0));
    layers.MapObjectToBroadPhaseLayer(sphereLayer, JPH::BroadPhaseLayer(1));
    JPH::ObjectVsBroadPhaseLayerFilterTable broadPhaseFilter(layers, 2, pairs, 2);

    JPH::TempAllocatorImpl scratch(10 * 1024 * 1024);
    JPH::JobSystemSingleThreaded jobs(JPH::cMaxPhysicsJobs);

    TerrainContacts contacts;
    JPH::PhysicsSystem physics;
    physics.Init(16, 0, 32, 16, layers, broadPhaseFilter, pairs);
    physics.SetContactListener(&contacts);
    auto& bodies = physics.GetBodyInterface();

    const auto terrain = CreateTerrainShape();
    if (terrain.HasError())
    {
        std::cerr << terrain.GetError().c_str() << '\n';
        return 1;
    }

    JPH::BodyCreationSettings ground(terrain.Get(), JPH::RVec3::sZero(),
                                     JPH::Quat::sIdentity(), JPH::EMotionType::Static, terrainLayer);
    const auto groundID = bodies.CreateAndAddBody(ground, JPH::EActivation::DontActivate);

    const JPH::RVec3 spawn(2.0f, 16.0f, 0.0f);
    JPH::RefConst<JPH::Shape> sphere = new JPH::SphereShape(0.8f);
    JPH::BodyCreationSettings ball(sphere, spawn, JPH::Quat::sIdentity(), JPH::EMotionType::Dynamic, sphereLayer);

    ball.mFriction = 0.6f;
    const auto ballID = bodies.CreateAndAddBody(ball, JPH::EActivation::Activate);

    // Store both wireframes once, in local coordinates.
    WireframeCollector renderer;
    terrain.Get()->Draw(&renderer, JPH::RMat44::sIdentity(), JPH::Vec3::sOne(),
                        JPH::Color::sWhite, false, true);

    const auto terrainCount = static_cast<GLsizei>(renderer.positions.size() / 3);
    sphere->Draw(&renderer, JPH::RMat44::sIdentity(), JPH::Vec3::sOne(),
                 JPH::Color::sWhite, false, true);

    const auto& vertices = renderer.positions;
    const auto sphereCount = static_cast<GLsizei>(vertices.size() / 3) - terrainCount;

    GLuint vertexArray = 0;
    GLuint vertexBuffer = 0;

    glGenVertexArrays(1, &vertexArray);
    glBindVertexArray(vertexArray);

    glGenBuffers(1, &vertexBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);

    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, nullptr);
    glEnableVertexAttribArray(0);

    // The marker uses one constant vertex position per draw.
    GLuint contactArray = 0;
    glGenVertexArrays(1, &contactArray);
    glPointSize(9.0f);

    glClearColor(0.08f, 0.10f, 0.14f, 1.0f);
    glUseProgram(program);

    const auto view = JPH::Mat44::sLookAt(
        JPH::Vec3(28.0f, 22.0f, 30.0f),
        JPH::Vec3(0.0f, 2.0f, 0.0f),
        JPH::Vec3::sAxisY());

    SetMatrix(glGetUniformLocation(program, "view"), view);

    const GLint clipScale = glGetUniformLocation(program, "clipScale");
    const GLint model = glGetUniformLocation(program, "model");
    const GLint lineColor = glGetUniformLocation(program, "lineColor");

    constexpr double step = 1.0 / 60.0;
    double previousTime = glfwGetTime();
    double accumulator = 0.0;
    JPH::Mat44 ballTransform = bodies.GetWorldTransform(ballID);

    while (!glfwWindowShouldClose(window))
    {
        const double now = glfwGetTime();
        accumulator += std::min(now - previousTime, 0.25);
        previousTime = now;

        while (accumulator >= step)
        {
            contacts.points.clear();
            physics.Update(static_cast<float>(step), 1, &scratch, &jobs);
            ballTransform = bodies.GetWorldTransform(ballID);
            accumulator -= step;
        }

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

            glBindVertexArray(vertexArray);
            SetMatrix(model, JPH::Mat44::sIdentity());
            glUniform3f(lineColor, 1.0f, 0.45f, 0.15f);
            glDrawArrays(GL_LINES, 0, terrainCount);

            SetMatrix(model, ballTransform);
            glUniform3f(lineColor, 0.77f, 0.71f, 0.87f);
            glDrawArrays(GL_LINES, terrainCount, sphereCount);

            glBindVertexArray(contactArray);
            SetMatrix(model, JPH::Mat44::sIdentity());
            glUniform3f(lineColor, 1.0f, 0.96f, 0.32f);
            for (const auto& point: contacts.points)
            {
                glVertexAttrib3f(0, point.GetX(), point.GetY(), point.GetZ());
                glDrawArrays(GL_POINTS, 0, 1);
            }
        }

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glDeleteVertexArrays(1, &contactArray);
    glDeleteVertexArrays(1, &vertexArray);
    glDeleteBuffers(1, &vertexBuffer);

    bodies.RemoveBody(ballID);
    bodies.DestroyBody(ballID);
    bodies.RemoveBody(groundID);
    bodies.DestroyBody(groundID);

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

    GLFWwindow* window = glfwCreateWindow(800, 600, "3 Hello Collision", nullptr, nullptr);
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

    JPH::RegisterDefaultAllocator();
    JPH::Factory factory;
    JPH::Factory::sInstance = &factory;
    JPH::RegisterTypes();

    const int result = RunScene(window, program);
    JPH::UnregisterTypes();
    JPH::Factory::sInstance = nullptr;

    glDeleteProgram(program);
    glfwDestroyWindow(window);
    glfwTerminate();

    return result;
}
