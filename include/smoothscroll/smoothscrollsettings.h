#pragma once

#include <QEasingCurve>
#include <Qt>

namespace sscroll {

enum class PixelDeltaMode {
    Native,
    Animated
};

enum class BoundaryPolicy {
    Propagate,
    Consume
};

struct SmoothScrollSettings {
    int animationDuration = 300;
    QEasingCurve easingCurve = QEasingCurve::OutCubic;
    qreal wheelDistanceFactor = 1.0;
    int wheelStep = 0;
    int minimumWheelStep = 20;
    int maximumPendingDistance = 1200;
    PixelDeltaMode pixelDeltaMode = PixelDeltaMode::Native;
    BoundaryPolicy boundaryPolicy = BoundaryPolicy::Propagate;
    bool shiftSwapsAxes = true;
    bool preserveControlWheel = true;
    bool kineticTouchEnabled = false;
    bool wheelAccelerationEnabled = false;
    qreal wheelAccelerationStrength = 1.5;
    bool wheelMomentumEnabled = true;
    int wheelMomentumMinimumSteps = 3;

    [[nodiscard]] SmoothScrollSettings normalized() const
    {
        SmoothScrollSettings result = *this;
        result.animationDuration = qMax(0, result.animationDuration);
        result.wheelDistanceFactor = qMax<qreal>(0.0, result.wheelDistanceFactor);
        result.wheelStep = qMax(0, result.wheelStep);
        result.minimumWheelStep = qMax(1, result.minimumWheelStep);
        result.maximumPendingDistance = qMax(0, result.maximumPendingDistance);
        result.wheelAccelerationStrength = qBound<qreal>(0.0, result.wheelAccelerationStrength, 3.0);
        result.wheelMomentumMinimumSteps = qMax(2, result.wheelMomentumMinimumSteps);
        return result;
    }
};

} // namespace sscroll
