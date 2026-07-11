# GLAD、Shader 与索引绘制

对应项目：[`3-hello-triangle`](../3-hello-triangle/)

## 学习目标

这个项目在窗口和清屏流程之上，第一次搭建了现代 OpenGL 的完整绘制链路：

- 请求 OpenGL 3.3 Core Profile 上下文；
- 通过 GLAD 加载 OpenGL 函数；
- 编译 vertex shader 和 fragment shader，并链接为 shader program；
- 使用 VBO 保存顶点数据；
- 使用 EBO 保存顶点索引；
- 使用 VAO 记录顶点属性和 EBO 状态；
- 通过 `glDrawElements` 将两个三角形绘制成一个矩形；
- 使用线框模式观察两个三角形的组成关系。

当前代码已经不再使用 `glBegin`、`glVertex3f`、`glColor3f`、`glEnd` 等旧式即时模式 API。

## 当前程序最终画出了什么

代码定义了四个顶点：

```text
3 (-0.5,  0.5) -------- 0 ( 0.5,  0.5)
       |                  / |
       |                /   |
       |              /     |
       |            /       |
2 (-0.5, -0.5) -------- 1 ( 0.5, -0.5)
```

索引数组把它们组合成两个三角形：

```cpp
unsigned int indices[] = {
    0, 1, 3,  // 第一个三角形
    1, 2, 3   // 第二个三角形
};
```

因此几何形状是一个矩形，而不是单个三角形。代码还启用了：

```cpp
glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
```

所以当前实际画面是橙色线框矩形，并且能看到中间由两个三角形共享形成的对角线。若将这行注释掉，OpenGL 会恢复默认的 `GL_FILL` 模式，矩形内部将被橙色填满。

## 一帧是怎样被画出来的

程序可以按下面的顺序理解：

```text
创建 OpenGL 3.3 上下文
        ↓
GLAD 加载 OpenGL 函数
        ↓
编译 Vertex Shader 与 Fragment Shader
        ↓
链接 Shader Program
        ↓
创建并配置 VAO、VBO、EBO
        ↓
进入渲染循环
        ↓
清屏 → 使用 Program → 绑定 VAO → glDrawElements
        ↓
交换前后缓冲区并处理窗口事件
```

Shader 和缓冲对象都在进入渲染循环之前创建。每一帧只需要选择已经创建好的 program 和 VAO，然后发起绘制，不需要重复上传不变的顶点数据。

## 1. 创建 OpenGL 3.3 Core Profile 上下文

```cpp
glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif
```

这些 hint 要在 `glfwCreateWindow` 之前设置。这里请求的是 OpenGL 3.3 Core Profile，与 shader 中的 `#version 330 core` 保持一致。

Core Profile 不提供旧式即时模式绘制接口，因此必须使用 shader、缓冲对象和顶点数组对象完成绘制。

macOS 创建 Core Profile 上下文时通常还需要设置 `GLFW_OPENGL_FORWARD_COMPAT`。它是上下文创建条件，不是让旧硬件自动获得新 OpenGL 功能的开关。

窗口创建并激活上下文：

```cpp
GLFWwindow* window = glfwCreateWindow(
    SCR_WIDTH,
    SCR_HEIGHT,
    "LearnOpenGL",
    NULL,
    NULL
);

glfwMakeContextCurrent(window);
```

## 2. 为什么 GLAD 必须在上下文之后初始化

```cpp
if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
    std::cout << "Failed to initialize GLAD" << std::endl;
    return -1;
}
```

OpenGL 规范定义了 API，但较新版本函数通常需要在运行时从当前上下文查询地址。`glfwGetProcAddress` 负责查询，GLAD 将结果保存为应用可以调用的函数指针。

因此初始化顺序必须是：

1. 初始化 GLFW；
2. 创建窗口和 OpenGL 上下文；
3. 使用 `glfwMakeContextCurrent` 激活上下文；
4. 调用 `gladLoadGLLoader` 加载函数。

如果没有当前上下文，GLAD 就无法正确加载 `glCreateShader`、`glGenVertexArrays`、`glDrawElements` 等函数。

GLAD 也不能让驱动凭空支持某个 OpenGL 版本。它只负责加载当前上下文实际提供的函数。

### 头文件顺序

```cpp
#include <glad/glad.h>
#include <GLFW/glfw3.h>
```

`glad.h` 放在 `glfw3.h` 之前，让 GLAD 负责 OpenGL 类型和函数声明，避免 GLFW 提前包含系统 OpenGL 头文件而造成声明冲突。

## 3. Vertex Shader：确定顶点位置

```glsl
#version 330 core
layout (location = 0) in vec3 aPos;

void main()
{
    gl_Position = vec4(aPos.x, aPos.y, aPos.z, 1.0);
}
```

Vertex shader 对每个输入顶点执行一次。

`layout (location = 0)` 表示顶点位置从编号为 `0` 的顶点属性读取。后面配置 VAO 时，`glVertexAttribPointer` 和 `glEnableVertexAttribArray` 也必须使用同一个编号 `0`。

`gl_Position` 是 vertex shader 必须写入的内建输出，表示顶点的裁剪空间坐标。当前顶点数据已经直接位于可见的标准化范围内，也没有 Model、View、Projection 变换，因此 shader 只是把 `vec3` 位置扩展为齐次坐标 `vec4`：

```glsl
vec4(aPos.x, aPos.y, aPos.z, 1.0)
```

最后一个分量 `w = 1.0` 表示普通位置点。

## 4. Fragment Shader：确定片元颜色

```glsl
#version 330 core
out vec4 FragColor;

void main()
{
    FragColor = vec4(1.0f, 0.5f, 0.2f, 1.0f);
}
```

光栅化阶段将三角形覆盖的区域转换为片元，fragment shader 对每个片元执行并输出颜色。

这里没有从 vertex shader 接收颜色，也没有纹理采样；所有片元都输出同一个 RGBA 值：

- 红色：`1.0`；
- 绿色：`0.5`；
- 蓝色：`0.2`；
- Alpha：`1.0`。

因此图形呈橙色。Fragment 是一个候选像素，还可能经过深度、模板、裁剪和混合等测试，不能始终把 fragment 与最终 framebuffer 像素视为完全相同的概念。

## 5. 编译 Shader

Vertex shader 的创建和编译流程是：

```cpp
unsigned int vertexShader = glCreateShader(GL_VERTEX_SHADER);
glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
glCompileShader(vertexShader);
```

三个调用分别负责：

1. `glCreateShader`：创建指定类型的 shader 对象并返回句柄；
2. `glShaderSource`：把 GLSL 源码交给 shader 对象；
3. `glCompileShader`：请求驱动编译 GLSL。

Fragment shader 使用相同流程，只是类型换成 `GL_FRAGMENT_SHADER`。

编译是否成功需要主动查询：

```cpp
int success;
char infoLog[512];

glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
if (!success) {
    glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
    std::cout << infoLog << std::endl;
}
```

OpenGL 调用通常不会用 C++ 异常报告 GLSL 语法错误。`glCompileShader` 返回后，必须通过 `GL_COMPILE_STATUS` 和 info log 获取结果。

## 6. 链接 Shader Program

分别编译好的 shader 还不能直接用于绘制，需要链接为一个完整 program：

```cpp
unsigned int shaderProgram = glCreateProgram();
glAttachShader(shaderProgram, vertexShader);
glAttachShader(shaderProgram, fragmentShader);
glLinkProgram(shaderProgram);
```

链接阶段会检查不同 shader stage 之间的输入输出是否匹配。链接结果同样需要查询：

```cpp
glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
if (!success) {
    glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
}
```

链接成功后，program 已经保存了可执行结果，单独的 shader 对象可以删除：

```cpp
glDeleteShader(vertexShader);
glDeleteShader(fragmentShader);
```

删除 shader 对象不会破坏已经成功链接的 program。

## 7. 顶点数据与索引数据

### 顶点数组

```cpp
float vertices[] = {
     0.5f,  0.5f, 0.0f,  // 0：右上
     0.5f, -0.5f, 0.0f,  // 1：右下
    -0.5f, -0.5f, 0.0f,  // 2：左下
    -0.5f,  0.5f, 0.0f   // 3：左上
};
```

每个顶点由三个连续的 `float` 表示，即 `(x, y, z)`。这些值最终被 vertex shader 的 `aPos` 读取。

### 索引数组

```cpp
unsigned int indices[] = {
    0, 1, 3,
    1, 2, 3
};
```

如果不用索引，为两个三角形分别提交顶点，需要存储六组位置。使用索引后，只保存四个互不重复的顶点，再通过六个索引描述两个三角形，可以复用右下和左上两个顶点。

## 8. VBO、EBO 与 VAO 分别保存什么

### VBO：保存顶点原始字节

```cpp
glBindBuffer(GL_ARRAY_BUFFER, VBO);
glBufferData(
    GL_ARRAY_BUFFER,
    sizeof(vertices),
    vertices,
    GL_STATIC_DRAW
);
```

`glBufferData` 将 CPU 数组 `vertices` 的内容复制到当前绑定的 VBO。

`GL_STATIC_DRAW` 是一个使用提示，表示数据预计很少修改、会被多次用于绘制。它不强制驱动采用某种唯一的内存策略。

### EBO：保存顶点索引

```cpp
glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
glBufferData(
    GL_ELEMENT_ARRAY_BUFFER,
    sizeof(indices),
    indices,
    GL_STATIC_DRAW
);
```

EBO（Element Buffer Object，也常称 Index Buffer Object）保存 `glDrawElements` 读取的索引。

### VAO：保存一次绘制所需的顶点输入配置

```cpp
glBindVertexArray(VAO);
```

VAO 不是顶点数据本身。它记录如何解释顶点数据，以及与顶点输入相关的绑定状态。这个示例的 VAO 主要记录：

- 属性 `0` 的格式；
- 属性 `0` 从哪个 VBO 取数据；
- 属性 `0` 是否启用；
- 当前绑定的 EBO。

配置时应先绑定 VAO，再设置 VBO、EBO 和属性状态。

## 9. `glVertexAttribPointer` 如何解释顶点数据

```cpp
glVertexAttribPointer(
    0,
    3,
    GL_FLOAT,
    GL_FALSE,
    3 * sizeof(float),
    (void*)0
);
glEnableVertexAttribArray(0);
```

各参数含义如下：

| 参数 | 当前值 | 含义 |
| --- | --- | --- |
| 属性位置 | `0` | 对应 shader 的 `layout (location = 0)` |
| 分量数 | `3` | 每个位置由 x、y、z 三个分量组成 |
| 数据类型 | `GL_FLOAT` | 每个分量是 `float` |
| 归一化 | `GL_FALSE` | 不对浮点数据做归一化转换 |
| 步长 | `3 * sizeof(float)` | 相邻两个顶点位置之间相隔三个 `float` |
| 偏移 | `(void*)0` | 位置属性从 VBO 第 0 字节开始 |

调用 `glVertexAttribPointer` 时，OpenGL 会把当时绑定在 `GL_ARRAY_BUFFER` 上的 VBO 记录为属性 `0` 的数据来源。因此配置完成后可以解绑 `GL_ARRAY_BUFFER`：

```cpp
glBindBuffer(GL_ARRAY_BUFFER, 0);
```

之后重新绑定 VAO，属性 `0` 仍然知道应该从原来的 VBO 读取数据。

### 为什么配置 VAO 时不能提前解绑 EBO

`GL_ELEMENT_ARRAY_BUFFER` 的绑定属于 VAO 状态。如果在 VAO 仍绑定时执行：

```cpp
glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
```

当前 VAO 记录的 EBO 也会被清除，之后 `glDrawElements` 就没有索引缓冲可以读取。

所以当前代码保持 EBO 绑定，然后解绑 VAO：

```cpp
glBindVertexArray(0);
```

## 10. 渲染循环中的真正绘制调用

每一帧先清除颜色缓冲：

```cpp
glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
glClear(GL_COLOR_BUFFER_BIT);
```

然后选择 shader program 和 VAO：

```cpp
glUseProgram(shaderProgram);
glBindVertexArray(VAO);
```

最后发起索引绘制：

```cpp
glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
```

参数含义是：

1. `GL_TRIANGLES`：每三个索引构成一个独立三角形；
2. `6`：读取六个索引，因此生成两个三角形；
3. `GL_UNSIGNED_INT`：EBO 中每个索引的类型是 `unsigned int`；
4. `0`：从当前 EBO 的第 0 字节开始读取索引。

这次调用发生时，OpenGL 从 VAO 找到属性配置和 EBO，从 EBO 读取顶点编号，再从 VBO 获取对应位置，运行 vertex shader，组装三角形，进行裁剪和光栅化，最后运行 fragment shader。

代码中的这一行目前被注释：

```cpp
// glDrawArrays(GL_TRIANGLES, 0, 6);
```

它不能直接替代当前的 `glDrawElements`。当前 VBO 只有四个顶点，而该调用要求从 VBO 连续读取六个顶点；如果要改用 `glDrawArrays`，必须先把两个三角形展开成六个顶点，并且不再依赖 EBO。

## 11. 交换缓冲与处理事件

```cpp
glfwSwapBuffers(window);
glfwPollEvents();
```

窗口通常使用双缓冲：OpenGL 在后缓冲中绘制，`glfwSwapBuffers` 将完成的画面呈现出来，避免用户看到逐步绘制过程。

`glfwPollEvents` 处理窗口尺寸变化、键盘输入等系统事件。按下 Escape 时，`processInput` 会将窗口标记为应该关闭。

## 12. framebuffer 与 viewport

```cpp
glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

void framebuffer_size_callback(GLFWwindow*, int width, int height) {
    glViewport(0, 0, width, height);
}
```

窗口逻辑尺寸和 framebuffer 像素尺寸不一定相同，在 Retina 屏幕上尤其如此。回调参数是 framebuffer 的像素尺寸，`glViewport` 决定裁剪空间经过透视除法后的标准化设备坐标最终映射到 framebuffer 的哪个矩形区域。

## 13. 资源释放

退出渲染循环后，程序释放自己创建的 OpenGL 对象：

```cpp
glDeleteVertexArrays(1, &VAO);
glDeleteBuffers(1, &VBO);
glDeleteBuffers(1, &EBO);
glDeleteProgram(shaderProgram);
```

最后再调用：

```cpp
glfwTerminate();
```

OpenGL 对象属于当前上下文，因此应当在上下文仍然有效时释放。

## 对象与状态关系总结

| 对象 | 当前示例中保存或表达的内容 | 绘制时如何使用 |
| --- | --- | --- |
| Vertex Shader | 将 `aPos` 写入 `gl_Position` | 每个索引引用的顶点执行一次或复用缓存结果 |
| Fragment Shader | 输出固定橙色 | 为光栅化产生的片元计算颜色 |
| Shader Program | 链接后的完整 shader 管线 | 由 `glUseProgram` 设为当前 program |
| VBO | 四个顶点的位置数据 | VAO 的属性 `0` 从中读取三个 `float` |
| EBO | 六个顶点索引 | `glDrawElements` 按索引组装两个三角形 |
| VAO | 属性格式、VBO 关联和 EBO 绑定 | 绘制前绑定即可恢复顶点输入配置 |

## 当前代码值得注意的地方

- `glfwInit()` 的返回值尚未检查；
- GLAD 初始化失败时直接返回，退出前没有调用 `glfwTerminate()`；
- shader 编译或 program 链接失败后，代码打印日志但仍继续执行；
- 当前启用了线框模式；注释 `glPolygonMode` 后才会看到填充矩形；
- `#include <chrono>` 已经没有被当前代码使用；
- 当前 shader 只有位置输入，颜色是 fragment shader 中写死的常量，因此不能产生原版本那种顶点颜色插值效果。

这些事项不妨碍理解当前绘制主流程，但在更完整的项目中应增加错误处理，并把 shader 编译、链接和资源管理封装成独立组件。
