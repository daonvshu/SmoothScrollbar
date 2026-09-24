# SmoothScrollbar

SmoothScrollbar is a small, dependency-free Qt Widgets library that adds
input-aware smooth scrolling to `QAbstractScrollArea` subclasses.

It animates discrete mouse-wheel input while preserving native pixel scrolling
from precision touchpads. Optional touch kinetic scrolling is delegated to
Qt's `QScroller` instead of implementing a second physics engine.

## Requirements

- C++17
- Qt 5.15.2 or Qt 6.5 and newer
- CMake 3.21 or newer

## Usage

The controller API is preferred because wheel events are delivered to a scroll
area's viewport rather than exclusively to its scroll bars.

```cpp
#include <smoothscroll/smoothscrollcontroller.h>

auto* controller = new smoothscroll::SmoothScrollController(view, view);

smoothscroll::SmoothScrollSettings settings;
settings.animationDuration = 300;
settings.easingCurve = QEasingCurve::OutCubic;
settings.kineticTouchEnabled = true;
controller->setSettings(settings);
```

For source compatibility with the original project, a `SmoothScrollbar`
subclass remains available:

```cpp
#include <smoothscrollbar.h>

view->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
view->setVerticalScrollBar(new SmoothScrollbar(Qt::Vertical, view));
```

The compatibility class only handles events delivered to that scroll bar. New
code should use `SmoothScrollController` so both axes, nested scrolling and
viewport events are handled consistently.

## Input behavior

- Precision `pixelDelta()` events retain Qt's native handling by default.
- Discrete `angleDelta()` events are converted using the platform wheel-line
  setting and the target scroll bar's single step. A configurable minimum step
  keeps pixel-scrolling item views visibly responsive.
- `Shift+Wheel` maps vertical input to the horizontal axis.
- `Ctrl+Wheel` is left untouched for application zoom gestures.
- Events that cannot move at a boundary are ignored so a parent scroll area can
  handle them.
- Opposite-direction input interrupts the pending target immediately.
- Mouse interaction with a scroll bar cancels its active animation.

All policies are configurable through `SmoothScrollSettings`.

## Build

```console
cmake -S . -B build -DSMOOTHSCROLLBAR_BUILD_TESTS=ON
cmake --build build
ctest --test-dir build --output-on-failure
```

Install and consume the exported CMake target as
`SmoothScrollbar::SmoothScrollbar`.

## License

MIT. See [LICENSE](LICENSE).
