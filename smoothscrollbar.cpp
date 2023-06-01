#include "smoothscrollbar.h"

#include <qevent.h>
#include <qqueue.h>
#include <qdatetime.h>
#include <qapplication.h>
#include <qmath.h>

SmoothScrollbar::SmoothScrollbar(QWidget* parent) 
    : QScrollBar(parent)
    , lastWheelEvent(nullptr)
    , stepsTotal(0)
{
    smoothMoveTimer = new QTimer(this);
    connect(smoothMoveTimer, &QTimer::timeout, this, &SmoothScrollbar::slotSmoothMove);
}

void SmoothScrollbar::wheelEvent(QWheelEvent* e) {
    static QQueue<qint64> scrollStamps;
    qint64 now = QDateTime::currentDateTime().toMSecsSinceEpoch();
    scrollStamps.enqueue(now);
    while (now - scrollStamps.front() > 500)
        scrollStamps.dequeue();
    double accRatio = qMin(scrollStamps.size() / 15.0, 1.0);

    if (!lastWheelEvent)
        lastWheelEvent = new QWheelEvent(*e);
    else
        *lastWheelEvent = *e;

    stepsTotal = 60 * 400 / 1000;
    double multiplier = 1.0;
    if (QApplication::keyboardModifiers() & Qt::ALT)
        multiplier *= 5.0;
    auto delta = e->angleDelta() * multiplier;
    delta += delta * 2.5 * accRatio;

    stepsLeftQueue.push_back(qMakePair(delta, stepsTotal));
    smoothMoveTimer->start(1000 / 60);

    bool isTop = delta.y() > 0 && value() == minimum();
    bool isBottom = delta.y() < 0 && value() == maximum();
    if (!isTop && !isBottom) {
        e->accept();
    }
}

QPointF SmoothScrollbar::subDelta(QPoint delta, int stepsLeft) const
{
    double m = stepsTotal / 2.0;
    double x = abs(stepsTotal - stepsLeft - m);
    return (cos(x * M_PI / m) + 1.0) / (2.0 * m) * delta;
}

void SmoothScrollbar::slotSmoothMove()
{
    QPointF totalDelta;

    for (auto & it : stepsLeftQueue) {
        totalDelta += subDelta(it.first, it.second);
        --(it.second);
    }
    while (!stepsLeftQueue.empty() && stepsLeftQueue.begin()->second == 0)
        stepsLeftQueue.pop_front();

    QWheelEvent e(
#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
        lastWheelEvent->position(),
        lastWheelEvent->globalPosition(),
#else
        lastWheelEvent->pos(),
        lastWheelEvent->globalPos(),
#endif
        QPoint(),
        totalDelta.toPoint(),
        lastWheelEvent->buttons(),
        lastWheelEvent->modifiers(),
        lastWheelEvent->phase(),
        lastWheelEvent->inverted()
    );
    QScrollBar::wheelEvent(&e);
    if (stepsLeftQueue.empty()) {
        smoothMoveTimer->stop();
    }
}
