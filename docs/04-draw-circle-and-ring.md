# 使用三角形近似圆形和圆环

对应项目：[`4-circle`](../4-circle/)

## 学习目标

OpenGL 的基本图元没有“圆”。这个项目展示如何利用三角函数计算圆周采样点，再把许多三角形组合成看起来连续的圆形或圆环。

项目提供两个绘制函数：

- `drawCircle()`：以圆心为公共顶点组成三角扇；
- `drawColorRing()`：连接内外两圈采样点组成圆环。

当前渲染循环实际调用的是 `drawColorRing()`。

## 从角度计算圆周坐标

半径为 `r`、圆心位于原点时，圆周上一点可以写成：

```text
x = cos(angle) × r
y = sin(angle) × r
```

代码把完整的一圈 `2π` 平均分成 `count` 段：

```cpp
float angle = 2 * pi / count * i;
float next_angle = 2 * pi / count * (i + 1);
```

`count` 越大，边缘越接近圆，但顶点数量和绘制工作也越多。当前值为 100，在普通窗口尺寸下已经能得到较平滑的轮廓。

最后一段中 `next_angle` 等于 `2π`，其余弦和正弦会回到接近起点的位置，因此图形能够闭合。因为浮点计算存在误差，终点不一定与起点逐位相等，但在这里不会造成可见缝隙。

## 用三角扇组成圆

每一段使用三个点：圆心、当前圆周点、下一个圆周点。

```cpp
glVertex3f(0.0f, 0.0f, 0.0f);
glVertex3f(cosf(angle) * radius, sinf(angle) * radius, 0.0f);
glVertex3f(cosf(next_angle) * radius, sinf(next_angle) * radius, 0.0f);
```

相邻三角形共享圆心和一条半径边，100 个窄三角形共同覆盖圆的内部。这种结构称为 triangle fan。

当前实现每段都单独调用一次 `glBegin`/`glEnd`。在即时模式下也可以用一次 `GL_TRIANGLE_FAN` 提交圆心和全部圆周顶点；现代 OpenGL 则通常一次生成所有顶点并通过 VBO 批量绘制。

## 用四边形条带组成圆环

圆环需要内半径和外半径：

```cpp
const float inner_radius = 0.4f;
const float outer_radius = 0.8f;
```

相邻两个角度在内外圆周上产生四个点，构成一个弯曲四边形。OpenGL 最终仍以三角形绘制，所以每一段需要拆成两个三角形。

可以把四个点记为：

```text
innerCurrent ---- outerCurrent
     |                  |
 innerNext ------- outerNext
```

一种保持一致逆时针绕序的拆分方式是：

```text
(innerCurrent, outerCurrent, outerNext)
(innerCurrent, outerNext, innerNext)
```

当前代码提交的两个三角形绕序并不一致。因为项目没有启用背面剔除，所以仍能看到完整圆环；如果以后启用 `glEnable(GL_CULL_FACE)`，其中一组三角形可能被剔除。顶点绕序是这个示例值得进一步验证的重点。

## 名称与实际颜色

函数名是 `drawColorRing()`，但当前实现没有调用 `glColor*`，因此圆环使用 OpenGL 当前保存的颜色状态。若要做彩色圆环，可以按角度生成 RGB/HSV 颜色，并在提交不同顶点前设置颜色；光栅化阶段会在顶点之间插值。

## 从即时模式过渡到现代 OpenGL

现代实现可以先在 CPU 端生成一次圆环网格：

1. 为每个角度生成内外两个顶点；
2. 生成三角形索引；
3. 把顶点与索引分别上传到 VBO 和 EBO；
4. 使用一次 `glDrawElements` 绘制整个圆环；
5. 只在半径或分段数变化时重建网格。

这样可以避免每帧重复计算三角函数，也能减少大量细碎的绘制调用。

