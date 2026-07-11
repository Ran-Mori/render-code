#ifndef APPLICATION_H
#define APPLICATION_H

#include <memory>

struct GLFWwindow;
class StateMachineLab;

class Application
{
public:
    Application();
    ~Application();

    Application(const Application&) = delete;
    Application& operator=(const Application&) = delete;

    int run();

private:
    static void keyCallback(
        GLFWwindow* window,
        int key,
        int scanCode,
        int action,
        int modifiers
    );
    static void framebufferSizeCallback(GLFWwindow* window, int width, int height);

    void initializeWindow();
    void handleKey(int key, int action);
    void handleFramebufferResize(int width, int height);
    void refreshWindowTitle();
    void cleanUp();

    GLFWwindow* window_ = nullptr;
    std::unique_ptr<StateMachineLab> lab_;
    bool glfwInitialized_ = false;
};

#endif
