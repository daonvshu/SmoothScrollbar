#include "SmoothScrollbar.h"

#include <qevent.h>
#include <qqueue.h>
#include <qdatetime.h>
#include <qapplication.h>
#include <qmath.h>

SmoothScrollbar::SmoothScrollbar(QWidget* parent) 
    : QScrollBar(parent) 
{
    lastWheelEvent = 0;
    smoothMoveTimer = new QTimer(this);

    connect(smoothMoveTimer, &QTimer::timeout, this, &SmoothScrollbar::slotSmoothMove);
}

void SmoothScrollbar::wheelEvent(QWheelEvent* e) {
    static QQueue<qint64> scrollStamps;
    qint64 now = QDateTime::currentDateTime().toMSecsSinceEpoch();
    scrollStamps.enqueue(now);
    while (now - scrollStamps.front() > 500)
        scrollStamps.dequeue();
    double accerationRatio = qMin(scrollStamps.size() / 15.0, 1.0);

    if (!lastWheelEvent)
        lastWheelEvent = new QWheelEvent(*e);
    else
        *lastWheelEvent = *e;

    stepsTotal = 60 * 400 / 1000;
    double multiplier = 1.0;
    if (QApplication::keyboardModifiers() & Qt::ALT)
        multiplier *= 5.0;
    double delta = e->delta() * multiplier;
    delta += delta * 2.5 * accerationRatio;

    stepsLeftQueue.push_back(qMakePair(delta, stepsTotal));
    smoothMoveTimer->start(1000 / 60);
}

double SmoothScrollbar::subDelta(double delta, int stepsLeft)
{
    double m = stepsTotal / 2.0;
    double x = abs(stepsTotal - stepsLeft - m);
    return (cos(x * M_PI / m) + 1.0) / (2.0 * m) * delta;
}

void SmoothScrollbar::slotSmoothMove()
{
    double totalDelta = 0;

    for (QList< QPair<double, int> >::Iterator it = stepsLeftQueue.begin();
        it != stepsLeftQueue.end(); ++it)
    {
        totalDelta += subDelta(it->first, it->second);
        --(it->second);
    }
    while (!stepsLeftQueue.empty() && stepsLeftQueue.begin()->second == 0)
        stepsLeftQueue.pop_front();

    Qt::Orientation orientation = lastWheelEvent->orientation();

    QWheelEvent e(
        lastWheelEvent->pos(),
        lastWheelEvent->globalPos(),
        qRound(totalDelta),
        lastWheelEvent->buttons(),
        0,
        orientation
    );
    QScrollBar::wheelEvent(&e);
    if (stepsLeftQueue.empty()) {
        smoothMoveTimer->stop();
    }
}
