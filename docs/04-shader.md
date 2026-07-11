# Shader 封装、顶点属性与颜色插值

对应项目：[`4-shader`](../4-shader/)

## 学习目标

这个项目延续上一章的现代 OpenGL 绘制链路，但把重点放到了 shader 的组织方式和顶点属性的数据流上：

- 将 vertex shader 和 fragment shader 从 C++ 字符串移到独立的 GLSL 文件；
- 使用 `Shader` 类完成文件读取、编译、链接和 program 激活；
- 在同一个 VBO 中交错保存位置与颜色；
- 使用两个顶点属性把位置和颜色分别送入 vertex shader；
- 将 vertex shader 的颜色输出传递给 fragment shader；
- 观察光栅化阶段对三个顶点颜色的自动插值；
- 使用 `glDrawArrays` 绘制一个不依赖 EBO 的三角形。

最终画面是一个填充三角形：右下角为红色，左下角为绿色，上方为蓝色，三角形内部显示三种颜色平滑混合后的渐变。

## 从 C++ 数据到屏幕颜色的完整路径

当前示例最核心的数据流可以概括为：

```text
vertices 数组
  ├─ 每个顶点的前 3 个 float：位置
  └─ 每个顶点的后 3 个 float：颜色
            ↓ glBufferData
           VBO
            ↓ VAO 中的两个属性描述
  location 0          location 1
     aPos                aColor
       ↓                   ↓
       └──── Vertex Shader ┘
              ↓ ourColor
       图元装配与光栅化
              ↓ 自动插值
       Fragment Shader
              ↓ FragColor
         颜色缓冲区
```

需要注意，VBO 只保存一段原始字节。它本身不知道哪些字节是位置、哪些字节是颜色。真正赋予这些字节含义的是 VAO 中由 `glVertexAttribPointer` 记录的属性格式。

## 1. `Shader` 类解决了什么问题

上一章在 `main.cpp` 中直接完成了 shader 源码定义、编译、错误检查和 program 链接。随着 shader 变长，这些代码会让主程序难以阅读。

当前项目使用：

```cpp
#include <learnopengl/shader_s.h>
```

并通过一行代码完成 shader program 的创建：

```cpp
Shader ourShader("3.3.shader.vs", "3.3.shader.fs");
```

这个构造函数内部依次执行：

1. 从磁盘读取 vertex shader 文件；
2. 从磁盘读取 fragment shader 文件；
3. 创建并编译两个 shader 对象；
4. 创建 program，并附加两个 shader；
5. 链接 program；
6. 删除链接后不再需要的独立 shader 对象；
7. 将最终 program 的 OpenGL 句柄保存在 `Shader::ID` 中。

`Shader` 类并不是 OpenGL 内置类型，而是 LearnOpenGL 示例为了减少重复代码提供的 C++ 封装。真正的 OpenGL program 仍然由 `glCreateProgram` 创建。

## 2. 从外部文件读取 GLSL 源码

构造函数接收两个文件路径：

```cpp
Shader(const char* vertexPath, const char* fragmentPath)
```

文件读取的主要过程是：

```cpp
std::ifstream vShaderFile;
std::ifstream fShaderFile;

vShaderFile.open(vertexPath);
fShaderFile.open(fragmentPath);

std::stringstream vShaderStream, fShaderStream;
vShaderStream << vShaderFile.rdbuf();
fShaderStream << fShaderFile.rdbuf();

vertexCode = vShaderStream.str();
fragmentCode = fShaderStream.str();
```

这里的对象职责分别是：

| 对象 | 作用 |
| --- | --- |
| `std::ifstream` | 打开并读取磁盘文件 |
| `std::stringstream` | 接收文件中的全部字符 |
| `std::string` | 在 C++ 侧保存完整 GLSL 源码 |
| `const char*` | 以 OpenGL API 要求的形式传给 `glShaderSource` |

转换代码：

```cpp
const char* vShaderCode = vertexCode.c_str();
const char* fShaderCode = fragmentCode.c_str();
```

这里的指针依赖对应 `std::string` 的存储空间。当前代码会在字符串仍然存活且未被修改时立即调用 `glShaderSource`，因此是安全的。`glShaderSource` 会复制源码，之后编译不再依赖这两个 C 字符串指针。

### 文件读取错误

代码为输入流启用了异常：

```cpp
vShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
fShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
```

打开失败或读取失败时，会进入 `catch` 并输出：

```text
ERROR::SHADER::FILE_NOT_SUCCESSFULLY_READ
```

当前实现打印错误后仍会继续编译空字符串，因此随后通常还会出现 shader 编译错误。在正式项目中，更合理的处理是让构造过程明确失败，而不是继续生成一个不可用的 program。

## 3. Shader 文件路径与运行目录

下面两个路径都是相对路径：

```cpp
Shader ourShader("3.3.shader.vs", "3.3.shader.fs");
```

相对路径不是相对于 `main.cpp`，也不是相对于可执行文件，而是相对于程序启动时的当前工作目录。

因此在项目目录中运行可以找到文件：

```bash
cd 4-shader
./build/4-shader
```

如果在仓库根目录直接执行：

```bash
./4-shader/build/4-shader
```

工作目录仍是仓库根目录，程序就会尝试读取根目录下的 `3.3.shader.vs` 和 `3.3.shader.fs`，从而读取失败。

这是运行时文件定位问题，与 C++ 的 `#include` 搜索路径不是一回事：

- `#include <learnopengl/shader_s.h>` 在编译阶段由编译器查找；
- `"3.3.shader.vs"` 和 `"3.3.shader.fs"` 在程序运行阶段由文件系统查找。

## 4. Vertex Shader：接收位置和颜色

Vertex shader 文件是 [`3.3.shader.vs`](../4-shader/3.3.shader.vs)：

```glsl
#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aColor;

out vec3 ourColor;

void main()
{
    gl_Position = vec4(aPos, 1.0);
    ourColor = aColor;
}
```

这个 shader 对每个顶点执行一次，并处理两类输入：

| GLSL 输入 | location | C++ 中的数据 | 用途 |
| --- | ---: | --- | --- |
| `aPos` | `0` | 每个顶点的前三个 `float` | 写入顶点位置 `gl_Position` |
| `aColor` | `1` | 每个顶点的后三个 `float` | 传递到后续渲染阶段 |

`gl_Position` 是 vertex shader 的内建输出。当前没有 Model、View、Projection 矩阵，顶点坐标直接作为裁剪空间坐标：

```glsl
gl_Position = vec4(aPos, 1.0);
```

`ourColor` 是程序自己定义的输出变量：

```glsl
out vec3 ourColor;
```

它只包含 RGB，不包含 Alpha。vertex shader 将当前顶点的 `aColor` 原样传给下一阶段。

## 5. Fragment Shader：接收插值后的颜色

Fragment shader 文件是 [`3.3.shader.fs`](../4-shader/3.3.shader.fs)：

```glsl
#version 330 core
out vec4 FragColor;

in vec3 ourColor;

void main()
{
    FragColor = vec4(ourColor, 1.0f);
}
```

Fragment shader 中的：

```glsl
in vec3 ourColor;
```

与 vertex shader 中的：

```glsl
out vec3 ourColor;
```

构成 stage 之间的接口。这里名称和类型都一致，所以 program 可以成功链接。

Fragment shader 把插值后的 RGB 与固定的 Alpha `1.0` 组合成 RGBA：

```glsl
FragColor = vec4(ourColor, 1.0f);
```

`FragColor` 最终作为当前片元的颜色输出。

## 6. 为什么三角形内部会出现渐变

三个顶点分别具有不同颜色：

```cpp
float vertices[] = {
     0.5f, -0.5f, 0.0f,  1.0f, 0.0f, 0.0f, // 右下：红
    -0.5f, -0.5f, 0.0f,  0.0f, 1.0f, 0.0f, // 左下：绿
     0.0f,  0.5f, 0.0f,  0.0f, 0.0f, 1.0f  // 上方：蓝
};
```

可以把画面理解为：

```text
                  蓝 (0, 0, 1)
                       /\
                      /  \
                     /    \
                    /      \
                   /        \
                  /__________\
       绿 (0, 1, 0)          红 (1, 0, 0)
```

Vertex shader 只对三个顶点执行并输出三份颜色。三角形内部却会生成大量片元，这些片元的颜色由光栅化阶段根据片元在三角形中的位置，对三个顶点输出进行插值。

因此：

- 越接近红色顶点，红色分量越大；
- 越接近绿色顶点，绿色分量越大；
- 越接近蓝色顶点，蓝色分量越大；
- 三角形内部同时受到多个顶点影响，形成连续渐变。

Fragment shader 每次执行时拿到的 `ourColor` 通常已经是插值结果，不是简单地从某一个顶点复制来的颜色。默认情况下，浮点型 vertex shader 输出会进行透视正确插值。

## 7. 一个 VBO 如何同时保存位置与颜色

当前顶点采用交错布局，每个顶点连续保存六个 `float`：

```text
顶点 0：[Px Py Pz | R G B]
顶点 1：[Px Py Pz | R G B]
顶点 2：[Px Py Pz | R G B]
```

单个顶点的总大小，也就是相邻两个同类属性之间的步长，是：

```cpp
6 * sizeof(float)
```

位置属性配置为：

```cpp
glVertexAttribPointer(
    0,
    3,
    GL_FLOAT,
    GL_FALSE,
    6 * sizeof(float),
    (void*)0
);
glEnableVertexAttribArray(0);
```

颜色属性配置为：

```cpp
glVertexAttribPointer(
    1,
    3,
    GL_FLOAT,
    GL_FALSE,
    6 * sizeof(float),
    (void*)(3 * sizeof(float))
);
glEnableVertexAttribArray(1);
```

二者的差别是属性编号和起始偏移：

| 属性 | location | 分量数 | stride | offset |
| --- | ---: | ---: | --- | --- |
| 位置 | `0` | 3 | `6 * sizeof(float)` | `0` |
| 颜色 | `1` | 3 | `6 * sizeof(float)` | `3 * sizeof(float)` |

颜色要跳过开头的三个位置分量，所以偏移是 `3 * sizeof(float)`。

### 读取过程示例

当 OpenGL 读取顶点 1 时：

- 属性 `0` 从顶点 1 起始位置读取三个 `float`，得到 `(-0.5, -0.5, 0.0)`；
- 属性 `1` 从顶点 1 起始位置再偏移三个 `float`，读取 `(0.0, 1.0, 0.0)`。

这两个属性会在同一次 vertex shader 执行中分别成为 `aPos` 和 `aColor`。

## 8. VAO 记录了哪些关系

配置前先绑定 VAO 和 VBO：

```cpp
glBindVertexArray(VAO);
glBindBuffer(GL_ARRAY_BUFFER, VBO);
glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
```

随后两次 `glVertexAttribPointer` 把下列关系记录到当前 VAO：

- location `0` 从当前 VBO 读取位置；
- location `1` 从当前 VBO 读取颜色；
- 两个属性的数据类型、分量数、步长和偏移；
- 两个属性都处于启用状态。

VAO 不会复制 VBO 中的顶点数据。它保存的是“怎样从 VBO 解释顶点输入”的状态。

## 9. `Shader::use()` 为什么要在绘制前调用

`Shader` 类中的：

```cpp
void use()
{
    glUseProgram(ID);
}
```

只是对 `glUseProgram` 的简单封装。渲染循环中调用：

```cpp
ourShader.use();
```

表示把 `ourShader.ID` 对应的 program 设为当前 program。之后的绘制命令才会使用这个 program 中链接好的 vertex shader 和 fragment shader。

创建 `Shader` 对象不等于自动永久启用它。OpenGL 是状态机，如果程序中有多个 shader program，每次绘制前需要选择这次绘制要使用的 program。

## 10. 使用 `glDrawArrays` 绘制三角形

当前 VBO 已经按绘制顺序直接保存三个顶点，因此不需要 EBO：

```cpp
glDrawArrays(GL_TRIANGLES, 0, 3);
```

参数含义是：

1. `GL_TRIANGLES`：每三个顶点组成一个独立三角形；
2. `0`：从编号为 0 的顶点开始；
3. `3`：连续读取三个顶点。

调用发生时，OpenGL 会结合当前状态完成以下工作：

```text
当前 VAO 找到两个顶点属性及其 VBO
                 ↓
连续读取 3 个顶点的位置和颜色
                 ↓
对每个顶点执行 vertex shader
                 ↓
将 3 个顶点组装为三角形
                 ↓
裁剪、透视除法、viewport 变换
                 ↓
光栅化并插值 ourColor
                 ↓
对产生的片元执行 fragment shader
                 ↓
结果写入颜色缓冲区
```

上一章的矩形需要复用顶点，所以使用 EBO 和 `glDrawElements` 更合适；本章只有三个互不重复的顶点，直接使用 `glDrawArrays` 更简单。

## 11. `Shader` 类中的 uniform 辅助方法

类中还提供了三个方法：

```cpp
void setBool(const std::string& name, bool value) const;
void setInt(const std::string& name, int value) const;
void setFloat(const std::string& name, float value) const;
```

它们的核心形式是：

```cpp
glUniform1f(glGetUniformLocation(ID, name.c_str()), value);
```

`glGetUniformLocation` 根据变量名查询 uniform 在当前 program 中的位置，`glUniform*` 再设置对应的值。

Uniform 与顶点属性的用途不同：

| 数据类型 | 一次绘制中的变化方式 | 当前示例 |
| --- | --- | --- |
| 顶点属性 | 不同顶点可以有不同值 | 三个顶点的位置和颜色 |
| Uniform | 一次绘制调用中通常对所有顶点和片元保持一致 | 尚未使用 |

当前两个 GLSL 文件没有声明 uniform，所以这些辅助方法只是为后续章节预留。如果要使用它们，应先启用对应 program，再设置 uniform。

## 12. 编译错误与链接错误的区别

`checkCompileErrors` 根据传入类型执行两种检查。

单个 shader 的 GLSL 语法或类型有误时，查询：

```cpp
glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
glGetShaderInfoLog(shader, 1024, NULL, infoLog);
```

两个 stage 单独编译成功，但接口不匹配时，program 可能链接失败。例如 vertex shader 输出 `vec3 ourColor`，fragment shader 却声明 `in vec2 ourColor`。链接状态通过下面的 API 查询：

```cpp
glGetProgramiv(shader, GL_LINK_STATUS, &success);
glGetProgramInfoLog(shader, 1024, NULL, infoLog);
```

可将两类问题简单区分为：

- 编译检查：一个 GLSL 文件自身是否合法；
- 链接检查：多个 shader stage 能否组合成完整管线。

## 13. 资源生命周期

Program 链接成功后，两个独立 shader 对象可以删除：

```cpp
glDeleteShader(vertex);
glDeleteShader(fragment);
```

Program 已经保留链接结果，所以这不会影响之后的绘制。

退出循环后，主程序释放 VAO 和 VBO：

```cpp
glDeleteVertexArrays(1, &VAO);
glDeleteBuffers(1, &VBO);
```

当前 `Shader` 类没有析构函数，因此没有调用：

```cpp
glDeleteProgram(ID);
```

进程退出和上下文销毁后，驱动最终会回收相关资源，但一个完整的 RAII 封装通常应该在对象生命周期结束时释放 program。还要注意释放顺序：OpenGL program 应在对应上下文仍然有效时删除。

## 对象与数据关系总结

| 对象或变量 | 当前示例中的内容 | 作用 |
| --- | --- | --- |
| `vertices` | 三个顶点的位置和颜色 | CPU 侧初始数据 |
| VBO | `vertices` 的 GPU 缓冲副本 | 为顶点属性提供原始数据 |
| VAO | location 0、1 的格式及 VBO 关联 | 解释交错的顶点数据 |
| Vertex Shader | 接收 `aPos`、`aColor` | 输出位置和顶点颜色 |
| `ourColor` | vertex 到 fragment 的 stage 接口 | 在光栅化时被自动插值 |
| Fragment Shader | 接收插值颜色 | 输出 RGBA 片元颜色 |
| Shader Program | 链接后的两个 shader stage | 由 `ourShader.use()` 激活 |

## 当前代码值得注意的地方

- `Shader` 构造函数使用相对路径，程序必须在能找到 GLSL 文件的工作目录中启动；
- shader 文件读取失败或编译、链接失败后，当前实现只打印日志，仍会继续运行；
- `Shader::ID` 是公开成员，封装性较弱；
- `Shader` 类没有析构函数，不会主动删除 program；
- `setBool`、`setInt`、`setFloat` 每次都会按名称查询 uniform location，频繁更新时可以缓存 location；
- `main.cpp` 在 GLAD 初始化失败时直接返回，没有先调用 `glfwTerminate()`；
- 当前颜色属性使用浮点数，所以 `normalized` 参数设为 `GL_FALSE`；如果颜色改为字节类型，是否归一化会直接影响 shader 接收到的数值；
- 当前没有解绑 VAO 和 VBO，这在只有一套顶点状态的示例中不会影响结果，但更复杂的程序需要清楚每次状态修改作用于哪个对象。

## 本章最重要的理解

这段代码不只是“给三角形加了颜色”，而是建立了第一条自定义的跨阶段数据通道：

```text
C++ 顶点颜色
    ↓
VBO 中的原始字节
    ↓
VAO 的 location 1 描述
    ↓
Vertex Shader 的 aColor
    ↓
Vertex Shader 输出 ourColor
    ↓
光栅化阶段插值
    ↓
Fragment Shader 输入 ourColor
    ↓
FragColor
```

其中，C++ 侧的属性编号必须与 vertex shader 的 `layout location` 对应，vertex shader 输出必须与 fragment shader 输入匹配。只要这两组接口关系清楚，就能把颜色以外的法线、纹理坐标等逐顶点数据沿着同样的路径送入后续渲染阶段。
