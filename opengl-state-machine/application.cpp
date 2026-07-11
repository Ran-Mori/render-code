#include "application.h"

#include "state_machine_lab.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <stdexcept>

namespace
{
constexpr int kWindowWidth = 960;
constexpr int kWindowHeight = 640;
}

Application::Application() = default;

Application::~Application()
{
    cleanUp();
}

int Application::run()
{
    initializeWindow();
    lab_.reset(new StateMachineLab());

    int framebufferWidth = 0;
    int framebufferHeight = 0;
    glfwGetFramebufferSize(window_, &framebufferWidth, &framebufferHeight);
    lab_->resize(framebufferWidth, framebufferHeight);
    refreshWindowTitle();

    while (!glfwWindowShouldClose(window_))
    {
        lab_->render();
        glfwSwapBuffers(window_);
        glfwPollEvents();
    }

    // OpenGL objects must be destroyed while their context is still current.
    lab_.reset();
    return 0;
}

void Application::initializeWindow()
{
    if (glfwInit() != GLFW_TRUE)
        throw std::runtime_error("Failed to initialize GLFW");

    glfwInitialized_ = true;
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    window_ = glfwCreateWindow(
        kWindowWidth,
        kWindowHeight,
        "OpenGL State Machine",
        nullptr,
        nullptr
    );
    if (window_ == nullptr)
        throw std::runtime_error("Failed to create GLFW window");

    glfwMakeContextCurrent(window_);
    glfwSwapInterval(1);
    glfwSetWindowUserPointer(window_, this);
    glfwSetKeyCallback(window_, keyCallback);
    glfwSetFramebufferSizeCallback(window_, framebufferSizeCallback);

    if (!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress)))
        throw std::runtime_error("Failed to initialize GLAD");
}

void Application::keyCallback(
    GLFWwindow* window,
    int key,
    int,
    int action,
    int
)
{
    Application* application = static_cast<Application*>(glfwGetWindowUserPointer(window));
    if (application != nullptr)
        application->handleKey(key, action);
}

void Application::framebufferSizeCallback(GLFWwindow* window, int width, int height)
{
    Application* application = static_cast<Application*>(glfwGetWindowUserPointer(window));
    if (application != nullptr)
        application->handleFramebufferResize(width, height);
}

void Application::handleKey(int key, int action)
{
    if (action != GLFW_PRESS || lab_ == nullptr)
        return;

    if (key == GLFW_KEY_ESCAPE)
    {
        glfwSetWindowShouldClose(window_, true);
        return;
    }

    if (key == GLFW_KEY_SPACE)
        lab_->nextScenario();
    else if (key == GLFW_KEY_BACKSPACE)
        lab_->previousScenario();
    else if (key == GLFW_KEY_R)
        lab_->resetScenario();
    else if (key >= GLFW_KEY_0 && key <= GLFW_KEY_5)
        lab_->selectScenario(key - GLFW_KEY_0);
    else
        return;

    refreshWindowTitle();
}

void Application::handleFramebufferResize(int width, int height)
{
    if (lab_ != nullptr)
        lab_->resize(width, height);
}

void Application::refreshWindowTitle()
{
    const std::string title = lab_->windowTitle();
    glfwSetWindowTitle(window_, title.c_str());
}

void Application::cleanUp()
{
    lab_.reset();

    if (window_ != nullptr)
    {
        glfwDestroyWindow(window_);
        window_ = nullptr;
    }

    if (glfwInitialized_)
    {
        glfwTerminate();
        glfwInitialized_ = false;
    }
}
