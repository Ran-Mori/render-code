# 创建窗口与 OpenGL 上下文

对应项目：[`1-create-a-window`](../1-create-a-window/)

## 学习目标

这个项目建立一个最小可运行的 OpenGL 程序，重点不是绘制复杂内容，而是理解 OpenGL 绘制之前需要准备什么：

1. 初始化 GLFW。
2. 创建窗口及其 OpenGL 上下文。
3. 将上下文设为当前线程的上下文。
4. 在渲染循环中交换缓冲区并处理事件。
5. 退出前释放 GLFW 资源。

## GLFW 与 OpenGL 的关系

OpenGL 是图形 API，负责发出清屏、绘制、纹理和着色等渲染命令，但它本身不负责：

- 创建操作系统窗口；
- 接收鼠标和键盘输入；
- 创建和管理 OpenGL 上下文；
- 处理不同桌面操作系统之间的窗口系统差异。

GLFW 补充了这些能力。它提供跨平台的窗口、上下文、输入和事件 API，让应用可以把主要精力放在 OpenGL 绘制逻辑上。

需要区分两层职责：GLFW 创建并管理“OpenGL 可以在哪里工作”的环境，OpenGL 负责“向这个环境绘制什么”。GLFW 还支持创建 OpenGL ES 或 Vulkan 所需的窗口环境，因此 GLFW 库本身并不等同于 OpenGL 渲染器。

## 代码执行顺序

### 1. 初始化 GLFW

```cpp
if (!glfwInit())
    return -1;
```

必须先成功初始化 GLFW，之后才能调用窗口和上下文相关 API。

### 2. 创建窗口和上下文

```cpp
GLFWwindow* window = glfwCreateWindow(
    640, 480, "Hello World", nullptr, nullptr
);
```

这个调用创建一个 `640 × 480` 的窗口。对 OpenGL 项目而言，它同时创建与窗口关联的 OpenGL 上下文。返回空指针表示创建失败。

### 3. 激活上下文

```cpp
glfwMakeContextCurrent(window);
```

OpenGL 命令总是作用于当前线程的“当前上下文”。如果没有先激活上下文，后续 OpenGL 调用就没有明确的目标状态和绘制表面。

### 4. 运行渲染循环

```cpp
while (!glfwWindowShouldClose(window)) {
    glClear(GL_COLOR_BUFFER_BIT);
    glfwSwapBuffers(window);
    glfwPollEvents();
}
```

循环中的三步分别表示：

1. `glClear` 清理颜色缓冲区；
2. `glfwSwapBuffers` 将已经绘制完成的后缓冲区显示到窗口；
3. `glfwPollEvents` 处理窗口、键盘和鼠标等系统事件。

窗口通常使用双缓冲：应用在不可见的后缓冲区绘制，完成后一次性交换到前台。这样可以减少逐步绘制造成的闪烁和撕裂感。

### 5. 释放资源

```cpp
glfwTerminate();
```

程序退出前调用它，销毁 GLFW 创建的窗口、上下文及其他内部资源。

## 本项目的边界

这个示例已经建立了完整的应用骨架，但每一帧只清理颜色缓冲区，没有设置新的清屏颜色，也没有提交几何图元。下一步可以在同一个循环中增加输入处理和 `glClearColor`，观察窗口内容与循环行为。

