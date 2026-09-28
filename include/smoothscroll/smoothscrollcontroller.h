#pragma once

#include <QObject>
#include <QPoint>
#include <QScopedPointer>

#include <smoothscroll/smoothscrollbar_export.h>
#include <smoothscroll/smoothscrollsettings.h>

class QAbstractScrollArea;

namespace sscroll {

class SmoothScrollControllerPrivate;

class SMOOTHSCROLLBAR_EXPORT SmoothScrollController final : public QObject {
    Q_OBJECT

public:
    explicit SmoothScrollController(QAbstractScrollArea* scrollArea,
                                    QObject* parent = nullptr);
    ~SmoothScrollController() override;

    [[nodiscard]] QAbstractScrollArea* scrollArea() const noexcept;
    [[nodiscard]] SmoothScrollSettings settings() const;
    void setSettings(const SmoothScrollSettings& settings);

    [[nodiscard]] bool isEnabled() const noexcept;
    void setEnabled(bool enabled);

    void scrollTo(const QPoint& position, int duration = -1);
    void stop();

signals:
    void enabledChanged(bool enabled);
    void scrollingChanged(bool scrolling);

protected:
    bool eventFilter(QObject* watched, QEvent* event) override;

private:
    QScopedPointer<SmoothScrollControllerPrivate> d;
};

} // namespace sscroll

