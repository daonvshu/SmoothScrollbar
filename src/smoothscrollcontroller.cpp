#include <smoothscroll/smoothscrollcontroller.h>

#include "qscrolleradapter_p.h"
#include "scrollaxisanimator_p.h"
#include "wheeldeltanormalizer_p.h"

#include <QAbstractScrollArea>
#include <QEvent>
#include <QElapsedTimer>
#include <QPointer>
#include <QScrollBar>
#include <QWheelEvent>
#include <QWidget>

namespace smoothscroll {

class SmoothScrollControllerPrivate {
public:
    SmoothScrollControllerPrivate(SmoothScrollController* owner,
                                  QAbstractScrollArea* area)
        : q(owner)
        , scrollArea(area)
        , horizontal(area ? area->horizontalScrollBar() : nullptr, owner)
        , vertical(area ? area->verticalScrollBar() : nullptr, owner)
        , scroller(area)
    {
        auto updateRunning = [this]() {
            const bool nowRunning = horizontal.isRunning() || vertical.isRunning();
            if (nowRunning != running) {
                running = nowRunning;
                emit q->scrollingChanged(running);
            }
        };
        QObject::connect(&horizontal, &ScrollAxisAnimator::runningChanged,
                         owner, updateRunning);
        QObject::connect(&vertical, &ScrollAxisAnimator::runningChanged,
                         owner, updateRunning);
    }

    void applySettings(const SmoothScrollSettings& newSettings)
    {
        horizontalInput.invalidate();
        verticalInput.invalidate();
        settings = newSettings.normalized();
        horizontal.setSettings(settings);
        vertical.setSettings(settings);
        scroller.setEnabled(enabled && settings.kineticTouchEnabled);
        prioritizeViewportFilter();
    }

    void prioritizeViewportFilter()
    {
        if (!scrollArea || !scrollArea->viewport()) {
            return;
        }
        scrollArea->viewport()->removeEventFilter(q);
        scrollArea->viewport()->installEventFilter(q);
    }

    void ensureScrollBars()
    {
        if (!scrollArea) {
            return;
        }

        auto rebind = [this](ScrollAxisAnimator& animator, QScrollBar* scrollBar) {
            if (animator.scrollBar() == scrollBar) {
                return;
            }
            if (animator.scrollBar()) {
                animator.scrollBar()->removeEventFilter(q);
            }
            animator.setScrollBar(scrollBar);
            animator.setSettings(settings);
            if (scrollBar) {
                scrollBar->installEventFilter(q);
            }
        };
        rebind(horizontal, scrollArea->horizontalScrollBar());
        rebind(vertical, scrollArea->verticalScrollBar());
    }

    bool handleWheel(QWheelEvent* event)
    {
        if (!enabled || !scrollArea) {
            return false;
        }

        ensureScrollBars();

        const NormalizedWheelDelta delta = WheelDeltaNormalizer::normalize(
            *event, *scrollArea->horizontalScrollBar(),
            *scrollArea->verticalScrollBar(), settings);
        if (delta.shouldPreserveNativeHandling) {
            horizontalInput.invalidate();
            verticalInput.invalidate();
            return false;
        }

        const auto accelerated = [this, &delta](qreal distance, QElapsedTimer& timer,
                                                qreal& lastDistance) {
            if (qFuzzyIsNull(distance)) {
                return qMakePair(distance, 1.0);
            }
            qreal multiplier = 1.0;
            if (!delta.usesPixelDelta && settings.wheelAccelerationEnabled) {
                if (timer.isValid() && distance * lastDistance > 0.0) {
                    const qreal speed = qMax<qreal>(0.0, 1.0 - timer.elapsed() / 180.0);
                    multiplier += speed * settings.wheelAccelerationStrength;
                }
                timer.start();
                lastDistance = distance;
            } else {
                timer.invalidate();
            }
            return qMakePair(distance * multiplier, multiplier);
        };
        bool handled = false;
        if (!qFuzzyIsNull(delta.valueDelta.x())) {
            const auto input = accelerated(delta.valueDelta.x(), horizontalInput,
                                           lastHorizontalDistance);
            handled = horizontal.scrollBy(input.first, input.second) || handled;
        }
        if (!qFuzzyIsNull(delta.valueDelta.y())) {
            const auto input = accelerated(delta.valueDelta.y(), verticalInput,
                                           lastVerticalDistance);
            handled = vertical.scrollBy(input.first, input.second) || handled;
        }

        if (handled || settings.boundaryPolicy == BoundaryPolicy::Consume) {
            event->accept();
            return true;
        }

        event->ignore();
        return false;
    }

    SmoothScrollController* q;
    QPointer<QAbstractScrollArea> scrollArea;
    ScrollAxisAnimator horizontal;
    ScrollAxisAnimator vertical;
    QScrollerAdapter scroller;
    SmoothScrollSettings settings;
    QElapsedTimer horizontalInput;
    QElapsedTimer verticalInput;
    qreal lastHorizontalDistance = 0.0;
    qreal lastVerticalDistance = 0.0;
    bool enabled = true;
    bool running = false;
};

SmoothScrollController::SmoothScrollController(QAbstractScrollArea* scrollArea,
                                               QObject* parent)
    : QObject(parent ? parent : scrollArea)
    , d(new SmoothScrollControllerPrivate(this, scrollArea))
{
    Q_ASSERT(scrollArea);
    if (!scrollArea) {
        d->enabled = false;
        return;
    }

    scrollArea->installEventFilter(this);
    scrollArea->viewport()->installEventFilter(this);
    scrollArea->horizontalScrollBar()->installEventFilter(this);
    scrollArea->verticalScrollBar()->installEventFilter(this);
    d->applySettings({});
}

SmoothScrollController::~SmoothScrollController() = default;

QAbstractScrollArea* SmoothScrollController::scrollArea() const noexcept
{
    return d->scrollArea;
}

SmoothScrollSettings SmoothScrollController::settings() const
{
    return d->settings;
}

void SmoothScrollController::setSettings(const SmoothScrollSettings& settings)
{
    d->applySettings(settings);
}

bool SmoothScrollController::isEnabled() const noexcept
{
    return d->enabled;
}

void SmoothScrollController::setEnabled(bool enabled)
{
    if (d->enabled == enabled) {
        return;
    }
    d->enabled = enabled;
    if (!enabled) {
        stop();
    }
    d->scroller.setEnabled(enabled && d->settings.kineticTouchEnabled);
    d->prioritizeViewportFilter();
    emit enabledChanged(enabled);
}

void SmoothScrollController::scrollTo(const QPoint& position, int duration)
{
    if (!d->enabled) {
        return;
    }
    d->ensureScrollBars();
    const bool horizontalStarted = d->horizontal.scrollTo(position.x(), duration);
    const bool verticalStarted = d->vertical.scrollTo(position.y(), duration);
    Q_UNUSED(horizontalStarted)
    Q_UNUSED(verticalStarted)
}

void SmoothScrollController::stop()
{
    d->horizontalInput.invalidate();
    d->verticalInput.invalidate();
    d->horizontal.stop();
    d->vertical.stop();
}

bool SmoothScrollController::eventFilter(QObject* watched, QEvent* event)
{
    if (!d->scrollArea) {
        return QObject::eventFilter(watched, event);
    }
    d->ensureScrollBars();
    switch (event->type()) {
    case QEvent::Wheel:
        if (watched != d->scrollArea
            && watched != d->scrollArea->viewport()
            && watched != d->horizontal.scrollBar()
            && watched != d->vertical.scrollBar()) {
            return false;
        }
        return d->handleWheel(static_cast<QWheelEvent*>(event));
    case QEvent::MouseButtonPress:
    case QEvent::MouseButtonDblClick:
        stop();
        break;
    case QEvent::Hide:
    case QEvent::EnabledChange:
        stop();
        break;
    default:
        break;
    }
    return QObject::eventFilter(watched, event);
}

} // namespace smoothscroll
