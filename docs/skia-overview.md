# Skia 概览

Skia 与仓库中的四个 GLFW/OpenGL 示例没有直接代码依赖，因此单独记录在这里。

## Skia 是什么

Skia 是开源的 2D 图形库，为不同硬件和软件平台提供统一的绘图 API。Chrome、ChromeOS、Android 和 Flutter 等项目都在其图形栈中使用 Skia。

应用通常通过 Canvas、Paint、Path、Image 等较高层抽象描述绘制内容，而不是直接提交底层 GPU 命令。Skia 再根据平台、设备能力和配置选择具体渲染路径。

## 渲染后端

Skia 可以使用不同后端完成实际绘制，包括：

- CPU 软件光栅化；
- OpenGL；
- Vulkan；
- Metal；
- 其他随 Skia 版本演进的 GPU 后端。

因此 Skia 和 OpenGL 不是同一层级：Skia 是较高层的 2D 图形库，OpenGL 可以是它使用的底层图形 API 之一。

## 一个简化的层级

```text
Android / Flutter / Chrome 等应用或 UI 框架
                  ↓
          Skia 等图形库
                  ↓
       OpenGL / Vulkan / Metal
                  ↓
             CPU / GPU
```

这个层级只用于建立直觉。真实系统还包含窗口系统、合成器、驱动、缓冲区队列和显示设备等组件。

## Android 中的 Skia

Skia 是 AOSP 的外部依赖之一。Android 图形栈可以借助 Skia 实现 Canvas 等 2D 绘制能力，但一次 Android 界面最终如何显示，还涉及应用进程、RenderThread、Surface、BufferQueue、SurfaceFlinger 和硬件合成等更多环节。

## 参考资料

- [Skia 官方文档](https://skia.org/docs/)
- [深入理解 Flutter 的图形图像绘制原理——图形库 Skia 剖析](https://juejin.cn/post/6914188284126035981)

