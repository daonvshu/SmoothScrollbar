#include "scrollaxisanimator_p.h"
#include "wheeldeltanormalizer_p.h"

#include <smoothscroll/smoothscrollcontroller.h>

#include <QApplication>
#include <QAbstractTableModel>
#include <QScrollArea>
#include <QScrollBar>
#include <QSignalSpy>
#include <QTableView>
#include <QTest>
#include <QWheelEvent>

using namespace sscroll;

class SmoothScrollbarTest : public QObject {
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    void settingsAreNormalized();
    void angleDeltaUsesSystemStep();
    void angleDeltaUsesConfiguredStep();
    void pixelDeltaDefaultsToNativeHandling();
    void shiftMovesVerticalDeltaToHorizontalAxis();
    void horizontalOnlyRangeUsesVerticalWheelInput();
    void animatorAccumulatesAndClamps();
    void animatorExpandsPendingLimitForAcceleration();
    void animatorReversesFromCurrentPosition();
    void animatorResynchronizesAfterExternalChange();
    void externalChangeCancelsAnimation();
    void controllerAnimatesWheelInput();
    void controllerAcceleratesRapidWheelInput();
    void controllerAnimatesTableViewPixelInput();
    void controllerKeepsWheelFilterPriority();
    void controllerTracksReplacementScrollBar();

private:
    int originalWheelScrollLines = 0;
};

class TestTableModel final : public QAbstractTableModel {
public:
    using QAbstractTableModel::QAbstractTableModel;

    int rowCount(const QModelIndex& parent = QModelIndex()) const override
    {
        return parent.isValid() ? 0 : 100000;
    }

    int columnCount(const QModelIndex& parent = QModelIndex()) const override
    {
        return parent.isValid() ? 0 : 3;
    }

    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override
    {
        return index.isValid() && role == Qt::DisplayRole
            ? QVariant(index.row())
            : QVariant();
    }
};

class WheelEatingFilter final : public QObject {
public:
    int wheelEventCount = 0;

protected:
    bool eventFilter(QObject* watched, QEvent* event) override
    {
        Q_UNUSED(watched)
        if (event->type() == QEvent::Wheel) {
            ++wheelEventCount;
            return true;
        }
        return false;
    }
};

static QWheelEvent makeWheelEvent(const QPoint& pixelDelta,
                                  const QPoint& angleDelta,
                                  Qt::KeyboardModifiers modifiers = Qt::NoModifier)
{
    return QWheelEvent(QPointF(10, 10), QPointF(10, 10),
                       pixelDelta, angleDelta, Qt::NoButton, modifiers,
                       Qt::NoScrollPhase, false);
}

void SmoothScrollbarTest::initTestCase()
{
    originalWheelScrollLines = QApplication::wheelScrollLines();
    QApplication::setWheelScrollLines(3);
}

void SmoothScrollbarTest::cleanupTestCase()
{
    QApplication::setWheelScrollLines(originalWheelScrollLines);
}

void SmoothScrollbarTest::settingsAreNormalized()
{
    SmoothScrollSettings settings;
    settings.animationDuration = -10;
    settings.wheelDistanceFactor = -2.0;
    settings.wheelStep = -5;
    settings.minimumWheelStep = 0;
    settings.maximumPendingDistance = -1;
    settings.wheelAccelerationStrength = 10.0;

    const auto result = settings.normalized();
    QCOMPARE(result.animationDuration, 0);
    QCOMPARE(result.wheelDistanceFactor, 0.0);
    QCOMPARE(result.wheelStep, 0);
    QCOMPARE(result.minimumWheelStep, 1);
    QCOMPARE(result.maximumPendingDistance, 0);
    QCOMPARE(result.wheelAccelerationStrength, 3.0);
}

void SmoothScrollbarTest::angleDeltaUsesSystemStep()
{
    QScrollBar horizontal(Qt::Horizontal);
    QScrollBar vertical(Qt::Vertical);
    vertical.setSingleStep(7);
    const auto event = makeWheelEvent({}, QPoint(0, -120));

    const auto delta = WheelDeltaNormalizer::normalize(
        event, horizontal, vertical, {});

    QCOMPARE(delta.valueDelta.y(),
             qreal(QApplication::wheelScrollLines() * 20));
    QVERIFY(!delta.shouldPreserveNativeHandling);
}

void SmoothScrollbarTest::angleDeltaUsesConfiguredStep()
{
    QScrollBar horizontal(Qt::Horizontal);
    QScrollBar vertical(Qt::Vertical);
    vertical.setSingleStep(100);
    SmoothScrollSettings settings;
    settings.wheelStep = 36;
    const auto event = makeWheelEvent({}, QPoint(0, -120));

    const auto delta = WheelDeltaNormalizer::normalize(
        event, horizontal, vertical, settings);

    QCOMPARE(delta.valueDelta.y(), 36.0);
    const auto halfStepEvent = makeWheelEvent({}, QPoint(0, -60));
    const auto halfStepDelta = WheelDeltaNormalizer::normalize(
        halfStepEvent, horizontal, vertical, settings);
    QCOMPARE(halfStepDelta.valueDelta.y(), 18.0);
}

void SmoothScrollbarTest::pixelDeltaDefaultsToNativeHandling()
{
    QScrollBar horizontal(Qt::Horizontal);
    QScrollBar vertical(Qt::Vertical);
    const auto event = makeWheelEvent(QPoint(0, 13), {});

    const auto delta = WheelDeltaNormalizer::normalize(
        event, horizontal, vertical, {});

    QVERIFY(delta.usesPixelDelta);
    QVERIFY(delta.shouldPreserveNativeHandling);
    QVERIFY(delta.valueDelta.isNull());

    SmoothScrollSettings settings;
    settings.pixelDeltaMode = PixelDeltaMode::Animated;
    const auto animatedDelta = WheelDeltaNormalizer::normalize(
        event, horizontal, vertical, settings);
    QVERIFY(animatedDelta.usesPixelDelta);
    QVERIFY(!animatedDelta.shouldPreserveNativeHandling);
    QCOMPARE(animatedDelta.valueDelta, QPointF(0, -13));
}

void SmoothScrollbarTest::shiftMovesVerticalDeltaToHorizontalAxis()
{
    QScrollBar horizontal(Qt::Horizontal);
    QScrollBar vertical(Qt::Vertical);
    horizontal.setSingleStep(5);
    const auto event = makeWheelEvent({}, QPoint(0, -120), Qt::ShiftModifier);

    const auto delta = WheelDeltaNormalizer::normalize(
        event, horizontal, vertical, {});

    QVERIFY(delta.valueDelta.x() > 0.0);
    QCOMPARE(delta.valueDelta.y(), 0.0);
}

void SmoothScrollbarTest::horizontalOnlyRangeUsesVerticalWheelInput()
{
    QScrollBar horizontal(Qt::Horizontal);
    QScrollBar vertical(Qt::Vertical);
    horizontal.setRange(0, 100);
    vertical.setRange(0, 0);
    const auto event = makeWheelEvent({}, QPoint(0, -120));

    const auto delta = WheelDeltaNormalizer::normalize(
        event, horizontal, vertical, {});

    QVERIFY(delta.valueDelta.x() > 0.0);
    QCOMPARE(delta.valueDelta.y(), 0.0);
}

void SmoothScrollbarTest::animatorAccumulatesAndClamps()
{
    QScrollBar scrollBar;
    scrollBar.setRange(0, 100);
    scrollBar.setValue(20);
    ScrollAxisAnimator animator(&scrollBar);
    SmoothScrollSettings settings;
    settings.animationDuration = 20;
    animator.setSettings(settings);

    QVERIFY(animator.scrollBy(50));
    QVERIFY(animator.scrollBy(80));
    QCOMPARE(animator.targetPosition(), 100.0);
    QTRY_COMPARE_WITH_TIMEOUT(scrollBar.value(), 100, 250);
}

void SmoothScrollbarTest::animatorReversesFromCurrentPosition()
{
    QScrollBar scrollBar;
    scrollBar.setRange(0, 200);
    scrollBar.setValue(100);
    ScrollAxisAnimator animator(&scrollBar);
    SmoothScrollSettings settings;
    settings.animationDuration = 100;
    animator.setSettings(settings);

    QVERIFY(animator.scrollBy(80));
    QTest::qWait(20);
    const int current = scrollBar.value();
    QVERIFY(animator.scrollBy(-30));
    QCOMPARE(animator.targetPosition(), qreal(current - 30));
}

void SmoothScrollbarTest::animatorExpandsPendingLimitForAcceleration()
{
    QScrollBar scrollBar;
    scrollBar.setRange(0, 1000);
    ScrollAxisAnimator animator(&scrollBar);
    SmoothScrollSettings settings;
    settings.animationDuration = 200;
    settings.maximumPendingDistance = 60;
    animator.setSettings(settings);

    QVERIFY(animator.scrollBy(60));
    QCOMPARE(animator.targetPosition(), 60.0);
    QVERIFY(animator.scrollBy(240, 4.0));
    QCOMPARE(animator.targetPosition(), 240.0);
}

void SmoothScrollbarTest::animatorResynchronizesAfterExternalChange()
{
    QScrollBar scrollBar;
    scrollBar.setRange(0, 200);
    ScrollAxisAnimator animator(&scrollBar);
    SmoothScrollSettings settings;
    settings.animationDuration = 0;
    animator.setSettings(settings);

    scrollBar.setValue(100);
    QVERIFY(animator.scrollBy(-20));
    QCOMPARE(scrollBar.value(), 80);
    QCOMPARE(animator.targetPosition(), 80.0);
}

void SmoothScrollbarTest::externalChangeCancelsAnimation()
{
    QScrollBar scrollBar;
    scrollBar.setRange(0, 200);
    ScrollAxisAnimator animator(&scrollBar);
    SmoothScrollSettings settings;
    settings.animationDuration = 100;
    animator.setSettings(settings);

    QVERIFY(animator.scrollBy(100));
    QVERIFY(animator.isRunning());
    scrollBar.setValue(40);
    QVERIFY(!animator.isRunning());
    QCOMPARE(animator.targetPosition(), 40.0);
    QTest::qWait(120);
    QCOMPARE(scrollBar.value(), 40);
}

void SmoothScrollbarTest::controllerAnimatesWheelInput()
{
    QScrollArea area;
    auto* content = new QWidget;
    content->resize(400, 2000);
    area.setWidget(content);
    area.resize(300, 300);
    area.show();
    QTest::qWait(20);

    SmoothScrollController controller(&area);
    SmoothScrollSettings settings;
    settings.animationDuration = 80;
    settings.pixelDeltaMode = PixelDeltaMode::Animated;
    settings.kineticTouchEnabled = true;
    controller.setSettings(settings);
    QSignalSpy scrollingSpy(&controller,
                            &SmoothScrollController::scrollingChanged);

    const int before = area.verticalScrollBar()->value();
    auto event = makeWheelEvent({}, QPoint(0, -120));
    QApplication::sendEvent(area.viewport(), &event);

    QVERIFY(event.isAccepted());
    QCOMPARE(area.verticalScrollBar()->value(), before);
    QCOMPARE(scrollingSpy.count(), 1);
    QCOMPARE(scrollingSpy.at(0).at(0).toBool(), true);
    QTRY_VERIFY_WITH_TIMEOUT(area.verticalScrollBar()->value() > before, 250);
}

void SmoothScrollbarTest::controllerTracksReplacementScrollBar()
{
    QScrollArea area;
    auto* content = new QWidget;
    content->resize(400, 2000);
    area.setWidget(content);
    area.resize(300, 300);
    area.show();
    QTest::qWait(20);

    SmoothScrollController controller(&area);
    SmoothScrollSettings settings;
    settings.animationDuration = 20;
    controller.setSettings(settings);

    auto* replacement = new QScrollBar(Qt::Vertical);
    area.setVerticalScrollBar(replacement);
    QTest::qWait(1);
    const int before = replacement->value();
    auto event = makeWheelEvent({}, QPoint(0, -120));
    QApplication::sendEvent(area.viewport(), &event);

    QVERIFY(event.isAccepted());
    QTRY_VERIFY_WITH_TIMEOUT(replacement->value() > before, 250);
}

void SmoothScrollbarTest::controllerAcceleratesRapidWheelInput()
{
    QScrollArea area;
    auto* content = new QWidget;
    content->resize(400, 4000);
    area.setWidget(content);
    area.resize(300, 300);
    area.show();
    QTest::qWait(20);

    SmoothScrollController controller(&area);
    SmoothScrollSettings settings;
    settings.animationDuration = 0;
    settings.wheelAccelerationEnabled = true;
    settings.wheelAccelerationStrength = 3.0;
    controller.setSettings(settings);

    auto firstEvent = makeWheelEvent({}, QPoint(0, -120));
    QApplication::sendEvent(area.viewport(), &firstEvent);
    const int firstDistance = area.verticalScrollBar()->value();

    auto secondEvent = makeWheelEvent({}, QPoint(0, -120));
    QApplication::sendEvent(area.viewport(), &secondEvent);
    const int secondDistance = area.verticalScrollBar()->value() - firstDistance;

    QCOMPARE(firstDistance, QApplication::wheelScrollLines() * 20);
    QVERIFY(secondDistance > firstDistance);
    QVERIFY(secondDistance <= firstDistance * 4);
}

void SmoothScrollbarTest::controllerAnimatesTableViewPixelInput()
{
    QTableView table;
    TestTableModel model(&table);
    table.setModel(&model);
    table.setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    table.resize(400, 300);
    table.show();
    QTest::qWait(20);

    SmoothScrollController controller(&table);
    SmoothScrollSettings settings;
    settings.animationDuration = 240;
    settings.pixelDeltaMode = PixelDeltaMode::Animated;
    settings.kineticTouchEnabled = true;
    controller.setSettings(settings);
    QSignalSpy scrollingSpy(&controller,
                            &SmoothScrollController::scrollingChanged);
    QSignalSpy valueSpy(table.verticalScrollBar(), &QScrollBar::valueChanged);

    const int before = table.verticalScrollBar()->value();
    auto event = makeWheelEvent(QPoint(0, -120), {});
    QApplication::sendEvent(table.viewport(), &event);

    QVERIFY(event.isAccepted());
    QCOMPARE(table.verticalScrollBar()->value(), before);
    QCOMPARE(scrollingSpy.count(), 1);
    QTest::qWait(80);
    QVERIFY(table.verticalScrollBar()->value() > before);
    QVERIFY(table.verticalScrollBar()->value() < before + 120);
    QVERIFY(valueSpy.count() >= 2);
    QTRY_COMPARE_WITH_TIMEOUT(table.verticalScrollBar()->value(), before + 120, 400);
}

void SmoothScrollbarTest::controllerKeepsWheelFilterPriority()
{
    QScrollArea area;
    auto* content = new QWidget;
    content->resize(400, 2000);
    area.setWidget(content);
    area.resize(300, 300);
    area.show();
    QTest::qWait(20);

    SmoothScrollController controller(&area);
    WheelEatingFilter competingFilter;
    area.viewport()->installEventFilter(&competingFilter);

    SmoothScrollSettings settings;
    settings.animationDuration = 80;
    settings.kineticTouchEnabled = true;
    controller.setSettings(settings);
    QSignalSpy scrollingSpy(&controller,
                            &SmoothScrollController::scrollingChanged);

    auto event = makeWheelEvent({}, QPoint(0, -120));
    QApplication::sendEvent(area.viewport(), &event);

    QCOMPARE(competingFilter.wheelEventCount, 0);
    QCOMPARE(scrollingSpy.count(), 1);
    QTRY_VERIFY_WITH_TIMEOUT(area.verticalScrollBar()->value() > 0, 250);
}

QTEST_MAIN(SmoothScrollbarTest)
#include "tst_smoothscrollbar.moc"
