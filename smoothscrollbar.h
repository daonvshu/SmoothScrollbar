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
    QList<QPair<QPoint, int>> stepsLeftQueue;

private:
    QPointF subDelta(QPoint delta, int stepsLeft) const;

private slots:
    void slotSmoothMove();
};

