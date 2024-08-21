#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "iostream"

void processInput(GLFWwindow *window);

void drawCircle();
void drawColorRing();

int main()
{
    // glfw: initialize and configure
    glfwInit();

    // glfw window creation
    GLFWwindow* window = glfwCreateWindow(800, 800, "DrawCircle", nullptr, nullptr);
    if (window == nullptr)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);

    // glad: load all OpenGL function pointers
    // ---------------------------------------
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    // print gl version
    std::cout << "opengl version  = " << glGetString(GL_VERSION) << std::endl;

    // render loop
    while (!glfwWindowShouldClose(window))
    {
        // input
        processInput(window);
        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

//        drawCircle();
        drawColorRing();

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // glfw: terminate, clearing all previously allocated GLFW resources.
    // ------------------------------------------------------------------
    glfwTerminate();
    return 0;
}

// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
// ---------------------------------------------------------------------------------------------------------
void processInput(GLFWwindow *window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
}

void drawCircle() {
    const float pi = 3.141592653589793238462643383279502884197;
    const float radius = 0.7;
    const unsigned int count = 100;

    for (int i = 0; i < count; ++i) {
        float angle = 2 * pi / count * (float )i;
        float next_angle = 2 * pi / count * (float )(i + 1);

        glBegin(GL_TRIANGLES);
        glVertex3f(0.0, 0.0, 0.0);
        glVertex3f(cosf(angle) * radius, sinf(angle) * radius, 0.0);
        glVertex3f(cosf(next_angle) * radius, sinf(next_angle) * radius, 0.0);
        glEnd();
    }
}


void drawColorRing() {
    const float pi = 3.141592653589793238462643383279502884197;
    const float inner_radius = 0.4;
    const float outer_radius = 0.8;
    const unsigned int count = 100;

    for (int i = 0; i < count; ++i) {
        float angle = 2 * pi / count * (float )i;
        float next_angle = 2 * pi / count * (float )(i + 1);

        glBegin(GL_TRIANGLES);
        glVertex3f(cosf(angle) * inner_radius, sinf(angle) * inner_radius, 0.0);
        glVertex3f(cosf(angle) * outer_radius, sinf(angle) * outer_radius, 0.0);
        glVertex3f(cosf(next_angle) * inner_radius, sinf(next_angle) * inner_radius, 0.0);
        glEnd();

        glBegin(GL_TRIANGLES);
        glVertex3f(cosf(next_angle) * inner_radius, sinf(next_angle) * inner_radius, 0.0);
        glVertex3f(cosf(next_angle) * outer_radius, sinf(next_angle) * outer_radius, 0.0);
        glVertex3f(cosf(angle) * outer_radius, sinf(angle) * outer_radius, 0.0);
        glEnd();
    }
}
