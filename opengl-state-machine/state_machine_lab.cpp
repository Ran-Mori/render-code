#include "state_machine_lab.h"

#include "geometry.h"

#include <glad/glad.h>
#include <learnopengl/filesystem.h>
#include <learnopengl/shader_s.h>

#include <array>
#include <iostream>
#include <stdexcept>

namespace
{
constexpr int kScenarioCount = 6;

const std::array<const char*, kScenarioCount> kScenarioNames = {
    "Baseline: explicit state for both objects",
    "VAO leak: object B keeps object A's VAO",
    "Blend leak: object B inherits GL_BLEND",
    "Scissor leak: object B is clipped away",
    "Polygon mode leak: object B inherits GL_LINE",
    "Explicit restore: object B establishes its own state"
};

const std::array<const char*, kScenarioCount> kScenarioExplanations = {
    "Both draw calls explicitly bind the geometry and fixed-function state they need.",
    "The second draw omits glBindVertexArray(rectangle.vao), so the current triangle VAO remains active.",
    "The first draw enables blending. The second draw does not disable it, so alpha unexpectedly affects object B.",
    "The first draw enables a left-half scissor box. The second draw does not disable it, so object B is rejected.",
    "The first draw selects GL_LINE. The second draw does not restore GL_FILL, so object B is also wireframe.",
    "Object A uses several special states. Object B explicitly restores every state boundary before drawing."
};

const float kTriangleVertices[] = {
    -0.30f, -0.30f,
     0.30f, -0.30f,
     0.00f,  0.35f,
     0.00f,  0.00f,
     0.00f,  0.00f,
     0.00f,  0.00f
};

const float kRectangleVertices[] = {
    -0.28f, -0.30f,
     0.28f, -0.30f,
     0.28f,  0.30f,
     0.28f,  0.30f,
    -0.28f,  0.30f,
    -0.28f, -0.30f
};

const char* enabledLabel(GLenum capability)
{
    return glIsEnabled(capability) == GL_TRUE ? "Enabled" : "Disabled";
}
}

StateMachineLab::StateMachineLab()
{
    const std::string vertexPath = FileSystem::getPath("state-machine.vs");
    const std::string fragmentPath = FileSystem::getPath("state-machine.fs");
    shader_.reset(new Shader(vertexPath.c_str(), fragmentPath.c_str()));

    uniforms_ = {
        glGetUniformLocation(shader_->id(), "uOffset"),
        glGetUniformLocation(shader_->id(), "uScale"),
        glGetUniformLocation(shader_->id(), "uColor")
    };
    if (uniforms_.offset < 0 || uniforms_.scale < 0 || uniforms_.color < 0)
        throw std::runtime_error("Failed to resolve state-machine shader uniforms");

    triangle_.reset(new Geometry(
        kTriangleVertices,
        sizeof(kTriangleVertices) / sizeof(kTriangleVertices[0])
    ));
    rectangle_.reset(new Geometry(
        kRectangleVertices,
        sizeof(kRectangleVertices) / sizeof(kRectangleVertices[0])
    ));

    std::cout
        << "\nOpenGL State Machine Lab\n"
        << "  Space      next scenario\n"
        << "  Backspace  previous scenario\n"
        << "  0..5       select scenario directly\n"
        << "  R          reset to baseline\n"
        << "  Esc        quit\n"
        << std::endl;
    announceScenario();
}

StateMachineLab::~StateMachineLab() = default;

void StateMachineLab::render()
{
    establishKnownFrameState();
    glClearColor(0.035f, 0.075f, 0.12f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    renderCurrentScenario();

    if (needsStateSnapshot_)
    {
        printStateSnapshot();
        needsStateSnapshot_ = false;
    }
}

void StateMachineLab::resize(int width, int height)
{
    framebufferWidth_ = width;
    framebufferHeight_ = height;
    glViewport(0, 0, width, height);
    needsStateSnapshot_ = true;
}

void StateMachineLab::nextScenario()
{
    selectScenario(scenarioIndex() + 1);
}

void StateMachineLab::previousScenario()
{
    selectScenario(scenarioIndex() - 1);
}

void StateMachineLab::selectScenario(int index)
{
    const int wrappedIndex = (index % kScenarioCount + kScenarioCount) % kScenarioCount;
    scenario_ = static_cast<Scenario>(wrappedIndex);
    needsStateSnapshot_ = true;
    announceScenario();
}

void StateMachineLab::resetScenario()
{
    selectScenario(0);
}

int StateMachineLab::scenarioIndex() const
{
    return static_cast<int>(scenario_);
}

const char* StateMachineLab::scenarioName() const
{
    return kScenarioNames[scenarioIndex()];
}

std::string StateMachineLab::windowTitle() const
{
    return
        "OpenGL State Machine [" + std::to_string(scenarioIndex()) + "/5] - " +
        scenarioName() + " | Space / Backspace / 0..5 / R";
}

void StateMachineLab::establishKnownFrameState() const
{
    shader_->use();
    glBindVertexArray(0);
    glDisable(GL_BLEND);
    glDisable(GL_SCISSOR_TEST);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    glViewport(0, 0, framebufferWidth_, framebufferHeight_);
}

void StateMachineLab::renderCurrentScenario()
{
    switch (scenario_)
    {
        case Scenario::Baseline:
            drawLeftTriangle();
            glDisable(GL_BLEND);
            glDisable(GL_SCISSOR_TEST);
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
            drawRightRectangle();
            break;

        case Scenario::VaoLeak:
            drawLeftTriangle();
            // Intentionally draw without binding the rectangle VAO.
            drawRightRectangle(1.0f, false);
            break;

        case Scenario::BlendLeak:
            glEnable(GL_BLEND);
            drawLeftTriangle(0.48f);
            // GL_BLEND intentionally remains enabled for object B.
            drawRightRectangle(0.32f);
            break;

        case Scenario::ScissorLeak:
            glEnable(GL_SCISSOR_TEST);
            glScissor(0, 0, framebufferWidth_ / 2, framebufferHeight_);
            drawLeftTriangle();
            // GL_SCISSOR_TEST intentionally remains enabled for object B.
            drawRightRectangle();
            break;

        case Scenario::PolygonModeLeak:
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
            drawLeftTriangle();
            // GL_LINE intentionally remains current for object B.
            drawRightRectangle();
            break;

        case Scenario::ExplicitRestore:
            glEnable(GL_BLEND);
            glEnable(GL_SCISSOR_TEST);
            glScissor(0, 0, framebufferWidth_ / 2, framebufferHeight_);
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
            drawLeftTriangle(0.60f);

            glDisable(GL_BLEND);
            glDisable(GL_SCISSOR_TEST);
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
            drawRightRectangle();
            break;
    }
}

void StateMachineLab::setObjectUniforms(
    float offsetX,
    float offsetY,
    float scale,
    float red,
    float green,
    float blue,
    float alpha
) const
{
    glUniform2f(uniforms_.offset, offsetX, offsetY);
    glUniform1f(uniforms_.scale, scale);
    glUniform4f(uniforms_.color, red, green, blue, alpha);
}

void StateMachineLab::drawLeftTriangle(float alpha) const
{
    setObjectUniforms(-0.48f, 0.0f, 0.9f, 0.20f, 0.65f, 1.0f, alpha);
    triangle_->draw();
}

void StateMachineLab::drawRightRectangle(float alpha, bool bindOwnVao) const
{
    setObjectUniforms(0.48f, 0.0f, 0.9f, 1.0f, 0.43f, 0.16f, alpha);

    if (bindOwnVao)
        rectangle_->draw();
    else
        rectangle_->drawBound();
}

void StateMachineLab::announceScenario() const
{
    std::cout << "\n============================================================\n"
              << "Scenario " << scenarioIndex() << ": " << scenarioName() << "\n"
              << kScenarioExplanations[scenarioIndex()] << "\n"
              << "============================================================\n";
}

void StateMachineLab::printStateSnapshot() const
{
    int currentProgram = 0;
    int currentVao = 0;
    int polygonMode[2] = {0, 0};
    int viewport[4] = {0, 0, 0, 0};

    glGetIntegerv(GL_CURRENT_PROGRAM, &currentProgram);
    glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &currentVao);
    glGetIntegerv(GL_POLYGON_MODE, polygonMode);
    glGetIntegerv(GL_VIEWPORT, viewport);

    std::cout << "State after object B:\n"
              << "  GL_CURRENT_PROGRAM       = " << currentProgram << "\n"
              << "  GL_VERTEX_ARRAY_BINDING  = " << currentVao << "\n"
              << "  GL_BLEND                 = " << enabledLabel(GL_BLEND) << "\n"
              << "  GL_SCISSOR_TEST          = " << enabledLabel(GL_SCISSOR_TEST) << "\n"
              << "  GL_POLYGON_MODE          = "
              << (polygonMode[0] == GL_LINE ? "GL_LINE" : "GL_FILL") << "\n"
              << "  GL_VIEWPORT              = "
              << viewport[0] << ", " << viewport[1] << ", "
              << viewport[2] << ", " << viewport[3] << "\n"
              << std::flush;
}
