#include <smoothscroll/smoothscrollbar.h>

#include "scrollaxisanimator_p.h"
#include "wheeldeltanormalizer_p.h"

#include <QMouseEvent>
#include <QWheelEvent>

namespace smoothscroll {

class SmoothScrollbarPrivate {
public:
    explicit SmoothScrollbarPrivate(SmoothScrollbar* owner)
        : animator(owner, owner)
    {
        animator.setSettings(settings);
    }

    SmoothScrollSettings settings;
    ScrollAxisAnimator animator;
};

} // namespace smoothscroll

SmoothScrollbar::SmoothScrollbar(QWidget* parent)
    : QScrollBar(parent)
    , d(new smoothscroll::SmoothScrollbarPrivate(this))
{
}

SmoothScrollbar::SmoothScrollbar(Qt::Orientation orientation, QWidget* parent)
    : QScrollBar(orientation, parent)
    , d(new smoothscroll::SmoothScrollbarPrivate(this))
{
}

SmoothScrollbar::~SmoothScrollbar() = default;

smoothscroll::SmoothScrollSettings SmoothScrollbar::settings() const
{
    return d->settings;
}

void SmoothScrollbar::setSettings(const smoothscroll::SmoothScrollSettings& settings)
{
    d->settings = settings.normalized();
    d->animator.setSettings(d->settings);
}

void SmoothScrollbar::wheelEvent(QWheelEvent* event)
{
    if (!event->pixelDelta().isNull()
        && d->settings.pixelDeltaMode == smoothscroll::PixelDeltaMode::Native) {
        d->animator.stop();
        QScrollBar::wheelEvent(event);
        return;
    }
    if (d->settings.preserveControlWheel
        && event->modifiers().testFlag(Qt::ControlModifier)) {
        event->ignore();
        return;
    }

    QPoint delta = !event->pixelDelta().isNull()
        ? event->pixelDelta()
        : event->angleDelta();
    int axisDelta = orientation() == Qt::Horizontal ? delta.x() : delta.y();
    if (orientation() == Qt::Horizontal && axisDelta == 0) {
        axisDelta = delta.y();
    }

    qreal valueDelta = 0.0;
    if (!event->pixelDelta().isNull()) {
        valueDelta = -axisDelta * d->settings.wheelDistanceFactor;
    } else {
        valueDelta = smoothscroll::WheelDeltaNormalizer::angleToValueDelta(
            axisDelta, *this, d->settings.wheelDistanceFactor,
            d->settings.minimumWheelStep);
    }

    if (d->animator.scrollBy(valueDelta)
        || d->settings.boundaryPolicy == smoothscroll::BoundaryPolicy::Consume) {
        event->accept();
    } else {
        event->ignore();
    }
}

void SmoothScrollbar::mousePressEvent(QMouseEvent* event)
{
    d->animator.stop();
    QScrollBar::mousePressEvent(event);
}

void SmoothScrollbar::mouseMoveEvent(QMouseEvent* event)
{
    d->animator.stop();
    QScrollBar::mouseMoveEvent(event);
}

void SmoothScrollbar::mouseReleaseEvent(QMouseEvent* event)
{
    d->animator.stop();
    QScrollBar::mouseReleaseEvent(event);
}
