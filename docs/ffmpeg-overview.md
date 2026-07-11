# FFmpeg 概览

FFmpeg 与仓库中的四个 GLFW/OpenGL 示例没有直接代码依赖，因此单独记录在这里。

## FFmpeg 是什么

FFmpeg 是用于处理音视频和其他多媒体数据的开源工具与库集合。常见能力包括：

- 音视频解封装与封装；
- 编码与解码；
- 格式转换；
- 缩放、裁剪、混音等滤镜处理；
- 推流、拉流和媒体信息分析。

命令行工具 `ffmpeg` 适合执行媒体转换与处理任务，`ffprobe` 用于检查媒体流和容器信息。需要把能力集成进程序时，可以使用 libavcodec、libavformat、libavfilter 等库。

## 与渲染的关系

FFmpeg 的主要职责是媒体数据处理，不是窗口或 UI 渲染。一个视频播放器中，两类组件通常会协作：

1. FFmpeg 解封装并解码压缩视频，得到像素帧；
2. OpenGL、Vulkan、Metal 或平台图形 API 把像素帧绘制并合成到屏幕。

因此可以把 FFmpeg 理解为“准备视频帧”，把图形 API 理解为“显示视频帧”。实际系统还可能通过硬件解码器和零拷贝缓冲区减少 CPU 与 GPU 之间的数据搬运。

## 参考资料

- [FFmpeg 官方网站](https://ffmpeg.org/)
- [FFmpeg GitHub 仓库](https://github.com/FFmpeg/FFmpeg)

