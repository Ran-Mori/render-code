# 清屏、输入处理与渲染循环

对应项目：[`2-hello-window-clear`](../2-hello-window-clear/)

## 学习目标

这个项目在最小窗口程序上增加了三类行为：

- 使用 `glClearColor` 设置清屏颜色；
- 每帧检测 Esc 键并关闭窗口；
- 输出循环次数和时间戳，观察渲染循环持续运行的特征。

## 清屏颜色与颜色缓冲区

```cpp
glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
glClear(GL_COLOR_BUFFER_BIT);
```

`glClearColor` 设置 OpenGL 状态中的清屏颜色，四个参数依次是红、绿、蓝和透明度，范围通常为 `[0, 1]`。这个函数只修改状态，不会立即改变窗口内容。

`glClear(GL_COLOR_BUFFER_BIT)` 才真正使用当前清屏颜色覆盖颜色缓冲区。因此调用顺序应当是先设置颜色，再执行清理。

`glClearColor` 是持续状态：如果颜色不变，它不必每一帧都设置。当前示例把它放在循环里，便于把“设置状态 → 使用状态”的关系放在一起观察。

## 输入处理

```cpp
if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
    glfwSetWindowShouldClose(window, true);
}
```

`glfwGetKey` 查询当前按键状态。按下 Esc 后，代码只设置窗口的关闭标记；渲染循环会在下一次检查 `glfwWindowShouldClose` 时退出。

输入查询依赖事件被处理。当前项目在每帧末尾调用 `glfwPollEvents()`，让 GLFW 收集操作系统事件并更新按键、窗口大小和关闭请求等状态。

## 一帧的执行流程

当前循环可以按下面的顺序理解：

1. 输出本轮循环信息；
2. 查询并处理输入；
3. 设置清屏颜色；
4. 清理后缓冲区；
5. 交换前后缓冲区；
6. 处理系统事件；
7. 开始下一轮。

这里没有主动限帧。循环速度会受垂直同步设置、显示器刷新率、日志输出和系统调度等因素影响。

## 时间戳代码的细节

函数名是 `getUnixTimeStamp`，实现实际返回的是从 Unix epoch 开始经过的毫秒数：

```cpp
std::chrono::duration_cast<std::chrono::milliseconds>(
    now.time_since_epoch()
).count();
```

代码中的中间变量声明成了 `std::chrono::microseconds`，但赋值来源是 `milliseconds`，最终数值仍然是毫秒精度。为了避免误解，更清晰的做法是让变量类型和函数命名都明确包含 `Milliseconds`。

此外，全局变量 `times` 同时用于渲染循环次数和 Esc 处理日志。它并不严格等于“已经渲染的帧数”；如果希望测量帧数，应为不同事件使用独立计数器。

## 从这个项目继续学习

- 使用 `glfwSwapInterval(1)` 观察垂直同步对循环速度的影响；
- 注册 framebuffer size 回调，在窗口变化时更新 OpenGL viewport；
- 将日志改成每秒汇总一次，避免逐帧 I/O 反过来显著影响循环性能。

