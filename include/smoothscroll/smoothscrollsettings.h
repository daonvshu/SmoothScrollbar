#pragma once

#include <QEasingCurve>
#include <Qt>

namespace smoothscroll {

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
    int minimumWheelStep = 20;
    int maximumPendingDistance = 1200;
    PixelDeltaMode pixelDeltaMode = PixelDeltaMode::Native;
    BoundaryPolicy boundaryPolicy = BoundaryPolicy::Propagate;
    bool shiftSwapsAxes = true;
    bool preserveControlWheel = true;
    bool kineticTouchEnabled = false;
    bool wheelAccelerationEnabled = false;
    qreal wheelAccelerationStrength = 1.5;

    [[nodiscard]] SmoothScrollSettings normalized() const
    {
        SmoothScrollSettings result = *this;
        result.animationDuration = qMax(0, result.animationDuration);
        result.wheelDistanceFactor = qMax<qreal>(0.0, result.wheelDistanceFactor);
        result.minimumWheelStep = qMax(1, result.minimumWheelStep);
        result.maximumPendingDistance = qMax(0, result.maximumPendingDistance);
        result.wheelAccelerationStrength = qBound<qreal>(0.0, result.wheelAccelerationStrength, 3.0);
        return result;
    }
};

} // namespace smoothscroll
