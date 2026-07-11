# OpenGL 状态机与状态泄漏实验

对应项目：[`opengl-state-machine`](../opengl-state-machine/)

交互式概念页：[`opengl-state-machine.html`](../html/opengl-state-machine.html)

## 学习目标

这个项目不是继续增加新的绘制功能，而是把前面已经使用过的 OpenGL API 重新组织成六个对照实验，重点理解：

- “OpenGL 是状态机”具体意味着什么；
- OpenGL Context 中保存了哪些当前状态；
- 为什么很多 OpenGL 函数没有直接接收对象句柄；
- 前一次绘制留下的状态怎样影响下一次绘制；
- VAO、Blend、Scissor 和 Polygon Mode 怎样产生状态泄漏；
- 为什么实验代码每帧建立已知状态；
- 真实项目中应该怎样管理状态，而不是机械地保存和恢复全部 Context；
- 怎样用断点和 `glGet*` 查询验证自己的判断。

项目固定绘制两个对象：

```text
左侧：蓝色三角形        右侧：橙色矩形

       /\                ┌─────────┐
      /  \               │         │
     /____\              └─────────┘
```

六个场景会故意改变或遗漏不同状态，让右侧矩形出现错误结果。

## 运行与交互

在仓库根目录构建：

```bash
cmake -S opengl-state-machine -B opengl-state-machine/build \
    -DCMAKE_BUILD_TYPE=Debug
cmake --build opengl-state-machine/build
```

运行：

```bash
./opengl-state-machine/build/opengl-state-machine
```

快捷键：

| 按键 | 行为 |
| --- | --- |
| `Space` | 下一个场景 |
| `Backspace` | 上一个场景 |
| `0`～`5` | 直接选择对应场景 |
| `R` | 回到场景 0 |
| `Esc` | 退出程序 |

窗口标题会显示当前场景。切换场景后，终端还会输出实际的 OpenGL 状态快照。

## 一句话理解 OpenGL 状态机

OpenGL 的行为不只取决于当前调用了哪个函数，还取决于此前在当前 OpenGL Context 中留下了什么状态。

例如：

```cpp
glBindVertexArray(triangleVao);
glDrawArrays(GL_TRIANGLES, 0, 6);
```

`glDrawArrays` 没有接收 `triangleVao` 参数。它会读取 Context 中的 Current VAO，也就是前一行绑定的 `triangleVao`。

如果后面没有绑定另一个 VAO：

```cpp
// glBindVertexArray(rectangleVao); 被遗漏
glDrawArrays(GL_TRIANGLES, 0, 6);
```

第二次绘制仍然使用三角形 VAO。

可以把 Context 简化理解为一组持续存在的当前值：

```text
OpenGL Context
├─ Current Program
├─ Current VAO
├─ Current Buffer bindings
├─ Active Texture Unit
├─ 每个 Texture Unit 绑定的 Texture
├─ Current Framebuffer
├─ Viewport
├─ Blend enabled / disabled
├─ Scissor enabled / disabled
├─ Polygon Mode
└─ 许多其他状态
```

OpenGL 调用通常属于三类动作：

```text
Bind       选择当前对象
Configure  修改当前状态或当前对象
Consume    使用当前状态执行清屏或绘制
```

例如：

| 类型 | 示例 | 含义 |
| --- | --- | --- |
| Bind | `glBindVertexArray(vao)` | 修改 Current VAO |
| Bind | `glUseProgram(program)` | 修改 Current Program |
| Configure | `glEnable(GL_BLEND)` | 修改 Blend 开关 |
| Configure | `glPolygonMode(...)` | 修改光栅化模式 |
| Consume | `glDrawArrays(...)` | 使用当前状态执行绘制 |
| Consume | `glClear(...)` | 使用当前 clear state 清理缓冲区 |

## 项目结构与职责

项目拆成三个主要模块：

```text
main.cpp
  └─ 创建 Application 并处理顶层异常

Application
  ├─ GLFW 生命周期
  ├─ Window 与 OpenGL Context
  ├─ GLAD 初始化
  ├─ 键盘与 framebuffer callback
  └─ 渲染循环

StateMachineLab
  ├─ Shader 与 Uniform
  ├─ 三角形和矩形 Geometry
  ├─ 当前实验场景
  ├─ 六个场景的渲染逻辑
  └─ OpenGL 状态查询与日志

Geometry
  ├─ VAO
  ├─ VBO
  ├─ 顶点数量
  └─ 绑定、绘制与资源释放
```

这种拆分体现的是职责聚合，而不是单纯追求更多文件：

- `Application` 不知道每个场景怎样设置 OpenGL 状态；
- `StateMachineLab` 不负责创建和销毁 GLFW Window；
- `Geometry` 不知道对象位于屏幕左侧还是右侧；
- `main.cpp` 不包含实验细节。

## 1. `Application`：Context 与生命周期所有者

程序入口只有：

```cpp
int main()
{
    try
    {
        Application application;
        return application.run();
    }
    catch (const std::exception& error)
    {
        std::cerr << "Fatal error: " << error.what() << std::endl;
        return -1;
    }
}
```

`Application::initializeWindow()` 依次完成：

```text
glfwInit
    ↓
设置 OpenGL 3.3 Core Profile hint
    ↓
glfwCreateWindow：创建 Window 和 Context
    ↓
glfwMakeContextCurrent：让 Context 成为当前 Context
    ↓
gladLoadGLLoader：加载 OpenGL 函数
    ↓
创建 StateMachineLab 中的 OpenGL 对象
```

顺序不能随意变化。`Geometry` 构造函数会调用 `glGenVertexArrays`、`glGenBuffers`，`Shader` 构造函数会调用 `glCreateShader`。这些操作必须发生在 Context 创建、激活并且 GLAD 初始化之后。

### 为什么退出时先销毁 `StateMachineLab`

事件循环结束后：

```cpp
lab_.reset();
```

之后 `Application` 才销毁 Window 并调用 `glfwTerminate()`。

这是因为 `StateMachineLab` 析构时还会继续调用：

```cpp
glDeleteVertexArrays(...);
glDeleteBuffers(...);
glDeleteProgram(...);
```

OpenGL 资源属于 Context，必须在 Context 仍然有效且当前时释放。

正确顺序：

```text
销毁 Geometry / Shader
        ↓
销毁 Window / Context
        ↓
glfwTerminate
```

## 2. GLFW Callback 怎样回到 `Application` 对象

GLFW callback 是普通函数指针，不能直接注册非静态成员函数。项目先将 `this` 存入 Window：

```cpp
glfwSetWindowUserPointer(window_, this);
```

静态 callback 再取回对象：

```cpp
Application* application =
    static_cast<Application*>(glfwGetWindowUserPointer(window));

application->handleKey(key, action);
```

因此数据流是：

```text
操作系统键盘事件
    ↓
GLFW keyCallback
    ↓ window user pointer
Application::handleKey
    ↓
StateMachineLab::selectScenario
```

`Application` 负责把输入映射成实验操作，而 `StateMachineLab` 不依赖 GLFW 的按键常量。

## 3. `Geometry` 封装了什么

`Geometry` 构造时创建 VAO 和 VBO：

```cpp
glGenVertexArrays(1, &vao_);
glGenBuffers(1, &vbo_);

glBindVertexArray(vao_);
glBindBuffer(GL_ARRAY_BUFFER, vbo_);
glBufferData(...);
glVertexAttribPointer(...);
glEnableVertexAttribArray(0);
```

对象内部保存：

```text
vao_
vbo_
vertexCount_
```

析构时自动释放：

```cpp
Geometry::~Geometry()
{
    glDeleteVertexArrays(1, &vao_);
    glDeleteBuffers(1, &vbo_);
}
```

这是一种 RAII：C++ 对象生命周期决定 OpenGL 资源生命周期。

### `bind()`、`drawBound()` 与 `draw()` 的区别

正常绘制：

```cpp
void Geometry::draw() const
{
    bind();
    drawBound();
}
```

即：

```text
绑定自己的 VAO
      ↓
使用当前 VAO 发出 Draw Call
```

`drawBound()` 故意只发出绘制命令：

```cpp
void Geometry::drawBound() const
{
    glDrawArrays(GL_TRIANGLES, 0, vertexCount_);
}
```

它不会绑定自己的 VAO，因此会使用 Context 中此前留下的 Current VAO。

在普通业务代码中，这种接口需要谨慎使用；在本实验中，它专门用来演示状态泄漏。

## 4. 为什么两个 Geometry 都有六个顶点

矩形由两个三角形构成，因此有六个顶点：

```text
三角形 1：左下、右下、右上
三角形 2：右上、左上、左下
```

三角形 Geometry 也准备了六个顶点：

```cpp
const float kTriangleVertices[] = {
    -0.30f, -0.30f,
     0.30f, -0.30f,
     0.00f,  0.35f,

     0.00f,  0.00f,
     0.00f,  0.00f,
     0.00f,  0.00f
};
```

前三个顶点组成可见三角形，后三个完全重合，组成退化三角形，不产生可见片元。

这样做是为了让 VAO 泄漏场景保持内存安全：矩形的 `drawBound()` 会请求六个顶点，即使当前错误绑定的是三角形 VAO，三角形 VBO 也确实包含六个顶点，不会越界读取。

## 5. Shader 与 Uniform 也是状态

Vertex Shader 使用：

```glsl
uniform vec2 uOffset;
uniform float uScale;
```

Fragment Shader 使用：

```glsl
uniform vec4 uColor;
```

构造 `StateMachineLab` 时只查询一次 uniform location：

```cpp
uniforms_ = {
    glGetUniformLocation(shader_->id(), "uOffset"),
    glGetUniformLocation(shader_->id(), "uScale"),
    glGetUniformLocation(shader_->id(), "uColor")
};
```

绘制每个对象前设置：

```cpp
glUniform2f(uniforms_.offset, offsetX, offsetY);
glUniform1f(uniforms_.scale, scale);
glUniform4f(uniforms_.color, red, green, blue, alpha);
```

Uniform 值属于 Program Object 的状态。调用 `glUniform*` 时，修改的是当前 Program 中对应 location 的值。

项目每帧先执行：

```cpp
shader_->use();
```

保证后续 `glUniform*` 和 Draw Call 使用正确的 Current Program。

## 6. 每帧建立已知状态

每帧开始时调用：

```cpp
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
```

它的目的不是模拟 OpenGL 自动重置，而是给实验建立确定的起点。

如果不建立基线：

```text
上一帧 Polygon Mode 场景留下 GL_LINE
                   ↓
切换到 VAO 场景
                   ↓
VAO 场景也意外保持线框
                   ↓
无法判断画面变化究竟来自 VAO 还是 Polygon Mode
```

有了已知基线，每个场景只故意泄漏一个状态，画面结果就能与唯一原因对应。

需要特别注意：

> `establishKnownFrameState()` 是实验隔离策略，不是每个真实组件都必须复制的最佳实践。

真实渲染器通常会缓存当前状态、按 Render Pass 或 Material 建立状态，并减少重复切换。

## 7. 场景 0：正确基线

按键：`0`

主要流程：

```cpp
drawLeftTriangle();

glDisable(GL_BLEND);
glDisable(GL_SCISSOR_TEST);
glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

drawRightRectangle();
```

两个对象都会绑定自己的 VAO：

```cpp
triangle_->draw();
rectangle_->draw();
```

预期结果：

```text
蓝色填充三角形 + 橙色填充矩形
```

对象 B 绘制完成后的状态应类似：

```text
GL_VERTEX_ARRAY_BINDING = rectangle VAO
GL_BLEND                = Disabled
GL_SCISSOR_TEST         = Disabled
GL_POLYGON_MODE         = GL_FILL
```

## 8. 场景 1：VAO 状态泄漏

按键：`1`

代码：

```cpp
drawLeftTriangle();
drawRightRectangle(1.0f, false);
```

`drawLeftTriangle()` 内部执行：

```cpp
triangle_->draw();
```

所以 Current VAO 变成三角形 VAO。

第二次绘制传入：

```cpp
bindOwnVao = false
```

于是调用：

```cpp
rectangle_->drawBound();
```

这里只执行 `glDrawArrays`，没有绑定矩形 VAO。虽然调用发生在 `rectangle_` 对象上，但 OpenGL 不知道 C++ 对象语义；它只读取当前 Context，所以右侧仍然使用三角形顶点。

预期结果：

```text
左侧蓝色三角形 + 右侧橙色三角形
```

这个场景说明：

> C++ 正在执行哪个对象的方法，不会自动改变 OpenGL Current VAO。

## 9. 默认参数为什么会出现在调试器中

声明：

```cpp
void drawRightRectangle(
    float alpha = 1.0f,
    bool bindOwnVao = true
) const;
```

调用：

```cpp
drawRightRectangle();
```

编译器会按默认参数处理为：

```cpp
drawRightRectangle(1.0f, true);
```

所以在断点处能够看到：

```text
alpha = 1
bindOwnVao = true
```

另外：

```cpp
drawRightRectangle(0.32f);
```

等价于：

```cpp
drawRightRectangle(0.32f, true);
```

VAO 泄漏场景则显式覆盖第二个参数：

```cpp
drawRightRectangle(1.0f, false);
```

## 10. 场景 2：Blend 状态泄漏

按键：`2`

代码：

```cpp
glEnable(GL_BLEND);
drawLeftTriangle(0.48f);

// 没有 glDisable(GL_BLEND)
drawRightRectangle(0.32f);
```

Blend 是 Context 中持续存在的开关。第一次 Draw Call 结束不会自动关闭。

Fragment Shader 输出：

```glsl
FragColor = uColor;
```

右侧矩形的 Alpha 是 `0.32`。启用标准 Alpha Blend 后，颜色大致按下面的方式组合：

```text
result = source × sourceAlpha
       + destination × (1 - sourceAlpha)
```

所以橙色矩形会与深色背景混合，看起来更暗、更透明。

如果 Blend 关闭，Alpha 分量不会自动让 RGB 变透明；Fragment Shader 输出的 RGB 会直接写入颜色缓冲区。

预期状态：

```text
GL_BLEND = Enabled
```

## 11. 场景 3：Scissor 状态泄漏

按键：`3`

代码：

```cpp
glEnable(GL_SCISSOR_TEST);
glScissor(0, 0, framebufferWidth_ / 2, framebufferHeight_);

drawLeftTriangle();

// 没有 glDisable(GL_SCISSOR_TEST)
drawRightRectangle();
```

Scissor Box 只覆盖 framebuffer 左半部分。右侧矩形位于 Scissor Box 外，因此它产生的片元会被拒绝。

预期结果：

```text
左侧三角形可见，右侧矩形消失
```

`glScissor` 使用 framebuffer 像素坐标，不是窗口逻辑尺寸，也不是 NDC 坐标。

Retina 屏幕上，一个 `960 × 640` 的窗口可能对应 `1920 × 1280` 的 framebuffer。因此项目保存的是 framebuffer callback 提供的尺寸：

```cpp
void StateMachineLab::resize(int width, int height)
{
    framebufferWidth_ = width;
    framebufferHeight_ = height;
}
```

## 12. 场景 4：Polygon Mode 状态泄漏

按键：`4`

代码：

```cpp
glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
drawLeftTriangle();

// 没有恢复 GL_FILL
drawRightRectangle();
```

`glPolygonMode` 设置的是 Context 光栅化状态，不属于某个 VAO，也不属于某个 Shader。

所以第二次 Draw Call 继续使用 `GL_LINE`，矩形只绘制边线以及组成矩形的三角形边。

预期结果：

```text
线框三角形 + 线框矩形
```

预期状态：

```text
GL_POLYGON_MODE = GL_LINE
```

## 13. 场景 5：显式恢复状态边界

按键：`5`

对象 A 使用特殊状态：

```cpp
glEnable(GL_BLEND);
glEnable(GL_SCISSOR_TEST);
glScissor(0, 0, framebufferWidth_ / 2, framebufferHeight_);
glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
drawLeftTriangle(0.60f);
```

绘制对象 B 之前显式建立它需要的状态：

```cpp
glDisable(GL_BLEND);
glDisable(GL_SCISSOR_TEST);
glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
drawRightRectangle();
```

预期结果：

```text
左侧对象使用特殊状态
右侧矩形仍然完整、不透明并采用填充模式
```

这个场景最重要的不是“必须恢复所有状态”，而是：

> 绘制对象 B 之前，必须确保 B 依赖的状态已经建立。

## 14. 怎样查询真实状态

项目在场景切换后调用：

```cpp
glGetIntegerv(GL_CURRENT_PROGRAM, &currentProgram);
glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &currentVao);
glGetIntegerv(GL_POLYGON_MODE, polygonMode);
glGetIntegerv(GL_VIEWPORT, viewport);

glIsEnabled(GL_BLEND);
glIsEnabled(GL_SCISSOR_TEST);
```

它们用于验证：

```text
我认为当前 VAO 是什么？
        ↓
glGetIntegerv 得到的实际值是什么？
        ↓
两者是否一致？
```

这非常适合学习和调试，但不适合在正式渲染循环中高频调用。状态查询可能需要驱动同步，并产生不必要的开销。

正式 Renderer 通常自己缓存最后设置的状态：

```cpp
if (cachedVao != requestedVao)
{
    glBindVertexArray(requestedVao);
    cachedVao = requestedVao;
}
```

应用通过自己发出的命令推断当前状态，而不是每次向 OpenGL 查询。

## 15. “保存、修改、绘制、恢复”是不是最佳实践

一种直观方案是：

```text
查询并保存原状态
    ↓
修改状态
    ↓
绘制组件
    ↓
恢复原状态
```

它在局部隔离、第三方渲染或临时插入绘制时有用，但通常不应该成为每个对象的固定流程。

如果每个对象都执行：

```cpp
glGetIntegerv(...);
glIsEnabled(...);
glBind...;
glEnable...;
glDraw...;
glBind previous...;
glDisable...;
```

会产生：

- 大量状态查询；
- 大量重复状态切换；
- CPU 与驱动的额外交互；
- 状态责任分散到每个业务组件；
- 很难按材质或 Render Pass 对 Draw Call 排序。

更常见的组织方式是：

```text
Renderer 建立不透明阶段状态
  ├─ 绘制不透明对象 A
  ├─ 绘制不透明对象 B
  └─ 绘制不透明对象 C

Renderer 建立透明阶段状态
  ├─ 绘制透明对象 A
  └─ 绘制透明对象 B
```

或者每个 Draw Command 明确描述自己需要的状态：

```text
RenderCommand
├─ Geometry / VAO
├─ Material / Program
├─ Texture bindings
├─ Transform
├─ Blend Mode
├─ Depth State
└─ Scissor State
```

Renderer 统一决定哪些状态需要实际改变。

因此更准确的最佳实践是：

> 明确状态所有权，在 Draw Call 前建立所需状态，由 Renderer 缓存并减少状态切换；只在确实需要局部隔离时保存和恢复。

## 16. Context 状态与对象内部状态

“OpenGL 状态”并不全部存在于同一个平面。需要区分 Context 当前绑定和对象内部保存的数据。

### Context 当前状态

例如：

- Current Program；
- Current VAO；
- Blend 是否启用；
- Scissor 是否启用；
- Polygon Mode；
- Viewport。

### VAO 内部状态

例如：

- 每个 attribute 是否启用；
- attribute 的类型、stride 和 offset；
- attribute 从哪个 VBO 读取；
- VAO 绑定的 EBO。

### Texture Object 内部状态

例如：

- 像素数据和 mipmap；
- wrapping；
- filtering。

### Program Object 内部状态

例如：

- 链接后的 Shader；
- uniform 值。

`glBind*` 通常是在 Context 中选择当前对象；后续配置调用则可能把状态写入这个对象内部。

## 17. 推荐的调试路径

项目已配置 VS Code LLDB。推荐先在下面的位置设置断点。

### 观察按键怎样切换场景

```cpp
Application::handleKey
```

关注：

```text
key
action
lab_->scenarioIndex()
```

### 观察场景分支

```cpp
StateMachineLab::renderCurrentScenario
```

按 `0`～`5` 切换，观察 `scenario_` 进入哪个 `case`。

### 观察默认参数

```cpp
StateMachineLab::drawRightRectangle
```

关注：

```text
alpha
bindOwnVao
```

### 观察 VAO 泄漏

在下面两处下断点：

```cpp
Geometry::bind
Geometry::drawBound
```

场景 1 中，右侧矩形会进入 `drawBound()`，但不会先进入矩形的 `bind()`。

### 观察状态快照

```cpp
StateMachineLab::printStateSnapshot
```

可以单步执行每次 `glGetIntegerv`，比较自己预测的状态和真实返回值。

## 18. RAII 与低耦合设计

当前项目中：

- `Application` 析构时清理 Lab、Window 和 GLFW；
- `StateMachineLab` 通过 `unique_ptr` 独占 Shader 和 Geometry；
- `Geometry` 析构时删除 VAO 和 VBO；
- `Shader` 析构时删除 Program；
- Shader 文件读取、编译或链接失败会抛出异常；
- `main` 统一捕获异常并输出错误。

`StateMachineLab` 头文件只前向声明：

```cpp
class Geometry;
class Shader;
```

具体 OpenGL 实现头文件只在 `.cpp` 中包含。这减少了头文件依赖，也避免修改 Geometry 实现时触发所有包含者重新编译。

## 关键状态关系总结

| 状态 | 谁修改 | 谁消费 | 泄漏后的现象 |
| --- | --- | --- | --- |
| Current Program | `glUseProgram` | `glUniform*`、Draw Call | 使用错误 Shader 或 Uniform |
| Current VAO | `glBindVertexArray` | `glDrawArrays` | 使用错误几何布局与 VBO |
| Blend Enable | `glEnable/glDisable(GL_BLEND)` | 片元合并阶段 | 后续对象意外透明 |
| Scissor Enable | `glEnable/glDisable(GL_SCISSOR_TEST)` | 片元测试阶段 | 后续对象被裁剪 |
| Scissor Box | `glScissor` | 片元测试阶段 | 只保留指定像素区域 |
| Polygon Mode | `glPolygonMode` | 光栅化阶段 | 后续对象意外变成线框 |
| Viewport | `glViewport` | NDC 到 framebuffer 映射 | 图形尺寸或位置错误 |
| Uniform | `glUniform*` | Shader | 后续对象使用上一对象参数 |

## 常见误区

### 误区 1：每个 Draw Call 结束后状态会恢复

不会。OpenGL 状态会持续，直到应用显式修改，或切换、销毁对应 Context。

### 误区 2：调用 `rectangle_->drawBound()` 就会使用矩形 VAO

不会。C++ 对象方法和 OpenGL Context 没有自动映射。`drawBound()` 只调用 `glDrawArrays`，使用的是 Current VAO。

### 误区 3：VAO 保存全部顶点数据

不会。顶点字节位于 VBO。VAO 保存顶点输入配置、VBO 关联和 EBO 绑定。

### 误区 4：Alpha 小于 1 就一定透明

不一定。只有启用 Blend 并设置适当混合函数后，Alpha 才会参与 framebuffer 合成。

### 误区 5：所有状态都应该在组件绘制后恢复

不一定。更重要的是明确状态所有者，并在下一次绘制前建立所需状态。

### 误区 6：`glGet*` 是正式渲染时管理状态的主要方式

不推荐。它适合验证和调试；正式 Renderer 更常自己缓存状态。

## 一帧的执行流程

```text
Application 渲染循环
        ↓
StateMachineLab::render
        ↓
establishKnownFrameState
  ├─ Current Program = 实验 Shader
  ├─ Blend = Disabled
  ├─ Scissor = Disabled
  ├─ Polygon Mode = Fill
  └─ Viewport = framebuffer 尺寸
        ↓
glClear
        ↓
renderCurrentScenario
  ├─ 绘制左侧对象
  ├─ 故意保留或恢复某项状态
  └─ 绘制右侧对象
        ↓
必要时查询并打印状态快照
        ↓
glfwSwapBuffers
        ↓
glfwPollEvents
```

## 本项目最重要的理解

OpenGL 状态机可以归纳为：

```text
先前命令修改 Context 或当前对象状态
              ↓
状态持续存在
              ↓
后续命令读取当前状态
              ↓
产生绘制结果
```

对于实际项目，更进一步的理解是：

```text
不是每个组件都保存和恢复整个 Context
                  ↓
Renderer 统一拥有和缓存状态
                  ↓
每个 Render Pass / Draw Command 描述所需状态
                  ↓
Renderer 只执行必要的状态切换
```

当前实验故意让状态泄漏，是为了让隐藏依赖变得可见。真正的渲染架构则应让依赖明确、状态所有权清晰，并让每次 Draw Call 的前置条件可以被预测。

## 从这个项目继续学习

- 给 Renderer 增加 `RenderStateCache`，避免重复调用 `glBindVertexArray` 和 `glEnable/glDisable`；
- 增加 Current Program 泄漏场景，观察使用错误 Shader 的结果；
- 增加 Texture Unit 泄漏场景，观察 Sampler 读取错误纹理；
- 增加 Depth Test 场景，理解深度缓冲和绘制顺序；
- 增加 Framebuffer 场景，理解 Draw Call 实际写入哪个目标；
- 使用 RenderDoc 检查某次 Draw Call 的完整 Pipeline State；
- 将对象提交为 Render Command，再按 Program、Material 或 Blend Mode 排序。
