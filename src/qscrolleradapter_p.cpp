#include "qscrolleradapter_p.h"

#include <QAbstractScrollArea>
#include <QScroller>
#include <QWidget>

namespace smoothscroll {

QScrollerAdapter::QScrollerAdapter(QAbstractScrollArea* scrollArea)
    : m_scrollArea(scrollArea)
{
}

QScrollerAdapter::~QScrollerAdapter()
{
    setEnabled(false);
}

void QScrollerAdapter::setEnabled(bool enabled)
{
    if (!m_scrollArea || !m_scrollArea->viewport()) {
        m_ownsGesture = false;
        return;
    }

    QObject* viewport = m_scrollArea->viewport();
    if (enabled && !m_ownsGesture) {
        if (QScroller::grabbedGesture(viewport) == Qt::GestureType(0)) {
            QScroller::grabGesture(viewport, QScroller::TouchGesture);
            m_ownsGesture = true;
        }
    } else if (!enabled && m_ownsGesture) {
        QScroller::ungrabGesture(viewport);
        m_ownsGesture = false;
    }
}

bool QScrollerAdapter::isEnabled() const noexcept
{
    return m_ownsGesture;
}

} // namespace smoothscroll
