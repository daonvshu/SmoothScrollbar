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
    [[nodiscard]] QScrollBar* scrollBar() const noexcept;
    void setSettings(const SmoothScrollSettings& settings);
    [[nodiscard]] bool scrollBy(qreal delta, qreal pendingDistanceMultiplier = 1.0,
                                int duration = -1);
    [[nodiscard]] bool scrollTo(qreal position, int duration = -1);
    [[nodiscard]] bool canScroll(qreal delta) const;
    [[nodiscard]] bool isRunning() const noexcept;
    [[nodiscard]] qreal targetPosition() const noexcept;
    void stop();

signals:
    void runningChanged(bool running);

private:
    [[nodiscard]] qreal bounded(qreal position) const;
    void startAnimation(int duration);

    QPointer<QScrollBar> m_scrollBar;
    QVariantAnimation m_animation;
    SmoothScrollSettings m_settings;
    qreal m_targetPosition = 0.0;
    bool m_running = false;
    bool m_applyingValue = false;
};

} // namespace sscroll
