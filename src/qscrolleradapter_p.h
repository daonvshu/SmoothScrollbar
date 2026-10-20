#pragma once

#include <QPointer>

class QAbstractScrollArea;
class QWidget;

namespace sscroll {

class QScrollerAdapter final {
public:
    explicit QScrollerAdapter(QAbstractScrollArea* scrollArea);
    ~QScrollerAdapter();

    void setEnabled(bool enabled);
    Q_REQUIRED_RESULT bool isEnabled() const noexcept;

private:
    QPointer<QAbstractScrollArea> m_scrollArea;
    QPointer<QWidget> m_viewport;
    bool m_ownsGesture = false;
};

} // namespace sscroll

