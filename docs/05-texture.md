# Texture Object、纹理坐标与采样

对应项目：[`5-texture`](../5-texture/)

## 学习目标

这个项目在 Shader、VAO、VBO 和 EBO 的基础上，第一次把一张磁盘图片送入 OpenGL 渲染管线，并将它贴到矩形表面。

本章重点理解：

- JPEG 文件怎样被 `stb_image` 解码为 CPU 内存中的像素；
- CPU 像素怎样上传到 OpenGL Texture Object；
- Texture Object、Texture Unit 和 GLSL Sampler 分别负责什么；
- 顶点纹理坐标怎样通过 VAO 和 Vertex Shader 进入光栅化阶段；
- Fragment Shader 怎样根据插值后的纹理坐标采样图片；
- wrapping、filtering 和 mipmap 怎样影响采样结果；
- 文件路径、图片通道数和图片坐标方向有哪些容易忽略的问题。

当前程序最终显示的是一个矩形，矩形表面覆盖 LearnOpenGL 的 `container.jpg` 图片。

## 从 JPEG 文件到屏幕像素的完整路径

这个项目的数据流可以分为“纹理创建”和“逐帧采样”两部分。

纹理创建发生在进入渲染循环之前：

```text
container.jpg
      ↓ FileSystem::getPath 定位文件
stbi_load 解码 JPEG
      ↓
CPU 内存中的 RGB 像素数组
      ↓ glTexImage2D
OpenGL Texture Object
      ↓ glGenerateMipmap
原始纹理 + 多级缩小纹理
```

逐帧绘制时：

```text
VBO 中的纹理坐标
      ↓ VAO 的 location 2 配置
Vertex Shader：aTexCoord
      ↓ 输出 TexCoord
光栅化阶段对 TexCoord 插值
      ↓
Fragment Shader：TexCoord
      ↓ texture(texture1, TexCoord)
Sampler 通过 Texture Unit 找到 Texture Object
      ↓
获得纹理颜色并写入 FragColor
```

必须区分这两个阶段：`glTexImage2D` 负责创建和上传纹理数据，`texture()` 负责绘制时根据坐标读取纹理。

## 1. 图片文件不是 OpenGL 纹理

磁盘上的 [`container.jpg`](../5-texture/resources/textures/container.jpg) 是经过 JPEG 压缩的文件。它不能直接作为 GPU 采样的数据。

JPEG 文件中除了压缩后的图像内容，还可能包含尺寸、颜色配置和 EXIF 等信息。OpenGL 的 `glTexImage2D` 不负责解析 JPEG、PNG 等文件格式，它只接收已经解码好的像素字节。

因此程序先调用：

```cpp
unsigned char* data = stbi_load(
    FileSystem::getPath("resources/textures/container.jpg").c_str(),
    &width,
    &height,
    &nrChannels,
    0
);
```

`stbi_load` 完成文件读取和 JPEG 解码。成功时返回一块动态分配的 CPU 内存，并填写：

| 输出 | 含义 |
| --- | --- |
| `data` | 指向解码后像素数组的指针 |
| `width` | 图片宽度，单位为像素 |
| `height` | 图片高度，单位为像素 |
| `nrChannels` | 原图片的通道数 |

最后一个参数为 `0`，表示不强制转换通道数，保留图片原本的通道数量。

当前 `container.jpg` 是 RGB JPEG，因此每个像素由三个连续的 `unsigned char` 表示：

```text
像素 0：[R G B]
像素 1：[R G B]
像素 2：[R G B]
...
```

每个通道占一个字节，值域是 `[0, 255]`。

## 2. `stb_image` 的声明与实现

主程序包含：

```cpp
#include <stb_image.h>
```

这个头文件提供 `stbi_load`、`stbi_image_free` 等函数的声明。

项目中还有一个独立的 [`stb_image.cpp`](../5-texture/stb_image.cpp)：

```cpp
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
```

`stb_image.h` 是单头文件库。没有定义 `STB_IMAGE_IMPLEMENTATION` 时，通常只提供声明；定义它以后再包含头文件，会生成函数实现。

这个宏只能在整个可执行程序的一个编译单元中定义一次。如果在多个 `.cpp` 文件中都写：

```cpp
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
```

链接时就可能出现 `stbi_load` 等符号被重复定义的问题。

当前项目将实现单独放在 `stb_image.cpp` 中，然后由 CMake 一起编译：

```cmake
add_executable(5-texture main.cpp stb_image.cpp)
```

这种组织方式让 `main.cpp` 只需要正常包含头文件，不需要关心实现宏。

## 3. 资源路径怎样被定位

当前纹理路径通过：

```cpp
FileSystem::getPath("resources/textures/container.jpg")
```

获得，而不是直接使用相对路径。

这是因为普通相对路径相对于程序启动时的当前工作目录，并不相对于 `main.cpp` 或可执行文件。

例如从不同目录启动：

```bash
cd 5-texture
./build/5-texture
```

和：

```bash
./5-texture/build/5-texture
```

两次启动的当前工作目录不同。如果直接写 `"resources/textures/container.jpg"`，第二种运行方式会在仓库根目录下寻找 `resources`，导致加载失败。

当前 CMake 使用模板生成 `root_directory.h`：

```cmake
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/learnopengl/root_directory.h.in"
    "${GENERATED_INCLUDE_DIR}/root_directory.h"
    @ONLY
)
```

模板中的：

```cpp
const char* logl_root = "@CMAKE_CURRENT_SOURCE_DIR@";
```

会在 CMake 配置阶段替换成 `5-texture` 的绝对路径。`FileSystem::getPath()` 再把资源相对路径拼接到项目根目录之后。

两个 Shader 文件也采用相同方式定位：

```cpp
const std::string vertexShaderPath =
    FileSystem::getPath("4.1.texture.vs");
const std::string fragmentShaderPath =
    FileSystem::getPath("4.1.texture.fs");
```

所以当前项目无论从 `5-texture` 目录还是仓库根目录启动，都能定位 Shader 和纹理文件。

## 4. 创建 Texture Object

纹理对象的创建方式与 VBO、EBO 类似：

```cpp
unsigned int texture;
glGenTextures(1, &texture);
```

`glGenTextures` 生成一个纹理对象名称，也就是 OpenGL 用于标识对象的整数句柄。此时它还没有图片像素和完整的采样参数。

接着执行：

```cpp
glBindTexture(GL_TEXTURE_2D, texture);
```

这表示把 `texture` 绑定到当前活动 Texture Unit 的 `GL_TEXTURE_2D` 目标。

OpenGL 是状态机。绑定完成后，后续以 `GL_TEXTURE_2D` 为目标的调用都会作用于当前绑定的这个纹理对象，例如：

```cpp
glTexParameteri(GL_TEXTURE_2D, ...);
glTexImage2D(GL_TEXTURE_2D, ...);
glGenerateMipmap(GL_TEXTURE_2D);
```

这些 API 没有直接接收变量 `texture`，它们通过当前绑定状态找到要修改的对象。

可以把调用关系理解为：

```text
glBindTexture(GL_TEXTURE_2D, texture)
                 ↓
当前 Texture Unit 的 2D 纹理 = texture
                 ↓
后续 GL_TEXTURE_2D 操作都作用于 texture
```

## 5. Texture Object 与 Texture Unit 不是同一个概念

Texture Object 保存纹理相关的数据和参数，例如：

- 图片像素及 mipmap；
- 宽度、高度和格式；
- wrapping 参数；
- filtering 参数。

Texture Unit 是 Shader 访问纹理时使用的绑定槽位。

可以把两者类比为：

```text
Texture Object：真正的纹理资源
Texture Unit：把某个纹理资源接到 Shader 输入上的插槽
```

OpenGL 提供多个 Texture Unit：

```text
Texture Unit 0 → Texture Object A
Texture Unit 1 → Texture Object B
Texture Unit 2 → Texture Object C
```

当前代码没有调用：

```cpp
glActiveTexture(GL_TEXTURE0);
```

OpenGL 初始活动 Texture Unit 是 0，因此当前的 `glBindTexture` 实际把纹理绑定到了 Texture Unit 0。

Fragment Shader 中：

```glsl
uniform sampler2D texture1;
```

Sampler uniform 保存的并不是 Texture Object 句柄，而是 Texture Unit 的编号。当前代码也没有显式设置 `texture1`，它依赖 sampler uniform 的初始值 `0`，因此会从 Texture Unit 0 采样。

对于学习单纹理流程，这可以正常工作；更清晰的代码通常会显式写出：

```cpp
ourShader.use();
ourShader.setInt("texture1", 0);
```

它表达的是：

```text
texture1 sampler → Texture Unit 0 → 当前绑定的 Texture Object
```

## 6. 设置纹理环绕方式

当前代码设置：

```cpp
glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
```

二维纹理坐标通常写作 `(s, t)`，也常写作 `(u, v)`：

- `S` 或 `U`：水平方向；
- `T` 或 `V`：垂直方向。

纹理坐标的标准范围是 `[0, 1]`：

```text
(0,1) ---------------- (1,1)
  |                       |
  |        texture        |
  |                       |
(0,0) ---------------- (1,0)
```

Wrapping 决定纹理坐标超出 `[0,1]` 时怎样处理。

`GL_REPEAT` 会只保留坐标的小数部分，让纹理重复：

```text
s = 0.25 → 采样 0.25
s = 1.25 → 再次采样 0.25
s = 2.25 → 再次采样 0.25
```

当前顶点纹理坐标都处于 `[0,1]`，所以暂时看不到 `GL_REPEAT` 的重复效果。把右侧坐标从 `1.0` 改为 `2.0`，就能看到水平方向出现两份纹理。

常见 wrapping 方式还有：

| 参数 | 超出范围时的行为 |
| --- | --- |
| `GL_REPEAT` | 重复纹理 |
| `GL_MIRRORED_REPEAT` | 镜像重复纹理 |
| `GL_CLAMP_TO_EDGE` | 使用最接近的边缘纹素 |
| `GL_CLAMP_TO_BORDER` | 使用指定的边界颜色 |

## 7. 设置纹理过滤方式

屏幕片元和纹理中的纹素（texel）通常不是一一对应的。

当纹理被放大时，一个纹素可能覆盖多个屏幕片元；当纹理被缩小时，一个屏幕片元可能对应纹理中的许多纹素。因此 OpenGL 需要决定怎样计算采样颜色。

当前代码设置：

```cpp
glTexParameteri(
    GL_TEXTURE_2D,
    GL_TEXTURE_MIN_FILTER,
    GL_LINEAR_MIPMAP_LINEAR
);

glTexParameteri(
    GL_TEXTURE_2D,
    GL_TEXTURE_MAG_FILTER,
    GL_LINEAR
);
```

### 放大过滤：`GL_LINEAR`

`GL_TEXTURE_MAG_FILTER` 在纹理被放大时使用。

`GL_LINEAR` 会根据采样位置附近的四个纹素进行双线性插值，产生平滑结果。

与之相对的是 `GL_NEAREST`，它直接选择最近的一个纹素，结果更锐利，也更容易看到块状像素。

### 缩小过滤：`GL_LINEAR_MIPMAP_LINEAR`

`GL_TEXTURE_MIN_FILTER` 在纹理被缩小时使用。

`GL_LINEAR_MIPMAP_LINEAR` 通常称为三线性过滤。它会：

1. 选择最接近目标尺寸的两个 mipmap level；
2. 在每个 level 内分别进行线性采样；
3. 再在两个 level 的采样结果之间插值。

这能减少远处或缩小纹理的闪烁和锯齿，但前提是纹理对象具有完整 mipmap。

## 8. `glTexImage2D` 上传了什么

图片成功解码后执行：

```cpp
glTexImage2D(
    GL_TEXTURE_2D,
    0,
    GL_RGB,
    width,
    height,
    0,
    GL_RGB,
    GL_UNSIGNED_BYTE,
    data
);
```

参数含义如下：

| 参数 | 当前值 | 含义 |
| --- | --- | --- |
| target | `GL_TEXTURE_2D` | 修改当前绑定的二维纹理 |
| level | `0` | 上传基础 mipmap level |
| internal format | `GL_RGB` | OpenGL 内部保存为 RGB |
| width | `width` | 图片宽度 |
| height | `height` | 图片高度 |
| border | `0` | Core Profile 中必须为 0 |
| source format | `GL_RGB` | `data` 中每个像素按 RGB 排列 |
| source type | `GL_UNSIGNED_BYTE` | 每个通道是一个无符号字节 |
| pixels | `data` | CPU 像素数组的起始地址 |

这个调用将 CPU 内存中的像素数据交给 OpenGL。调用返回后，纹理对象已经拥有自己的纹理存储，不再依赖 `data`。

因此随后可以释放 `stb_image` 分配的 CPU 内存：

```cpp
stbi_image_free(data);
```

不能使用普通 `delete[] data` 代替，因为这块内存应由 `stb_image` 配套的释放函数管理。

## 9. 为什么不能对所有图片固定使用 `GL_RGB`

当前 `container.jpg` 是三通道图片，所以 source format 使用 `GL_RGB`。

如果换成带 Alpha 的四通道 PNG，`stbi_load` 返回的数据布局通常是：

```text
R G B A | R G B A | ...
```

此时仍告诉 OpenGL 数据是 `GL_RGB`，OpenGL 就会以每三个字节为一个像素解释四通道数据，导致颜色错乱和行数据错位。

更通用的代码需要根据 `nrChannels` 选择格式，例如：

```cpp
GLenum format = 0;
if (nrChannels == 1)
    format = GL_RED;
else if (nrChannels == 3)
    format = GL_RGB;
else if (nrChannels == 4)
    format = GL_RGBA;
```

然后将对应格式传给 `glTexImage2D`。

还要注意像素行对齐。OpenGL 默认的 `GL_UNPACK_ALIGNMENT` 是 4。如果 RGB 图片每行字节数不是 4 的倍数，可能需要在上传前调用：

```cpp
glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
```

当前 512 像素宽的 RGB 图片每行有 `512 × 3 = 1536` 字节，是 4 的倍数，因此不会触发这个问题。

## 10. 生成 Mipmap

基础纹理上传后执行：

```cpp
glGenerateMipmap(GL_TEXTURE_2D);
```

OpenGL 会根据 level 0 自动生成一系列逐级缩小的纹理：

```text
Level 0：512 × 512
Level 1：256 × 256
Level 2：128 × 128
Level 3： 64 × 64
...
Level 9：  1 × 1
```

当纹理在屏幕上显示得很小时，直接从完整的 512×512 图片采样会让一个片元覆盖大量纹素，容易出现闪烁和摩尔纹。

Mipmap 提供预先过滤的低分辨率版本。GPU 可以选择与屏幕覆盖范围更接近的 level，减少采样误差并提高缓存效率。

当前缩小过滤设置为 `GL_LINEAR_MIPMAP_LINEAR`。如果没有生成 mipmap，纹理会不完整，使用这种 min filter 时可能无法正常采样。

## 11. 顶点数据增加了纹理坐标

当前每个顶点包含八个 `float`：

```cpp
float vertices[] = {
    // position          // color           // texture coordinate
     0.5f,  0.5f, 0.0f,  1.0f, 0.0f, 0.0f,  1.0f, 1.0f,
     0.5f, -0.5f, 0.0f,  0.0f, 1.0f, 0.0f,  1.0f, 0.0f,
    -0.5f, -0.5f, 0.0f,  0.0f, 0.0f, 1.0f,  0.0f, 0.0f,
    -0.5f,  0.5f, 0.0f,  1.0f, 1.0f, 0.0f,  0.0f, 1.0f
};
```

单个顶点布局为：

```text
[Px Py Pz | R G B | S T]
```

所以三个属性共享同一个 VBO，stride 都是：

```cpp
8 * sizeof(float)
```

VAO 中的配置为：

| 属性 | location | 分量数 | offset |
| --- | ---: | ---: | --- |
| 位置 | 0 | 3 | `0` |
| 颜色 | 1 | 3 | `3 * sizeof(float)` |
| 纹理坐标 | 2 | 2 | `6 * sizeof(float)` |

纹理坐标配置代码：

```cpp
glVertexAttribPointer(
    2,
    2,
    GL_FLOAT,
    GL_FALSE,
    8 * sizeof(float),
    (void*)(6 * sizeof(float))
);
glEnableVertexAttribArray(2);
```

它表示：location 2 从当前 VBO 读取两个 `float`，相邻顶点相隔八个 `float`，第一个纹理坐标从顶点开头偏移六个 `float`。

## 12. Vertex Shader 传递纹理坐标

[`4.1.texture.vs`](../5-texture/4.1.texture.vs) 声明第三个顶点属性：

```glsl
layout (location = 2) in vec2 aTexCoord;
```

这个 location 必须与 C++ 中 `glVertexAttribPointer(2, ...)` 的属性编号一致。

Vertex Shader 定义输出：

```glsl
out vec2 TexCoord;
```

并赋值：

```glsl
TexCoord = vec2(aTexCoord.x, aTexCoord.y);
```

这里也可以直接写：

```glsl
TexCoord = aTexCoord;
```

二者含义相同。

Vertex Shader 只在矩形的四个顶点上得到四组纹理坐标。矩形内部大量片元所需的坐标，由光栅化阶段在顶点输出之间自动插值。

例如三角形一个顶点的纹理坐标是 `(0,0)`，另一个是 `(1,0)`，两者中间位置就会得到接近 `(0.5,0)` 的坐标。

## 13. Fragment Shader 使用 Sampler 采样

[`4.1.texture.fs`](../5-texture/4.1.texture.fs) 接收插值后的纹理坐标：

```glsl
in vec2 TexCoord;
```

声明二维纹理 sampler：

```glsl
uniform sampler2D texture1;
```

并调用：

```glsl
FragColor = texture(texture1, TexCoord);
```

`texture()` 大致执行：

1. 读取 `texture1` 指向的 Texture Unit 编号；
2. 找到该 Texture Unit 上绑定的 `GL_TEXTURE_2D` Texture Object；
3. 使用 `TexCoord` 确定采样位置；
4. 根据 wrapping 处理越界坐标；
5. 根据纹理在屏幕上的缩放程度选择 filtering 和 mipmap；
6. 返回采样得到的 RGBA 颜色。

JPEG 纹理本身只有 RGB，采样返回 `vec4` 时 Alpha 会得到 `1.0`。

当前 Fragment Shader 虽然还声明了：

```glsl
in vec3 ourColor;
```

但最终输出没有使用它：

```glsl
FragColor = texture(texture1, TexCoord);
```

所以 VBO 中的顶点颜色仍然被上传、读取、插值，但不会影响最终画面。

如果写成：

```glsl
FragColor = texture(texture1, TexCoord) * vec4(ourColor, 1.0);
```

顶点颜色就会作为乘法颜色调制纹理，但这不是当前代码的行为。

## 14. 为什么绘制前还要绑定纹理

渲染循环中调用：

```cpp
glBindTexture(GL_TEXTURE_2D, texture);
```

它确保当前活动 Texture Unit 0 上绑定的是本次绘制需要的纹理。

随后：

```cpp
ourShader.use();
glBindVertexArray(VAO);
glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
```

发起绘制时需要的关键状态是：

```text
当前 Shader Program
  └─ texture1 sampler 指向 Texture Unit 0

Texture Unit 0
  └─ GL_TEXTURE_2D 绑定 texture

当前 VAO
  ├─ location 0：位置
  ├─ location 1：颜色
  ├─ location 2：纹理坐标
  └─ EBO：矩形索引
```

如果程序只有这一张纹理，初始化时绑定后从未改变，循环中的重复绑定不是绝对必要的。但显式绑定能够保证绘制前状态正确，也为以后加入多个对象和多张纹理做好准备。

## 15. 图片坐标方向与纹理坐标方向

`stb_image` 解码后的第一个像素通常位于图片左上角，图片数据按从上到下的扫描行排列。

OpenGL 教程中使用的纹理坐标通常把 `(0,0)` 理解为左下角：

```text
图片数据常见原点       OpenGL 纹理坐标示意

(0,0) → x              t
  ↓                     ↑
  y                     |
                        +----→ s
                      (0,0)
```

因此直接上传图片后，有时会观察到纹理上下颠倒。

`stb_image` 提供：

```cpp
stbi_set_flip_vertically_on_load(true);
```

它会在加载时垂直翻转图片数据。

当前项目没有调用这个函数，而是直接使用原始解码方向。是否需要翻转取决于图片内容、纹理坐标约定以及希望得到的画面方向，不能认为所有 OpenGL 纹理都必须固定翻转。

另一种处理方式是不改图片数据，而是将纹理坐标的 `t` 值上下交换。

## 16. 资源生命周期

CPU 图片数据在上传完成后立即释放：

```cpp
stbi_image_free(data);
```

这不会破坏 OpenGL Texture Object，因为 `glTexImage2D` 已经接收了纹理数据。

退出渲染循环后，代码释放：

```cpp
glDeleteVertexArrays(1, &VAO);
glDeleteBuffers(1, &VBO);
glDeleteBuffers(1, &EBO);
```

当前没有释放 Texture Object。更完整的资源清理还应在 OpenGL 上下文仍有效时调用：

```cpp
glDeleteTextures(1, &texture);
```

此外，当前 `Shader` 类没有析构函数，也不会主动调用 `glDeleteProgram`。进程退出和上下文销毁后，系统最终会回收资源，但显式管理生命周期更适合长期运行或频繁创建、销毁资源的程序。

## 关键对象与数据关系总结

| 对象或概念 | 当前示例中的内容 | 主要职责 |
| --- | --- | --- |
| JPEG 文件 | `container.jpg` | 保存压缩后的磁盘图像 |
| `stb_image` | `stbi_load` | 将 JPEG 解码为 CPU 像素 |
| CPU 像素数组 | `unsigned char* data` | 临时保存 RGB 像素 |
| Texture Object | `texture` | 保存 GPU 可采样的纹理和采样参数 |
| Texture Unit | 默认的 Unit 0 | 在 Sampler 和 Texture Object 之间建立绑定槽位 |
| Sampler | `sampler2D texture1` | 指定 Shader 从哪个 Texture Unit 采样二维纹理 |
| 纹理坐标 | VBO 中每个顶点的 `(s,t)` | 指定几何表面对应图片的哪个位置 |
| VAO location 2 | 2 个 `float`、stride 8、offset 6 | 说明怎样从 VBO 读取纹理坐标 |
| Mipmap | 从 512×512 到 1×1 | 为缩小采样提供预过滤纹理 |
| `texture()` | Fragment Shader 采样函数 | 根据坐标和纹理参数计算片元颜色 |

## 当前代码值得注意的地方

- `glfwInit()` 的返回值尚未检查；
- GLAD 初始化失败时没有先调用 `glfwTerminate()`；
- 图片加载失败后程序仍会继续进入渲染循环；
- 图片上传格式固定为 `GL_RGB`，更换四通道 PNG 时需要改为 `GL_RGBA`；
- 没有显式设置 `texture1` 的 Texture Unit，依赖 sampler 初始值 0；
- 没有显式调用 `glActiveTexture(GL_TEXTURE0)`，依赖初始活动 Texture Unit 0；
- Vertex Shader 仍传递 `ourColor`，但 Fragment Shader 没有使用；
- 没有调用 `stbi_set_flip_vertically_on_load`，纹理方向由当前图片数据和坐标共同决定；
- 退出时没有调用 `glDeleteTextures`；
- 当前内部格式使用普通 `GL_RGB`，没有讨论 sRGB 纹理与 gamma 校正，后续光照章节需要重新关注颜色空间。

## 一帧纹理绘制的完整过程

纹理创建完成后，每一帧可以按下面的顺序理解：

```text
清除颜色缓冲区
      ↓
把 texture 绑定到活动 Texture Unit 0
      ↓
启用包含 texture1 sampler 的 Shader Program
      ↓
绑定记录位置、颜色、纹理坐标和 EBO 的 VAO
      ↓
glDrawElements 读取六个索引
      ↓
从 VBO 获取四个顶点的位置和纹理坐标
      ↓
Vertex Shader 输出 gl_Position 与 TexCoord
      ↓
图元装配、光栅化并插值 TexCoord
      ↓
Fragment Shader 使用 texture1 和 TexCoord 采样
      ↓
wrapping + filtering + mipmap 决定采样结果
      ↓
采样颜色写入 FragColor
      ↓
交换缓冲区，图片出现在窗口中
```

## 本章最重要的理解

纹理并不是“直接贴到三角形上的图片”。它由几组相互配合的状态构成：

```text
图片文件
  ↓ 解码
CPU 像素
  ↓ 上传
Texture Object
  ↓ 绑定到
Texture Unit
  ↑ sampler 指向
Fragment Shader
  ↑ 接收插值坐标
Vertex Shader
  ↑ VAO 解释
VBO 中的纹理坐标
```

其中：

- Texture Object 回答“纹理数据和采样参数在哪里”；
- Texture Unit 回答“这次绘制把哪张纹理接到哪个槽位”；
- Sampler 回答“Shader 从哪个槽位、以什么纹理类型采样”；
- 纹理坐标回答“当前片元应该读取图片的哪个位置”；
- Wrapping、filtering 和 mipmap 回答“坐标越界或像素比例不一致时怎样得到颜色”。

理解这条链路后，多纹理只是增加 Texture Unit 和 Sampler 的对应关系：使用 `glActiveTexture` 选择不同 Unit，把不同 Texture Object 绑定进去，再把多个 sampler uniform 分别指向这些 Unit。

## 从这个项目继续学习

- 显式调用 `glActiveTexture(GL_TEXTURE0)` 并设置 `texture1 = 0`，让单纹理绑定关系更清晰；
- 再加载一张带 Alpha 的 PNG，比较 `GL_RGB` 与 `GL_RGBA`；
- 使用两个 Texture Unit 和两个 sampler，在 Fragment Shader 中混合两张纹理；
- 修改纹理坐标到 `[0,2]`，观察不同 wrapping 模式；
- 在 `GL_NEAREST`、`GL_LINEAR` 和 mipmap 过滤之间切换，观察放大与缩小时的差异；
- 了解 sRGB 纹理内部格式与 gamma 校正，区分颜色纹理和数据纹理。
