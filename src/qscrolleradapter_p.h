#pragma once

#include <QPointer>

class QAbstractScrollArea;

namespace smoothscroll {

class QScrollerAdapter final {
public:
    explicit QScrollerAdapter(QAbstractScrollArea* scrollArea);
    ~QScrollerAdapter();

    void setEnabled(bool enabled);
    [[nodiscard]] bool isEnabled() const noexcept;

private:
    QPointer<QAbstractScrollArea> m_scrollArea;
    bool m_ownsGesture = false;
};

} // namespace smoothscroll

