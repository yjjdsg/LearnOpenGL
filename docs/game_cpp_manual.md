# game.cpp 函数文档

> 对应文件：`Project1/game.cpp`（Game 类实现）· 声明在 `Project1/game.h` · 入口 `main()` 在 `源2.cpp`
> 生成日期：2026-11-27

---

## 1. 文件定位

`game.cpp` 是唯一的 **Game 类实现翻译单元**。它只负责"把窗口、GL 上下文、着色器、场景对象串起来并跑循环"，具体职责被拆到下列文件中：

| 文件 | 提供什么 | 是否可被 `#include` |
|---|---|---|
| `game.h` | `class Game` 的完整声明 | ✅ 只有它该被 include |
| `shaderr.h` / `shaderr.cpp` | `Shader` 类（着色器编译链接封装） | `shaderr.h` ✅ |
| `camera.cpp` | `Camera` 类（自由飞行相机，header-only） | ✅ |
| `mesh.cpp` | `Vertex` / `Texture` / `Mesh`（header-only） | ✅ |
| `model.cpp` | `Model` + `TextureFromFile`（header-only，含 `inline`） | ✅ |
| `myimage.cpp` | `Image` 贴图辅助（header-only） | ✅ |
| `glad.c` / `stb_image.cpp` | 第三方实现，各编译一次 | ❌ |
| `源2.cpp` | `main()` | ❌ |

**约定**：`game.cpp` 自己包含 `camera.cpp`、`model.cpp`，但它本身**不能被别的 .cpp include**，否则 Game 的每个成员函数都会出现第二份定义（LNK2005）。

---

## 2. 匿名命名空间：常量与工具函数

### 2.1 常量

| 名称 | 值 | 含义 |
|---|---|---|
| `kCubeSize` | `0.5f` | 单个方块的边长（世界单位）。顶点数据是 1×1×1 的单位立方体，靠 model 矩阵缩放得到。相邻方块中心正好相距 `kCubeSize`，无缝贴合 |
| `kSameSpot` | `1e-4f` | 判断"该位置已有方块"的距离容差，用于 `addCube` 去重 |
| `kCrosshairHalfLengthPx` | `10.0f` | 准心**半长**（像素），总长 20 px，不随分辨率变化 |
| `kCrosshairRimThickness` | `0.22f` | 准心暗边粗细，单位是"半长的比例"（0.22 × 10 ≈ 2.2 px 半厚） |
| `kCrosshairCoreThickness` | `0.13f` | 准心白色核心粗细，同上比例 |

> 三个准心常量都在 `renderCrosshair()` 里换算成 NDC 后传给 uniform。

### 2.2 `bool rayIntersectBox(...)`

```cpp
bool rayIntersectBox(const glm::vec3& origin, const glm::vec3& dir,
                     const glm::vec3& boxMin, const glm::vec3& boxMax,
                     float& tHit, glm::vec3& normal)
```

- **作用**：射线与轴对齐包围盒（AABB）求交，slab 法。用于右键放置方块时的拾取。
- **参数**
  - `origin` / `dir`：射线起点与方向，`dir` 应为单位向量（内部不做归一化）
  - `boxMin` / `boxMax`：包围盒的两个对角
  - `tHit`（输出）：沿射线的命中距离；起点已在盒内时取 `0`
  - `normal`（输出）：**进入面**的外法线，主轴方向之一，如 `(0,0,1)`
- **返回**：命中返回 `true`；射线平行且不在 slab 内、或整个盒子在射线背后时返回 `false`
- **实现要点**：逐轴求 `tNear/tFar`，若 `tNear > tFar` 则交换并翻转该轴法线；三轴区间取交集，交集为空即未命中

---

## 3. Game 类成员函数

### 3.0 速查表

| 函数 | 可见性 | 一句话 |
|---|---|---|
| `Game(int w, int h)` | public | 只记录窗口尺寸，不碰 GL |
| `~Game()` | public | 按序释放 GL 资源、销毁窗口、终止 GLFW |
| `bool init()` | public | 三步初始化总入口 |
| `void run()` | public | 主循环，直到窗口关闭 |
| `void addCube(const glm::vec3&)` | public | 往方块数组里加一个（自动去重） |
| `const std::vector<glm::vec3>& getCubes() const` | public | 只读访问方块数组（定义在 game.h 内联） |
| `bool initWindow()` | private | 建 GLFW 窗口与上下文、注册回调 |
| `bool initGL()` | private | 加载 GLAD、设视口、开深度测试 |
| `bool initScene()` | private | 建 VAO/VBO、编译着色器、建相机、放首个方块、加载模型 |
| `void processInput()` | private | 每帧读键盘（ESC / X） |
| `void update(float dt)` | private | 相机移动与鼠标视角 |
| `void render()` | private | 清屏 → 设 uniform → 画所有方块 → 画准心 |
| `void drawCube(const glm::vec3&)` | private | 画一个方块（需 litShader 已绑定） |
| `void tryPlaceCube()` | private | 右键拾取，在命中面上放新方块 |
| `void renderCrosshair(int,int)` | private | 屏幕空间画准心 |
| `framebufferSizeCallback` | private static | 窗口尺寸变化 → `glViewport` |
| `mouseCallback` | private static | 累计鼠标位移到 `xoffset/yoffset` |
| `scrollCallback` | private static | 滚轮改 FOV（1..45） |
| `mouseButtonCallback` | private static | 右键按下 → `tryPlaceCube()` |

---

### 3.1 构造与析构

#### `Game::Game(int w = 800, int h = 600)`

```cpp
Game::Game(int w, int h) : width(w), height(h) {}
```

- **作用**：仅保存窗口尺寸。**不初始化任何 GL 对象**，也不调用 `glfwInit`，所以构造 `Game` 不会失败。
- **为什么延迟**：GL 上下文必须由 `initWindow()` 创建后才能创建 GL 对象。

#### `Game::~Game()`

- **作用**：逆序释放所有资源，顺序很重要：

| 顺序 | 动作 | 原因 |
|---|---|---|
| 1 | `litShader.reset()` / `crosshairShader.reset()` / `model.reset()` / `camera.reset()` | 删除 GL program / VAO / 纹理，必须在上下文仍有效时执行 |
| 2 | `glDeleteBuffers` / `glDeleteVertexArrays` | 释放 `cubeVBO`、`cubeVAO`、`crosshairVAO` |
| 3 | `glfwDestroyWindow(window)` | 销毁窗口（同时销毁其 GL 上下文） |
| 4 | `glfwTerminate()` | 仅当 `glfwReady` 为真时调用，避免对未初始化的 GLFW 收尾 |

- **注意**：所有 `glDelete*` 都先判空（`if (cubeVBO)`），因为 `init()` 可能在中途失败。

---

### 3.2 初始化链路

#### `bool Game::init()`

```cpp
if (!initWindow()) return false;
if (!initGL())    return false;
return initScene();
```

- **顺序不可交换**：`initWindow` 建上下文 → `initGL` 加载函数指针 → `initScene` 才能创建 VAO/着色器。
- **返回**：三步全成功返回 `true`；任一步失败立即返回 `false`，`main()` 据此退出。
- **调用者**：`源2.cpp` 的 `main()`。

#### `bool Game::initWindow()`

| 项 | 说明 |
|---|---|
| 作用 | `glfwInit` → 设置 GL 3.3 Core → 创建 800×600 窗口 → 设为当前上下文 → 注册 4 个回调 |
| 参数 | 用成员 `width` / `height`，窗口标题固定 `"LearnOpenGL"` |
| 返回 | GLFW 初始化失败或窗口创建失败返回 `false` |
| 副作用 | 置 `glfwReady = true`；`glfwSetWindowUserPointer(window, this)` 把 this 指针挂到窗口上，供静态回调找回实例 |

> 回调函数是 C 函数指针，不能是成员函数，所以都声明成 `static`，通过 `glfwGetWindowUserPointer()` 拿回 `Game*`。

#### `bool Game::initGL()`

- **作用**：`gladLoadGLLoader` 加载 OpenGL 函数指针 → `glViewport(0,0,width,height)` → `glEnable(GL_DEPTH_TEST)`
- **返回**：GLAD 加载失败返回 `false`
- **注意**：必须在 `initWindow()` 之后，因为 GLAD 需要 `glfwGetProcAddress`

#### `bool Game::initScene()`

一次性创建全部场景资源，分四段：

**① 方块几何**

```cpp
glGenBuffers / glBufferData(..., sizeof(kCubeVertices), kCubeVertices, GL_STATIC_DRAW);
glGenVertexArrays / glBindVertexArray(cubeVAO);
glVertexAttribPointer(0, 3, ..., 6*sizeof(float), (void*)0);                  // 位置
glVertexAttribPointer(1, 3, ..., 6*sizeof(float), (void*)(3*sizeof(float)));  // 法线
```

- 交错布局：`[px,py,pz, nx,ny,nz]`，步长 6 floats
- location 0 = 位置，location 1 = 法线，必须和 `vertex` 着色器的 `layout(location=...)` 一致

**② 主着色器**

```cpp
litShader = std::make_unique<Shader>("vertex", "fragmenttex");
if (!litShader->isValid()) { ...; return false; }
```

- 失败即整体失败（没有着色器就没有画面）

**③ 准心**

```cpp
glGenVertexArrays(1, &crosshairVAO);      // 空 VAO，顶点由 gl_VertexID 生成
crosshairShader = std::make_unique<Shader>("crosshair_vertex", "crosshair_fragment");
if (!crosshairShader->isValid()) { 打印警告，但继续 }   // 准心是装饰，失败不阻断
```

**④ 相机、首个方块、模型**

```cpp
camera = std::make_unique<Camera>(glm::vec3(0.0f, 0.0f, 3.0f));  // 原点后方 3 单位
camera->setMouseSensitivity(0.05f);
glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);      // 锁定光标
addCube(glm::vec3(0.0f, 0.0f, 0.0f));                             // 正对相机的一个方块
model = std::make_unique<Model>("cube.obj");                      // 加载但不绘制
```

- `GLFW_CURSOR_DISABLED` 让鼠标不移动、只产生位移量 —— 这也是"准心即屏幕中心"的前提
- `cube.obj` 是 2×2×2 立方体，会包住 0.5 的小方块，所以 `render()` 里的 `model->Draw()` 目前是注释状态

---

### 3.3 主循环

#### `void Game::run()`

```cpp
while (!glfwWindowShouldClose(window))
{
    float currentFrame = (float)glfwGetTime();
    deltaTime = currentFrame - lastFrame;
    lastFrame = currentFrame;

    processInput();
    update(deltaTime);
    render();

    glfwSwapBuffers(window);
    glfwPollEvents();
}
```

- **作用**：阻塞式主循环，直到窗口关闭或按 ESC
- **每帧顺序固定**：输入 → 更新 → 渲染 → 交换缓冲 → 处理事件
- `deltaTime` 为秒，用于把"每秒 2 单位"的速度换算成"本帧位移"

#### `void Game::processInput()`

| 按键 | 行为 |
|---|---|
| `ESC` | `glfwSetWindowShouldClose(window, true)` |
| `X` | 切换准心显隐（带边沿检测，按住只触发一次） |

```cpp
const bool toggleDown = glfwGetKey(window, GLFW_KEY_X) == GLFW_PRESS;
if (toggleDown && !crosshairToggleHeld)
    crosshairVisible = !crosshairVisible;
crosshairToggleHeld = toggleDown;
```

> 边沿检测靠成员 `crosshairToggleHeld` 保存上一帧状态，否则按住 X 会每帧翻转、画面闪烁。

#### `void Game::update(float dt)`

```cpp
camera->setspeed(2.0f * dt);                          // 速度 = 2 单位/秒 × 帧时长
camera->ProcessMouseMovement((float)xoffset, (float)yoffset);
xoffset = 0.0; yoffset = 0.0;                          // 消费完立即清零
camera->processInput(window);                          // WASD / 空格 / 左Ctrl
```

- **参数** `dt`：上一帧耗时（秒）
- **副作用**：消费并清零鼠标增量 `xoffset/yoffset`；相机位置与朝向被更新
- **注意**：`xoffset/yoffset` 由 `mouseCallback` 累加，必须在这里清零，否则会持续漂移

#### `void Game::render()`

```cpp
int fbWidth, fbHeight;
glfwGetFramebufferSize(window, &fbWidth, &fbHeight);
if (fbHeight == 0) return;                 // 窗口最小化，跳过整帧
```

| 步骤 | 代码 | 说明 |
|---|---|---|
| 1 | `glClearColor(0.1,0.1,0.1,1)` + `glClear` | 深灰背景，同时清颜色和深度 |
| 2 | `glm::perspective(radians(fov), aspect, 0.1f, 100.0f)` | 投影矩阵，近平面 0.1、远平面 100 |
| 3 | `camera->GetViewMatrix()` | 视图矩阵 |
| 4 | `litShader->use()` | **必须最先调用**，`glUniform*` 只作用于当前 program |
| 5 | 设置 `projection` / `view` / `viewPos` | 每帧都会变 |
| 6 | 设置定向光 4 个 uniform | 固定值，逐帧重复设置（简单优先） |
| 7 | 设置材质 4 个 uniform | 同上 |
| 8 | `for (cube : cubes) drawCube(cube)` | 遍历数组绘制 |
| 9 | `glBindVertexArray(0)` | 解绑，避免污染后续调用 |
| 10 | `renderCrosshair(fbWidth, fbHeight)` | 最后画，覆盖在 3D 之上 |

- **返回**：无（`void`）；窗口最小化时提前返回，不渲染
- **被注释的部分**：`if (model) model->Draw(*litShader);` —— 恢复它就能显示 assimp 模型

#### `void Game::renderCrosshair(int fbWidth, int fbHeight)`

| 项 | 说明 |
|---|---|
| 作用 | 在屏幕正中画十字准心（暗边 + 白核两层） |
| 参数 | `fbWidth/fbHeight`：**像素尺寸**（来自 `glfwGetFramebufferSize`，不是窗口逻辑尺寸） |
| 提前返回 | 准心被隐藏 / shader 为空 / shader 无效 / 尺寸 ≤ 0 |
| 状态保护 | 进入时 `glDisable(GL_DEPTH_TEST)` + 开混合；退出时恢复 `glDisable(GL_BLEND)` + `glEnable(GL_DEPTH_TEST)` |

**尺寸换算**（关键）：

```cpp
const float halfSize = kCrosshairHalfLengthPx * 2.0f / (float)fbHeight;  // → NDC
const float aspect   = (float)fbWidth / (float)fbHeight;
```

- `1.0 NDC = fbHeight / 2` 像素，所以要乘 2 再除以高度
- 着色器里 x 方向**除以** aspect 才是正方形 —— 乘会得到扁十字

**两次绘制**：

```cpp
uThickness = kCrosshairRimThickness;  uColor = 黑;  glDrawArrays(...);  // 底
uThickness = kCrosshairCoreThickness; uColor = 白;  glDrawArrays(...);  // 芯
```

- 只用一个 VAO、4 个顶点、`GL_TRIANGLE_STRIP`（顶点由 `gl_VertexID` 生成）

---

### 3.4 方块管理

#### `void Game::addCube(const glm::vec3& center)` — public

```cpp
for (const auto& existing : cubes)
    if (glm::length(existing - center) < kSameSpot) return;   // 已占用，忽略
cubes.push_back(center);
```

- **参数** `center`：方块**中心**的世界坐标（不是角点）
- **返回**：无。重复位置静默忽略，不报错
- **复杂度**：O(n) 线性查重；方块数量大时可换 `unordered_set` 或网格哈希
- **调用者**：`initScene()`、`tryPlaceCube()`
- **getter**：`game.h` 里内联的 `getCubes()` 返回 `const std::vector<glm::vec3>&`

#### `void Game::drawCube(const glm::vec3& center)`

```cpp
glm::mat4 model(1.0f);
model = glm::translate(model, center);
model = glm::scale(model, glm::vec3(kCubeSize));
litShader->setMat4("model", model);
glBindVertexArray(cubeVAO);
glDrawArrays(GL_TRIANGLES, 0, 36);
```

- **前置条件**：`litShader` 必须已经 `use()` 且投影/视图/光照 uniform 已设置
- **为什么一模一样**：所有方块共用同一个 VAO 与 shader，差异只在 model 矩阵
- **顶点数**：36 = 6 面 × 2 三角形 × 3 顶点

#### `void Game::tryPlaceCube()`

右键触发，流程：

```
相机位置 + 相机朝向  →  射线
        ↓
遍历 cubes，对每个方块做 rayIntersectBox（AABB 半边长 = kCubeSize/2）
        ↓
记录 t 最小的命中（nearest）与命中面法线
        ↓
未命中 → 直接返回，什么都不做
        ↓
新方块中心 = hitCenter + hitNormal * kCubeSize   （贴着被击中的那一面）
        ↓
addCube(...)
```

- **前提**：光标被 `GLFW_CURSOR_DISABLED` 锁住，所以"瞄准方向"就是 `camera->Front`
- **`dir` 归一化**：`rayIntersectBox` 内部不归一化，这里显式 `normalize`
- **为什么用 `kCubeSize` 而不是 `kCubeSize/2`**：从中心 A 挪到相邻格中心 B，位移正好是一整个边长
- **无返回值**：失败静默

---

### 3.5 GLFW 静态回调

四个回调都遵循同一模式：从窗口上取回 `Game*`，判空，再操作成员。

```cpp
Game* self = static_cast<Game*>(glfwGetWindowUserPointer(win));
if (!self) return;
```

| 函数 | 触发时机 | 行为 |
|---|---|---|
| `framebufferSizeCallback(win, w, h)` | 窗口缩放 / DPI 改变 | `glViewport(0, 0, w, h)` |
| `mouseCallback(win, xpos, ypos)` | 鼠标移动 | 首次只记录位置（`mouseFirstSample`），之后累加 `xoffset/yoffset`，**y 取反**（屏幕 y 向下） |
| `scrollCallback(win, xo, yo)` | 滚轮 | `fov -= yo`，夹在 `[1, 45]` 度 |
| `mouseButtonCallback(win, btn, action, mods)` | 鼠标按下/松开 | 仅 `GLFW_MOUSE_BUTTON_RIGHT + GLFW_PRESS` → `tryPlaceCube()` |

- **`mouseFirstSample` 的作用**：窗口刚获得焦点时 `xpos/ypos` 可能是 (0,0)，直接用会产生一次巨大跳变，所以第一帧只记录不计算
- **`scrollCallback` 的参数名**：`xOffset/yOffset` 是局部参数名，注意和成员 `xoffset/yoffset` 区分（成员是鼠标位移）
- **FOV 上限 45°**：和初始值一致，滚轮只能缩小视野（放大画面）

---

## 4. 静态数据成员

#### `static const float Game::kCubeVertices[216]`

- **内容**：36 个顶点 × 6 个 float = 216，交错存放 `位置(3) + 法线(3)`
- **形状**：以原点为中心的 1×1×1 立方体（±0.5），6 个面各 2 个三角形，逆时针缠绕
- **为什么静态**：不依赖实例；放在 `.cpp` 里定义避免头文件重复定义
- **法线**：每个面 6 个顶点法线相同（平面法线，不做平滑）

---

## 5. 一帧的完整调用顺序

```
main()                          [源2.cpp]
 └─ game.init()
     ├─ initWindow()            窗口 + 上下文 + 4 个回调
     ├─ initGL()                GLAD + viewport + depth test
     └─ initScene()             VAO/VBO + 2 套 shader + 相机 + 首方块 + 模型
 └─ game.run()
     └─ while (窗口未关闭)
         ├─ 计算 deltaTime
         ├─ processInput()      ESC / X
         ├─ update(dt)          相机移动 + 鼠标视角
         ├─ render()
         │   ├─ 清屏
         │   ├─ litShader->use() + 10 个 uniform
         │   ├─ drawCube() × N
         │   └─ renderCrosshair()
         ├─ glfwSwapBuffers()
         └─ glfwPollEvents()    → 可能触发 mouseCallback / mouseButtonCallback
                                   → mouseButtonCallback 里调 tryPlaceCube()
 └─ ~Game()                      释放顺序：GL 对象 → 窗口 → GLFW
```

---

## 6. 与外部模块的接口

### 6.1 用到的类方法

| 类 | 方法 | 用在 |
|---|---|---|
| `Shader` | `Shader(vsPath, fsPath)` / `use()` / `isValid()` / `setMat4` / `setVec3` / `setFloat` | `initScene`、`render`、`drawCube`、`renderCrosshair` |
| `Camera` | 构造 / `setMouseSensitivity` / `setspeed` / `ProcessMouseMovement` / `processInput` / `GetViewMatrix` / 成员 `Position`、`Front` | `initScene`、`update`、`render`、`tryPlaceCube` |
| `Model` | 构造（文件路径） / `Draw(Shader&)` | `initScene`（`Draw` 目前被注释） |

### 6.2 uniform 名字清单

**主着色器**（`vertex` + `fragmenttex`）

| uniform | 类型 | 设置位置 |
|---|---|---|
| `model` | mat4 | `drawCube()` |
| `view` | mat4 | `render()` |
| `projection` | mat4 | `render()` |
| `viewPos` | vec3 | `render()` |
| `light.direction` | vec3 | `render()` |
| `light.ambient` / `light.diffuse` / `light.specular` | vec3 | `render()` |
| `material.ambient` / `material.diffuse` / `material.specular` | vec3 | `render()` |
| `material.shininess` | float | `render()` |

**准心着色器**（`crosshair_vertex` + `crosshair_fragment`）

| uniform | 类型 | 含义 |
|---|---|---|
| `uAspect` | float | 宽高比，x 方向要除以它 |
| `uHalfSize` | float | 方形半边长（NDC） |
| `uThickness` | float | 条的半厚 / 半长 |
| `uColor` | vec3 | 本层颜色（黑边或白核） |

### 6.3 运行时需要的资源文件

必须在**工作目录**下（VS 调试时是 `$(ProjectDir)`；双击 exe 时是 exe 所在目录）：

```
vertex   fragmenttex   crosshair_vertex   crosshair_fragment   cube.obj   cube.mtl
```

---

## 7. 常见扩展点

| 想做的事 | 改哪里 |
|---|---|
| 调整方块大小 | `kCubeSize`（间距自动跟随） |
| 换打光方向 | `game.h` 的 `lightDir`（光线**传播方向**，会走 normalize） |
| 改准心大小/粗细 | `kCrosshairHalfLengthPx` / `kCrosshairRimThickness` / `kCrosshairCoreThickness` |
| 让方块有不同颜色 | `render()` 里按方块传 `material.*`，或给 `drawCube` 加颜色参数 |
| 左键删除方块 | 仿 `tryPlaceCube()` 写 `tryRemoveCube()`，在 `mouseButtonCallback` 里接 `GLFW_MOUSE_BUTTON_LEFT`，从 `cubes` 中 erase 命中项 |
| 恢复显示 assimp 模型 | 取消 `render()` 末尾 `model->Draw(*litShader)` 的注释，并给它一个 `model` 矩阵挪开位置 |
| 换窗口标题/尺寸 | `initWindow()` 里的 `glfwCreateWindow(width, height, "LearnOpenGL", ...)` |
| 加准心开关以外的按键 | `processInput()`，注意用边沿检测（参考 X 键） |
