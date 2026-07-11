# GLAD、三角形与 Shader 基础

对应项目：[`3-hello-triangle`](../3-hello-triangle/)

## 学习目标

这个项目在窗口和清屏流程上继续增加：

- 通过 GLAD 加载 OpenGL 函数；
- 监听 framebuffer 尺寸变化；
- 查询当前 OpenGL 版本；
- 使用旧式即时模式提交三个顶点并绘制彩色三角形。

## 为什么需要 GLAD

OpenGL 规范定义了 API，但操作系统、驱动和硬件不一定以普通链接符号的方式直接暴露所有 OpenGL 函数。尤其是较新版本的函数和扩展函数，通常需要在创建上下文之后动态查询函数地址。

GLAD 是基于 OpenGL、OpenGL ES、EGL、GLX、WGL 和 Vulkan 等规范生成加载代码的工具。它的核心作用是：

1. 声明所选版本和扩展对应的类型与函数指针；
2. 在运行时通过平台提供的地址查询函数加载实现；
3. 让应用能够判断当前上下文实际暴露了哪些版本和扩展能力。

GLAD 不能让旧显卡凭空支持新功能。它负责“找到可用函数”，应用仍然需要根据实际版本与扩展决定是否使用某项功能以及是否提供降级路径。

## 初始化顺序

### 1. 先创建并激活上下文

```cpp
GLFWwindow* window = glfwCreateWindow(800, 600, "LearnOpenGL", nullptr, nullptr);
glfwMakeContextCurrent(window);
```

动态加载器需要从当前 OpenGL 上下文查询函数，因此 GLAD 初始化必须发生在上下文创建并激活之后。

### 2. 再加载函数

```cpp
if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
    std::cout << "Failed to initialize GLAD" << std::endl;
    return -1;
}
```

GLFW 的 `glfwGetProcAddress` 负责按名字查询当前上下文中的 OpenGL 函数地址，GLAD 将查询结果存进生成的函数指针。

### 3. 头文件顺序

```cpp
#include <glad/glad.h>
#include <GLFW/glfw3.h>
```

`glad.h` 应放在 `glfw3.h` 之前。GLAD 需要接管 OpenGL 类型和函数声明；如果 GLFW 已经通过系统头文件包含了 OpenGL 声明，随后再包含 GLAD 可能产生重复声明或被 GLAD 主动拒绝。

也可以在包含 GLFW 前定义 `GLFW_INCLUDE_NONE`，明确要求 GLFW 不包含系统 OpenGL 头文件。不过本项目使用“先 GLAD、后 GLFW”的顺序已经满足要求。

## framebuffer 与 viewport

```cpp
glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

void framebuffer_size_callback(GLFWwindow*, int width, int height) {
    glViewport(0, 0, width, height);
}
```

窗口逻辑尺寸和 framebuffer 像素尺寸不一定相同，在 Retina 屏幕上尤其如此。回调提供的是 framebuffer 的像素尺寸，`glViewport` 决定标准化设备坐标最终映射到 framebuffer 的哪个矩形区域。

## 当前项目如何绘制三角形

```cpp
glBegin(GL_TRIANGLES);

glColor3f(1.0f, 0.0f, 0.0f);
glVertex3f(0.0f, 0.5f, 0.0f);

glColor3f(0.0f, 1.0f, 0.0f);
glVertex3f(-0.5f, -0.5f, 0.0f);

glColor3f(0.0f, 0.0f, 1.0f);
glVertex3f(0.5f, -0.5f, 0.0f);

glEnd();
```

`GL_TRIANGLES` 每三个顶点组成一个独立三角形。三个顶点分别设置红、绿、蓝，光栅化过程中顶点颜色会在三角形内部插值。

这里使用的是固定功能管线的即时模式。`glBegin`、`glColor3f`、`glVertex3f` 和 `glEnd` 已不属于现代 OpenGL Core Profile。现代写法通常会使用：

- Vertex Buffer Object（VBO）保存顶点数据；
- Vertex Array Object（VAO）描述顶点属性；
- vertex shader 处理顶点；
- fragment shader 计算片元颜色；
- `glDrawArrays` 或 `glDrawElements` 发起绘制。

## Vertex Shader 与 Fragment Shader

虽然当前代码没有自定义 shader，但理解它们有助于从即时模式过渡到现代 OpenGL。

### Vertex Shader

Vertex shader 针对每个顶点执行，常见职责是把顶点从模型局部坐标逐步变换到裁剪空间：

1. Model 变换：局部坐标 → 世界坐标；
2. View 变换：世界坐标 → 相机观察坐标；
3. Projection 变换：观察坐标 → 裁剪空间。

它还可以向后续阶段输出颜色、纹理坐标和法线等顶点属性。

### Fragment Shader

光栅化阶段将三角形覆盖区域转换为片元，并对 vertex shader 的输出进行插值。Fragment shader 接收这些插值结果，通过纹理采样、光照和材质计算等操作决定片元颜色，也可以丢弃片元。

片元通常对应一个候选像素，但还要经过深度、模板、混合等测试，不能简单地把 fragment 和最终屏幕像素视为完全相同的概念。

## 注意事项

- 应检查 `glfwInit()` 的返回值，当前项目没有处理初始化失败；
- GLAD 初始化失败时，退出前也应调用 `glfwTerminate()`；
- macOS 支持的 OpenGL 最高版本及 Profile 有平台限制，创建现代上下文时需要显式设置 GLFW window hints；
- `glGetString(GL_VERSION)` 必须在上下文激活且函数加载完成后调用。

