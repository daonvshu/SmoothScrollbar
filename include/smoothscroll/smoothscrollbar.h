#pragma once

#include <QScopedPointer>
#include <QScrollBar>

#include <smoothscroll/smoothscrollbar_export.h>
#include <smoothscroll/smoothscrollsettings.h>

class QMouseEvent;
class QWheelEvent;

namespace smoothscroll {
class SmoothScrollbarPrivate;
}

class SMOOTHSCROLLBAR_EXPORT SmoothScrollbar : public QScrollBar {
    Q_OBJECT

public:
    explicit SmoothScrollbar(QWidget* parent = nullptr);
    explicit SmoothScrollbar(Qt::Orientation orientation, QWidget* parent = nullptr);
    ~SmoothScrollbar() override;

    [[nodiscard]] smoothscroll::SmoothScrollSettings settings() const;
    void setSettings(const smoothscroll::SmoothScrollSettings& settings);

protected:
    void wheelEvent(QWheelEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;

private:
    QScopedPointer<smoothscroll::SmoothScrollbarPrivate> d;
};

