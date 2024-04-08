#include <GLFW/glfw3.h>
#include "iostream"
#include <chrono>

void processInput(GLFWwindow *window);
long getUnixTimeStamp();

int times = 0;

int main()
{

    GLFWwindow* window;

    /* Initialize the library */
    if (!glfwInit())
        return -1;

    /* Create a windowed mode window and its OpenGL context */
    window = glfwCreateWindow(640, 480, "Hello World", NULL, NULL);
    if (!window)
    {
        glfwTerminate();
        return -1;
    }

    /* Make the window's context current */
    glfwMakeContextCurrent(window);

    /* Loop until the user closes the window */
    while (!glfwWindowShouldClose(window))
    {
        std::cout << "while loop call times = " << times++  << "unix time = " << getUnixTimeStamp() << std::endl;
        /* process events */
        processInput(window);

        /* Render here */
        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        /* Swap front and back buffers */
        glfwSwapBuffers(window);

        /* Poll for and process events */
        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
}

void processInput(GLFWwindow* window) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
        std::cout << "processInput GLFW_KEY_ESCAPE == GLFW_PRESS call times = " << times++ << std::endl;
        glfwSetWindowShouldClose(window, true);
    }
}

long getUnixTimeStamp() {
    // Get the current time point
    std::chrono::system_clock::time_point now = std::chrono::system_clock::now();

    // Convert the time point to the Unix timestamp
    std::chrono::microseconds unixTimestamp = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch());

    // Extract the value of the Unix timestamp
    return unixTimestamp.count();
}