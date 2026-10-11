#include "qscrolleradapter_p.h"

#include <QAbstractScrollArea>
#include <QScroller>
#include <QWidget>

namespace sscroll {

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
    if (!enabled) {
        if (m_ownsGesture && m_viewport) {
            QScroller::ungrabGesture(m_viewport);
        }
        m_ownsGesture = false;
        m_viewport = nullptr;
        return;
    }

    QWidget* viewport = m_scrollArea ? m_scrollArea->viewport() : nullptr;
    if (m_viewport != viewport) {
        setEnabled(false);
        m_viewport = viewport;
    }
    if (m_viewport && !m_ownsGesture) {
        if (QScroller::grabbedGesture(m_viewport) == Qt::GestureType(0)) {
            QScroller::grabGesture(m_viewport, QScroller::TouchGesture);
            m_ownsGesture = true;
        }
    }
}

bool QScrollerAdapter::isEnabled() const noexcept
{
    return m_ownsGesture;
}

} // namespace sscroll
