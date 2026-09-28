#include "scrollaxisanimator_p.h"

#include <QScrollBar>

#include <algorithm>
#include <cmath>

namespace sscroll {

ScrollAxisAnimator::ScrollAxisAnimator(QScrollBar* scrollBar, QObject* parent)
    : QObject(parent)
{
    connect(&m_animation, &QVariantAnimation::valueChanged, this,
            [this](const QVariant& value) {
                if (m_scrollBar) {
                    m_applyingValue = true;
                    m_scrollBar->setValue(qRound(value.toReal()));
                    m_applyingValue = false;
                }
            });
    connect(&m_animation, &QVariantAnimation::finished, this, [this]() {
        if (m_scrollBar) {
            m_applyingValue = true;
            m_scrollBar->setValue(qRound(m_targetPosition));
            m_applyingValue = false;
        }
        if (m_running) {
            m_running = false;
            emit runningChanged(false);
        }
    });

    setScrollBar(scrollBar);
}

void ScrollAxisAnimator::setScrollBar(QScrollBar* scrollBar)
{
    if (m_scrollBar == scrollBar) {
        return;
    }

    stop();
    if (m_scrollBar) {
        disconnect(m_scrollBar, nullptr, this, nullptr);
    }
    m_scrollBar = scrollBar;
    m_targetPosition = scrollBar ? scrollBar->value() : 0.0;

    if (!m_scrollBar) {
        return;
    }
    connect(m_scrollBar, &QScrollBar::rangeChanged, this, [this]() {
        m_targetPosition = bounded(m_targetPosition);
        if (m_animation.state() == QAbstractAnimation::Running) {
            startAnimation(m_animation.duration());
        }
    });
    connect(m_scrollBar, &QScrollBar::valueChanged, this, [this](int value) {
        if (m_applyingValue) {
            return;
        }
        if (m_animation.state() == QAbstractAnimation::Running) {
            m_animation.stop();
            if (m_running) {
                m_running = false;
                emit runningChanged(false);
            }
        }
        m_targetPosition = value;
    });
    connect(m_scrollBar, &QObject::destroyed, this, [this]() {
        m_animation.stop();
        m_scrollBar = nullptr;
        if (m_running) {
            m_running = false;
            emit runningChanged(false);
        }
    });
}

QScrollBar* ScrollAxisAnimator::scrollBar() const noexcept
{
    return m_scrollBar;
}

void ScrollAxisAnimator::setSettings(const SmoothScrollSettings& settings)
{
    m_settings = settings.normalized();
    m_animation.setEasingCurve(m_settings.easingCurve);
}

bool ScrollAxisAnimator::scrollBy(qreal delta, qreal pendingDistanceMultiplier,
                                  int duration)
{
    if (!m_scrollBar || qFuzzyIsNull(delta) || !canScroll(delta)) {
        return false;
    }

    const qreal current = m_scrollBar->value();
    if (m_animation.state() != QAbstractAnimation::Running) {
        m_targetPosition = current;
    }
    const qreal outstanding = m_targetPosition - current;
    if (!qFuzzyIsNull(outstanding) && std::signbit(outstanding) != std::signbit(delta)) {
        m_targetPosition = current;
    }

    m_targetPosition = bounded(m_targetPosition + delta);
    if (m_settings.maximumPendingDistance > 0) {
        const qreal limit = m_settings.maximumPendingDistance
            * qMax<qreal>(1.0, pendingDistanceMultiplier);
        m_targetPosition = std::clamp(m_targetPosition, current - limit, current + limit);
        m_targetPosition = bounded(m_targetPosition);
    }

    startAnimation(duration >= 0 ? duration : m_settings.animationDuration);
    return true;
}

bool ScrollAxisAnimator::scrollTo(qreal position, int duration)
{
    if (!m_scrollBar) {
        return false;
    }

    m_targetPosition = bounded(position);
    if (qFuzzyCompare(m_targetPosition + 1.0, qreal(m_scrollBar->value()) + 1.0)) {
        stop();
        return false;
    }

    startAnimation(duration >= 0 ? duration : m_settings.animationDuration);
    return true;
}

bool ScrollAxisAnimator::canScroll(qreal delta) const
{
    if (!m_scrollBar || qFuzzyIsNull(delta)) {
        return false;
    }

    const qreal reference = m_animation.state() == QAbstractAnimation::Running
        ? m_targetPosition
        : m_scrollBar->value();
    return delta < 0.0 ? reference > m_scrollBar->minimum()
                       : reference < m_scrollBar->maximum();
}

bool ScrollAxisAnimator::isRunning() const noexcept
{
    return m_running;
}

qreal ScrollAxisAnimator::targetPosition() const noexcept
{
    return m_targetPosition;
}

void ScrollAxisAnimator::stop()
{
    m_animation.stop();
    if (m_scrollBar) {
        m_targetPosition = m_scrollBar->value();
    }
    if (m_running) {
        m_running = false;
        emit runningChanged(false);
    }
}

qreal ScrollAxisAnimator::bounded(qreal position) const
{
    if (!m_scrollBar) {
        return position;
    }
    return std::clamp(position,
                      qreal(m_scrollBar->minimum()),
                      qreal(m_scrollBar->maximum()));
}

void ScrollAxisAnimator::startAnimation(int duration)
{
    if (!m_scrollBar) {
        return;
    }

    m_animation.stop();
    if (duration <= 0) {
        m_applyingValue = true;
        m_scrollBar->setValue(qRound(m_targetPosition));
        m_applyingValue = false;
        if (m_running) {
            m_running = false;
            emit runningChanged(false);
        }
        return;
    }

    m_animation.setDuration(duration);
    m_animation.setEasingCurve(m_settings.easingCurve);
    m_animation.setStartValue(qreal(m_scrollBar->value()));
    m_animation.setEndValue(m_targetPosition);
    if (!m_running) {
        m_running = true;
        emit runningChanged(true);
    }
    m_animation.start();
}

} // namespace sscroll
