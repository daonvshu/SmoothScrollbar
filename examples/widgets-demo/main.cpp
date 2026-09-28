#include <smoothscroll/smoothscrollcontroller.h>

#include <QAbstractTableModel>
#include <QAbstractItemView>
#include <QApplication>
#include <QComboBox>
#include <QCheckBox>
#include <QDoubleSpinBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QSpinBox>
#include <QTableView>
#include <QVBoxLayout>
#include <QWidget>

class DemoTableModel final : public QAbstractTableModel {
public:
    using QAbstractTableModel::QAbstractTableModel;

    int rowCount(const QModelIndex& parent = QModelIndex()) const override
    {
        return parent.isValid() ? 0 : 100;
    }

    int columnCount(const QModelIndex& parent = QModelIndex()) const override
    {
        return parent.isValid() ? 0 : 3;
    }

    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override
    {
        if (!index.isValid() || role != Qt::DisplayRole) {
            return {};
        }

        switch (index.column()) {
        case 0:
            return index.row() + 1;
        case 1:
            return QStringLiteral("Item %1").arg(index.row() + 1);
        case 2:
            return QStringLiteral("Smooth scrolling backed by QAbstractTableModel");
        default:
            return {};
        }
    }

    QVariant headerData(int section, Qt::Orientation orientation,
                        int role = Qt::DisplayRole) const override
    {
        if (role != Qt::DisplayRole) {
            return {};
        }
        if (orientation == Qt::Vertical) {
            return section + 1;
        }

        static const QStringList headers = {
            QStringLiteral("Index"),
            QStringLiteral("Name"),
            QStringLiteral("Description")
        };
        return headers.value(section);
    }
};

int main(int argc, char* argv[])
{
    QApplication application(argc, argv);

    QWidget window;
    window.setWindowTitle(QStringLiteral("SmoothScrollbar Widgets Demo"));
    window.resize(760, 520);

    auto* layout = new QVBoxLayout(&window);
    auto* controls = new QHBoxLayout;
    auto* easingCombo = new QComboBox(&window);
    easingCombo->addItem(QStringLiteral("Linear"), int(QEasingCurve::Linear));
    easingCombo->addItem(QStringLiteral("Out Quad"), int(QEasingCurve::OutQuad));
    easingCombo->addItem(QStringLiteral("Out Cubic"), int(QEasingCurve::OutCubic));
    easingCombo->addItem(QStringLiteral("Out Quart"), int(QEasingCurve::OutQuart));
    easingCombo->addItem(QStringLiteral("Out Sine"), int(QEasingCurve::OutSine));
    easingCombo->setCurrentIndex(2);

    auto* durationSpin = new QSpinBox(&window);
    durationSpin->setRange(0, 1000);
    durationSpin->setSingleStep(10);
    durationSpin->setSuffix(QStringLiteral(" ms"));
    durationSpin->setValue(300);

    auto* wheelStepSpin = new QSpinBox(&window);
    wheelStepSpin->setRange(1, 160);
    wheelStepSpin->setSingleStep(4);
    wheelStepSpin->setSuffix(QStringLiteral(" px"));
    wheelStepSpin->setValue(36);

    controls->addWidget(new QLabel(QStringLiteral("Easing"), &window));
    controls->addWidget(easingCombo);
    controls->addSpacing(16);
    controls->addWidget(new QLabel(QStringLiteral("Duration"), &window));
    controls->addWidget(durationSpin);
    controls->addSpacing(16);
    controls->addWidget(new QLabel(QStringLiteral("Wheel step"), &window));
    controls->addWidget(wheelStepSpin);
    controls->addStretch();
    layout->addLayout(controls);

    auto* speedControls = new QHBoxLayout;
    auto* accelerationCheck = new QCheckBox(QStringLiteral("Accelerate wheel input"), &window);
    accelerationCheck->setChecked(false);
    auto* momentumCheck = new QCheckBox(QStringLiteral("Continue after wheel stops"), &window);
    momentumCheck->setChecked(true);
    auto* momentumStepsSpin = new QSpinBox(&window);
    momentumStepsSpin->setRange(2, 12);
    momentumStepsSpin->setValue(3);
    momentumStepsSpin->setToolTip(QStringLiteral("Minimum wheel notches before momentum starts"));
    auto* strengthSpin = new QDoubleSpinBox(&window);
    strengthSpin->setRange(0.0, 3.0);
    strengthSpin->setSingleStep(0.1);
    strengthSpin->setDecimals(1);
    strengthSpin->setValue(1.5);
    strengthSpin->setEnabled(false);
    speedControls->addWidget(momentumCheck);
    speedControls->addWidget(new QLabel(QStringLiteral("Min notches"), &window));
    speedControls->addWidget(momentumStepsSpin);
    speedControls->addSpacing(16);
    speedControls->addWidget(accelerationCheck);
    speedControls->addWidget(new QLabel(QStringLiteral("Strength"), &window));
    speedControls->addWidget(strengthSpin);
    speedControls->addStretch();
    layout->addLayout(speedControls);

    auto* table = new QTableView(&window);
    DemoTableModel model(table);
    table->setModel(&model);
    table->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    table->setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
    layout->addWidget(table);

    auto* controller = new sscroll::SmoothScrollController(table, table);
    sscroll::SmoothScrollSettings settings;
    settings.animationDuration = 300;
    settings.maximumPendingDistance = 360;
    settings.wheelStep = wheelStepSpin->value();
    settings.wheelMomentumMinimumSteps = momentumStepsSpin->value();
    controller->setSettings(settings);

    QObject::connect(easingCombo,
                     QOverload<int>::of(&QComboBox::currentIndexChanged),
                     controller, [=](int index) {
        auto updatedSettings = controller->settings();
        updatedSettings.easingCurve = static_cast<QEasingCurve::Type>(
            easingCombo->itemData(index).toInt());
        controller->setSettings(updatedSettings);
    });
    QObject::connect(durationSpin,
                     QOverload<int>::of(&QSpinBox::valueChanged),
                     controller, [=](int duration) {
        auto updatedSettings = controller->settings();
        updatedSettings.animationDuration = duration;
        controller->setSettings(updatedSettings);
    });
    QObject::connect(wheelStepSpin,
                     QOverload<int>::of(&QSpinBox::valueChanged),
                     controller, [=](int step) {
        auto updatedSettings = controller->settings();
        updatedSettings.wheelStep = step;
        controller->setSettings(updatedSettings);
    });

    QObject::connect(accelerationCheck, &QCheckBox::toggled, controller, [=](bool enabled) {
        auto updatedSettings = controller->settings();
        updatedSettings.wheelAccelerationEnabled = enabled;
        controller->setSettings(updatedSettings);
        strengthSpin->setEnabled(enabled);
    });
    QObject::connect(momentumCheck, &QCheckBox::toggled, controller, [=](bool enabled) {
        auto updatedSettings = controller->settings();
        updatedSettings.wheelMomentumEnabled = enabled;
        controller->setSettings(updatedSettings);
        momentumStepsSpin->setEnabled(enabled);
    });
    QObject::connect(momentumStepsSpin,
                     QOverload<int>::of(&QSpinBox::valueChanged),
                     controller, [=](int steps) {
        auto updatedSettings = controller->settings();
        updatedSettings.wheelMomentumMinimumSteps = steps;
        controller->setSettings(updatedSettings);
    });
    QObject::connect(strengthSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
                     controller, [=](double strength) {
        auto updatedSettings = controller->settings();
        updatedSettings.wheelAccelerationStrength = strength;
        controller->setSettings(updatedSettings);
    });

    window.show();
    return application.exec();
}
