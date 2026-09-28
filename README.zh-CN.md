# SmoothScrollbar

[English](README.md)

SmoothScrollbar 是一个轻量、无额外依赖的 Qt Widgets 库，为 `QAbstractScrollArea`
及其子类提供根据输入方式调整的平滑滚动。

它会为离散的鼠标滚轮输入添加动画，同时保留精密触控板的原生像素滚动。
可选的触摸惯性滚动由 Qt 的 `QScroller` 负责，无须另外实现一套物理引擎。

## 环境要求

- C++17
- Qt 5.15.2 或 Qt 6.5 及更新版本
- CMake 3.21 或更新版本

## 使用方法

推荐使用控制器 API，因为滚轮事件通常会发送到滚动区域的 viewport，
而不只是滚动条。

```cpp
#include <smoothscroll/smoothscrollcontroller.h>

auto* controller = new sscroll::SmoothScrollController(view, view);

sscroll::SmoothScrollSettings settings;
settings.animationDuration = 300; // 滚轮动画时长（毫秒）；0 表示关闭动画和惯性。
settings.easingCurve = QEasingCurve::OutCubic; // 滚轮及惯性动画的缓动曲线。
settings.wheelDistanceFactor = 1.0; // 换算后滚轮位移的倍率。
settings.wheelStep = 0; // 每格滚轮移动的滚动条单位数；0 表示按系统滚轮行数换算。
settings.minimumWheelStep = 20; // wheelStep 为 0 时使用的最小滚动条单步距离。
settings.maximumPendingDistance = 1200; // 动画器的待滚动距离上限；0 表示不启用该限制。
settings.pixelDeltaMode = sscroll::PixelDeltaMode::Native; // 触控板像素滚动交给 Qt；Animated 改由控制器处理。
settings.boundaryPolicy = sscroll::BoundaryPolicy::Propagate; // 到达边界时交给父滚动区；Consume 则直接消费事件。
settings.shiftSwapsAxes = true; // Shift+滚轮切换为水平滚动。
settings.preserveControlWheel = true; // Ctrl+滚轮留给应用处理，例如缩放。
settings.kineticTouchEnabled = false; // 为触摸手势启用 Qt QScroller 惯性滚动。
settings.wheelAccelerationEnabled = false; // 连续快速滚轮输入时增加每次位移。
settings.wheelAccelerationStrength = 1.5; // 加速强度（0-3）；仅启用滚轮加速时生效。
settings.wheelMomentumEnabled = true; // 快速滚轮输入结束后继续滑动。
settings.wheelMomentumMinimumSteps = 3; // 触发惯性所需的同向滚轮格数，最小为 2。
controller->setSettings(settings);
```

鼠标滚轮惯性默认开启。连续快速向同一方向滚动达到
`wheelMomentumMinimumSteps` 格后，滚轮停止时才会继续滑动；默认门槛为 3 格。
接下来的 2 格会逐步增强惯性，续滑距离和动画时长也会随最终输入速度增加，
然后按选定的缓动曲线减速。由于速度计算需要连续输入，门槛最小为 2 格。
设置 `settings.wheelMomentumEnabled = false` 可关闭该功能。
独立的 `wheelAccelerationEnabled` 选项用于增加连续滚轮输入期间每次事件的
滚动距离，默认关闭。原生触控板像素滚动不会额外叠加滚轮惯性。

## 输入行为

- 精密触控板产生的 `pixelDelta()` 事件默认由 Qt 原生处理。
- 离散的 `angleDelta()` 事件根据系统滚轮行数和目标滚动条的单步距离换算。
  可配置的最小步长使按像素滚动的列表视图仍有明显的滚动反馈。
- 将 `wheelStep` 设置为正数，可直接指定每格滚轮对应的滚动条单位数。
  在 `ScrollPerPixel` 模式下，该单位就是像素；默认值 `0` 表示使用系统设置换算。
- `Shift+Wheel` 将垂直滚轮输入映射到水平方向。
- `Ctrl+Wheel` 保持原有处理方式，以便应用实现缩放。
- 到达边界且无法继续滚动时，事件会被忽略，允许外层滚动区域接手。
- 反向滚动会立即中断尚未完成的目标位移。
- 鼠标操作滚动条会取消当前动画。

所有策略都可通过 `SmoothScrollSettings` 配置。

## 构建

```console
cmake -S . -B build -DSMOOTHSCROLLBAR_BUILD_TESTS=ON
cmake --build build
ctest --test-dir build --output-on-failure
```

安装后可通过导出的 CMake 目标 `SmoothScrollbar::SmoothScrollbar` 引用本库。

## 许可证

MIT，详见 [LICENSE](LICENSE)。
