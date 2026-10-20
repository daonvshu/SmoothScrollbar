#pragma once

#include <QObject>
#include <QPointer>
#include <QVariantAnimation>

#include <smoothscroll/smoothscrollbar_export.h>
#include <smoothscroll/smoothscrollsettings.h>

class QScrollBar;

namespace sscroll {

class SMOOTHSCROLLBAR_EXPORT ScrollAxisAnimator final : public QObject {
    Q_OBJECT

public:
    explicit ScrollAxisAnimator(QScrollBar* scrollBar, QObject* parent = nullptr);

    void setScrollBar(QScrollBar* scrollBar);
    Q_REQUIRED_RESULT QScrollBar* scrollBar() const noexcept;
    void setSettings(const SmoothScrollSettings& settings);
    Q_REQUIRED_RESULT bool scrollBy(qreal delta, qreal pendingDistanceMultiplier = 1.0,
                                int duration = -1);
    Q_REQUIRED_RESULT bool scrollTo(qreal position, int duration = -1);
    Q_REQUIRED_RESULT bool canScroll(qreal delta) const;
    Q_REQUIRED_RESULT bool isRunning() const noexcept;
    Q_REQUIRED_RESULT qreal targetPosition() const noexcept;
    void stop();

signals:
    void runningChanged(bool running);

private:
    Q_REQUIRED_RESULT qreal bounded(qreal position) const;
    void startAnimation(int duration);

    QPointer<QScrollBar> m_scrollBar;
    QVariantAnimation m_animation;
    SmoothScrollSettings m_settings;
    qreal m_targetPosition = 0.0;
    bool m_running = false;
    bool m_applyingValue = false;
};

} // namespace sscroll
