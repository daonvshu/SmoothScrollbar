#pragma once

#include <QPointF>

#include <smoothscroll/smoothscrollbar_export.h>
#include <smoothscroll/smoothscrollsettings.h>

class QScrollBar;
class QWheelEvent;

namespace sscroll {

struct NormalizedWheelDelta {
    QPointF valueDelta;
    bool usesPixelDelta = false;
    bool shouldPreserveNativeHandling = false;
};

class SMOOTHSCROLLBAR_EXPORT WheelDeltaNormalizer final {
public:
    [[nodiscard]] static NormalizedWheelDelta normalize(
        const QWheelEvent& event,
        const QScrollBar& horizontalScrollBar,
        const QScrollBar& verticalScrollBar,
        const SmoothScrollSettings& settings);

    [[nodiscard]] static qreal angleToValueDelta(
        int angleDelta,
        const QScrollBar& scrollBar,
        qreal distanceFactor,
        int minimumWheelStep = 1,
        int wheelStep = 0);
};

} // namespace sscroll
