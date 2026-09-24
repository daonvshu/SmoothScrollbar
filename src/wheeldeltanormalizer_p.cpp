#include "wheeldeltanormalizer_p.h"

#include <QApplication>
#include <QScrollBar>
#include <QWheelEvent>

#include <algorithm>

namespace smoothscroll {

NormalizedWheelDelta WheelDeltaNormalizer::normalize(
    const QWheelEvent& event,
    const QScrollBar& horizontalScrollBar,
    const QScrollBar& verticalScrollBar,
    const SmoothScrollSettings& settings)
{
    NormalizedWheelDelta result;
    const SmoothScrollSettings effective = settings.normalized();

    if (effective.preserveControlWheel
        && event.modifiers().testFlag(Qt::ControlModifier)) {
        result.shouldPreserveNativeHandling = true;
        return result;
    }

    if (!event.pixelDelta().isNull()) {
        result.usesPixelDelta = true;
        if (effective.pixelDeltaMode == PixelDeltaMode::Native) {
            result.shouldPreserveNativeHandling = true;
            return result;
        }
        result.valueDelta = -QPointF(event.pixelDelta()) * effective.wheelDistanceFactor;
        return result;
    }

    QPoint angleDelta = event.angleDelta();
    const bool shouldUseHorizontalAxis = effective.shiftSwapsAxes
        && (event.modifiers().testFlag(Qt::ShiftModifier)
            || (verticalScrollBar.minimum() == verticalScrollBar.maximum()
                && horizontalScrollBar.minimum() != horizontalScrollBar.maximum()));
    if (shouldUseHorizontalAxis
        && angleDelta.x() == 0
        && angleDelta.y() != 0) {
        angleDelta.setX(angleDelta.y());
        angleDelta.setY(0);
    }

    result.valueDelta.setX(angleToValueDelta(
        angleDelta.x(), horizontalScrollBar, effective.wheelDistanceFactor,
        effective.minimumWheelStep));

    result.valueDelta.setY(angleToValueDelta(
        angleDelta.y(), verticalScrollBar, effective.wheelDistanceFactor,
        effective.minimumWheelStep));
    return result;
}

qreal WheelDeltaNormalizer::angleToValueDelta(
    int angleDelta,
    const QScrollBar& scrollBar,
    qreal distanceFactor,
    int minimumWheelStep)
{
    if (angleDelta == 0) {
        return 0.0;
    }

    const int lines = std::max(1, QApplication::wheelScrollLines());
    const int step = std::max(minimumWheelStep, scrollBar.singleStep());
    return -(qreal(angleDelta) / 120.0) * lines * step * distanceFactor;
}

} // namespace smoothscroll
