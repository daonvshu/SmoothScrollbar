# SmoothScrollbar

[简体中文](README.zh-CN.md)

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

auto* controller = new sscroll::SmoothScrollController(view, view);

sscroll::SmoothScrollSettings settings;
settings.animationDuration = 300; // Wheel animation duration in ms; 0 disables animation and momentum.
settings.easingCurve = QEasingCurve::OutCubic; // Easing curve for wheel and momentum animations.
settings.wheelDistanceFactor = 1.0; // Multiplier for normalized wheel travel.
settings.wheelStep = 0; // Scroll-bar units per notch; 0 uses the system wheel-line setting.
settings.minimumWheelStep = 20; // Minimum scroll-bar single step used when wheelStep is 0.
settings.maximumPendingDistance = 1200; // Animator's pending-travel limit; 0 disables that clamp.
settings.pixelDeltaMode = sscroll::PixelDeltaMode::Native; // Native touchpad pixels; Animated uses this controller.
settings.boundaryPolicy = sscroll::BoundaryPolicy::Propagate; // Let a parent scroll area handle boundary input; Consume swallows it.
settings.shiftSwapsAxes = true; // Route Shift+Wheel to horizontal scrolling.
settings.preserveControlWheel = true; // Leave Ctrl+Wheel to the application (for example, zoom).
settings.kineticTouchEnabled = false; // Enable Qt QScroller for touch gestures.
settings.wheelAccelerationEnabled = false; // Increase travel for rapid, repeated wheel input.
settings.wheelAccelerationStrength = 1.5; // Acceleration amount (0-3); used only when acceleration is enabled.
settings.wheelMomentumEnabled = true; // Continue scrolling after a sufficiently fast wheel burst.
settings.wheelMomentumMinimumSteps = 3; // Same-direction notches required for momentum; minimum 2.
controller->setSettings(settings);
```

Mouse-wheel momentum is enabled by default. Rapid, consecutive wheel input
continues after the last event only after `wheelMomentumMinimumSteps` notches
in the same direction (default: three). Momentum ramps up over two more notches;
both travel distance and animation duration then scale with the final input
speed before decelerating with the selected easing curve. The minimum is two
notches because velocity requires consecutive inputs. Set
`settings.wheelMomentumEnabled = false` to disable it. The separate
`wheelAccelerationEnabled` option increases each wheel event's distance while
input is arriving; it is disabled by default. Native touchpad pixel scrolling
is not given additional momentum.

## Input behavior

- Precision `pixelDelta()` events retain Qt's native handling by default.
- Discrete `angleDelta()` events are converted using the platform wheel-line
  setting and the target scroll bar's single step. A configurable minimum step
  keeps pixel-scrolling item views visibly responsive.
- Set `wheelStep` to a positive value to choose the distance per wheel notch
  directly in scroll bar units. In `ScrollPerPixel` mode this value is pixels;
  the default `0` keeps the platform-based calculation.
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
