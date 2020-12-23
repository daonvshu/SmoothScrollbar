#pragma once

#include <qscrollbar.h>
#include <qtimer.h>

class SmoothScrollbar : public QScrollBar {
public:
    explicit SmoothScrollbar(QWidget* parent = nullptr);

protected:
    void wheelEvent(QWheelEvent* e) override;

private:
    QTimer* smoothMoveTimer;
    QWheelEvent* lastWheelEvent;

    int stepsTotal;
    QList<QPair<double, int>> stepsLeftQueue;

private:
    double subDelta(double delta, int stepsLeft);

private slots:
    void slotSmoothMove();
};

