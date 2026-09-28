#include <smoothscroll/smoothscrollcontroller.h>

#include "qscrolleradapter_p.h"
#include "scrollaxisanimator_p.h"
#include "wheeldeltanormalizer_p.h"

#include <QAbstractScrollArea>
#include <QEvent>
#include <QElapsedTimer>
#include <QPointer>
#include <QScrollBar>
#include <QTimer>
#include <QWheelEvent>
#include <QWidget>

#include <algorithm>
#include <cmath>

namespace sscroll {

namespace {
constexpr int wheelIdleIntervalMs = 90;
constexpr int maximumWheelSampleGapMs = 180;
constexpr int minimumWheelSampleIntervalMs = 16;
constexpr qreal minimumMomentumSpeed = 550.0;
constexpr qreal maximumMomentumSpeed = 3000.0;
constexpr qreal momentumRampSteps = 2.0;
constexpr int maximumMomentumDurationMs = 900;
constexpr qreal pendingMomentumMultiplier = 2.0;
}

class SmoothScrollControllerPrivate {
public:
    struct WheelMotion {
        QElapsedTimer inputClock;
        QTimer idleTimer;
        QPointer<QScrollBar> scrollBar;
        qreal lastDistance = 0.0;
        qreal velocity = 0.0;
        qreal wheelSteps = 0.0;
        int consecutiveInputs = 0;
    };

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

        auto setupMomentum = [this, owner](WheelMotion& motion,
                                           ScrollAxisAnimator& animator) {
            motion.idleTimer.setSingleShot(true);
            auto* motionState = &motion;
            auto* axisAnimator = &animator;
            QObject::connect(&motion.idleTimer, &QTimer::timeout, owner,
                             [this, motionState, axisAnimator]() {
                const qreal velocity = motionState->velocity;
                const qreal wheelSteps = motionState->wheelSteps;
                const auto scrollBar = motionState->scrollBar;
                resetMotion(*motionState);
                if (!enabled || !settings.wheelMomentumEnabled
                    || settings.animationDuration == 0
                    || !scrollBar || axisAnimator->scrollBar() != scrollBar
                    || wheelSteps < settings.wheelMomentumMinimumSteps
                    || std::abs(velocity) < minimumMomentumSpeed) {
                    return;
                }

                const qreal speed = std::abs(velocity);
                const qreal speedRatio = std::clamp(
                    (speed - minimumMomentumSpeed)
                        / (maximumMomentumSpeed - minimumMomentumSpeed),
                    0.0, 1.0);
                const qreal stepRatio = std::clamp(
                    (wheelSteps - settings.wheelMomentumMinimumSteps + 1.0)
                        / (momentumRampSteps + 1.0),
                    0.0, 1.0);
                const qreal baseDuration = std::clamp<qreal>(
                    settings.animationDuration * 1.3, 150.0, maximumMomentumDurationMs);
                const int duration = qRound(baseDuration
                    + (maximumMomentumDurationMs - baseDuration) * speedRatio * stepRatio);
                const qreal pendingLimit = settings.maximumPendingDistance > 0
                    ? settings.maximumPendingDistance : 600.0;
                const qreal distance = std::copysign(
                    std::min(speed * duration / 3000.0,
                             pendingLimit * (1.0 + speedRatio)) * stepRatio,
                    velocity);
                static_cast<void>(axisAnimator->scrollBy(
                    distance, pendingMomentumMultiplier, duration));
            });
        };
        setupMomentum(horizontalMotion, horizontal);
        setupMomentum(verticalMotion, vertical);
    }

    void resetMotion(WheelMotion& motion)
    {
        motion.idleTimer.stop();
        motion.inputClock.invalidate();
        motion.scrollBar = nullptr;
        motion.lastDistance = 0.0;
        motion.velocity = 0.0;
        motion.wheelSteps = 0.0;
        motion.consecutiveInputs = 0;
    }

    void recordWheelInput(WheelMotion& motion, QScrollBar* scrollBar,
                          qreal distance, qreal wheelSteps)
    {
        const qint64 elapsed = motion.inputClock.isValid()
            ? motion.inputClock.elapsed() : 0;
        if (scrollBar != motion.scrollBar || elapsed > maximumWheelSampleGapMs
            || distance * motion.lastDistance <= 0.0) {
            motion.velocity = 0.0;
            motion.wheelSteps = 0.0;
            motion.consecutiveInputs = 0;
        } else {
            const qreal instantaneous = distance * 1000.0
                / std::max<qint64>(minimumWheelSampleIntervalMs, elapsed);
            motion.velocity = motion.consecutiveInputs == 0
                ? instantaneous : 0.5 * motion.velocity + 0.5 * instantaneous;
            ++motion.consecutiveInputs;
        }
        motion.lastDistance = distance;
        motion.wheelSteps += wheelSteps;
        motion.scrollBar = scrollBar;
        motion.inputClock.start();
        motion.idleTimer.start(wheelIdleIntervalMs);
    }

    void applySettings(const SmoothScrollSettings& newSettings)
    {
        resetMotion(horizontalMotion);
        resetMotion(verticalMotion);
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
            resetMotion(horizontalMotion);
            resetMotion(verticalMotion);
            horizontalInput.invalidate();
            verticalInput.invalidate();
            return false;
        }
        if (delta.usesPixelDelta) {
            resetMotion(horizontalMotion);
            resetMotion(verticalMotion);
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
            const bool moved = horizontal.scrollBy(input.first, input.second);
            if (moved && settings.wheelMomentumEnabled && !delta.usesPixelDelta) {
                const int angle = event->angleDelta().x() != 0
                    ? event->angleDelta().x() : event->angleDelta().y();
                recordWheelInput(horizontalMotion, horizontal.scrollBar(),
                                 input.first, std::abs(angle) / 120.0);
            } else {
                resetMotion(horizontalMotion);
            }
            handled = moved || handled;
        }
        if (!qFuzzyIsNull(delta.valueDelta.y())) {
            const auto input = accelerated(delta.valueDelta.y(), verticalInput,
                                           lastVerticalDistance);
            const bool moved = vertical.scrollBy(input.first, input.second);
            if (moved && settings.wheelMomentumEnabled && !delta.usesPixelDelta) {
                recordWheelInput(verticalMotion, vertical.scrollBar(), input.first,
                                 std::abs(event->angleDelta().y()) / 120.0);
            } else {
                resetMotion(verticalMotion);
            }
            handled = moved || handled;
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
    WheelMotion horizontalMotion;
    WheelMotion verticalMotion;
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
    d->resetMotion(d->horizontalMotion);
    d->resetMotion(d->verticalMotion);
    const bool horizontalStarted = d->horizontal.scrollTo(position.x(), duration);
    const bool verticalStarted = d->vertical.scrollTo(position.y(), duration);
    Q_UNUSED(horizontalStarted)
    Q_UNUSED(verticalStarted)
}

void SmoothScrollController::stop()
{
    d->resetMotion(d->horizontalMotion);
    d->resetMotion(d->verticalMotion);
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

} // namespace sscroll
