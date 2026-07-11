# render-code

`render-code` 是一个使用 C++、GLFW 和 OpenGL 编写的基础渲染示例集合。仓库中的四个独立项目按照“创建窗口 → 清理窗口 → 绘制三角形 → 用三角形组合图形”的顺序组织。

## 项目结构

| 目录 | 内容 |
| --- | --- |
| [`1-create-a-window`](1-create-a-window/) | 初始化 GLFW，创建 OpenGL 上下文和窗口，并运行最小渲染循环 |
| [`2-hello-window-clear`](2-hello-window-clear/) | 设置清屏颜色、处理键盘输入并观察渲染循环 |
| [`3-hello-triangle`](3-hello-triangle/) | 使用 Shader、VAO、VBO 和 EBO，以两个三角形索引绘制矩形 |
| [`4-circle`](4-circle/) | 使用多个三角形近似圆形和圆环 |
| [`docs`](docs/) | 与示例代码对应的说明文档及相关主题资料 |

## 环境要求

- CMake 3.25 或更高版本
- 支持 C++14 的编译器
- GLFW 3.3 或更高版本
- OpenGL
- GLAD（已放在 `third_party/glad`，由 `3-hello-triangle` 和 `4-circle` 使用）

项目当前面向 macOS。示例 3 和示例 4 使用了旧式 OpenGL 即时模式，适合观察基本绘制流程，但不代表现代 OpenGL 的推荐实现方式。

## 构建

每个示例都是独立的 CMake 项目。以第一个项目为例：

```bash
cd 1-create-a-window
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build
./build/1-create-a-window
```

其他项目可以用相同方式配置和构建。四个项目都会通过 CMake 自动查找 GLFW 和系统 OpenGL；示例 3、4 会直接编译仓库中自带的 GLAD 源码。

## 文档

- [创建窗口与 OpenGL 上下文](docs/01-create-a-window.md)
- [清屏、输入处理与渲染循环](docs/02-clear-window.md)
- [GLAD、Shader 与索引绘制](docs/03-draw-triangle.md)
- [使用三角形近似圆形和圆环](docs/04-draw-circle-and-ring.md)
- [Skia 概览](docs/skia-overview.md)
- [FFmpeg 概览](docs/ffmpeg-overview.md)
