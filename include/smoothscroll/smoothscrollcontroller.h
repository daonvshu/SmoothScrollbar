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

    Q_REQUIRED_RESULT QAbstractScrollArea* scrollArea() const noexcept;
    Q_REQUIRED_RESULT SmoothScrollSettings settings() const;
    void setSettings(const SmoothScrollSettings& settings);

    Q_REQUIRED_RESULT bool isEnabled() const noexcept;
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

