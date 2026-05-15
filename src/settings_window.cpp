#include "settings_window.hpp"

#include <QCloseEvent>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QSpinBox>
#include <QVBoxLayout>

#include "binding_capture_button.hpp"

namespace arrower {

SettingsWindow::SettingsWindow(QWidget* parent) : QWidget(parent) {
    setWindowTitle("arrower Settings");
    resize(520, 520);

    auto* root_layout = new QVBoxLayout(this);

    auto* status_group = new QGroupBox("Status", this);
    auto* status_layout = new QVBoxLayout(status_group);
    status_label_ = new QLabel("Service status: disabled", status_group);
    info_label_ = new QLabel(status_group);
    info_label_->setWordWrap(true);
    info_label_->setText("Emergency quit: Ctrl + Alt + Esc");
    toggle_button_ = new QPushButton("Enable", status_group);
    status_layout->addWidget(status_label_);
    status_layout->addWidget(new QLabel("Emergency quit: Ctrl + Alt + Esc", status_group));
    status_layout->addWidget(toggle_button_);
    status_layout->addWidget(info_label_);

    auto* onboarding_group = new QGroupBox("How It Works", this);
    auto* onboarding_layout = new QVBoxLayout(onboarding_group);
    auto* onboarding = new QLabel(
        "Hold the activation modifier to enter mouse mode.\n"
        "Arrow keys move the cursor.\n"
        "Tap '.' for left click or hold it to drag.\n"
        "Tap '/' for right click or hold it with Up/Down to scroll.\n"
        "Settings are saved directly to config.json.",
        onboarding_group);
    onboarding->setWordWrap(true);
    config_path_label_ = new QLabel(onboarding_group);
    config_path_label_->setWordWrap(true);
    onboarding_layout->addWidget(onboarding);
    onboarding_layout->addWidget(config_path_label_);

    auto* bindings_group = new QGroupBox("Bindings", this);
    auto* bindings_layout = new QFormLayout(bindings_group);
    activation_modifier_ = new BindingCaptureButton(bindings_group);
    up_ = new BindingCaptureButton(bindings_group);
    down_ = new BindingCaptureButton(bindings_group);
    left_ = new BindingCaptureButton(bindings_group);
    right_ = new BindingCaptureButton(bindings_group);
    left_click_ = new BindingCaptureButton(bindings_group);
    right_click_ = new BindingCaptureButton(bindings_group);
    bindings_layout->addRow("Activation modifier", activation_modifier_);
    bindings_layout->addRow("Move up", up_);
    bindings_layout->addRow("Move down", down_);
    bindings_layout->addRow("Move left", left_);
    bindings_layout->addRow("Move right", right_);
    bindings_layout->addRow("Left click / drag", left_click_);
    bindings_layout->addRow("Right click / scroll", right_click_);

    auto* rates_group = new QGroupBox("Movement", this);
    auto* rates_layout = new QFormLayout(rates_group);
    base_speed_ = new QDoubleSpinBox(rates_group);
    acceleration_ = new QDoubleSpinBox(rates_group);
    max_speed_ = new QDoubleSpinBox(rates_group);
    update_rate_ = new QSpinBox(rates_group);
    drag_rate_ = new QSpinBox(rates_group);
    scroll_rate_ = new QSpinBox(rates_group);

    for (auto* spin_box : {base_speed_, acceleration_, max_speed_}) {
        spin_box->setDecimals(2);
        spin_box->setRange(0.0, 1000.0);
    }

    for (auto* spin_box : {update_rate_, drag_rate_, scroll_rate_}) {
        spin_box->setRange(1, 1000);
    }

    rates_layout->addRow("Base speed (px/tick)", base_speed_);
    rates_layout->addRow("Acceleration (px/tick)", acceleration_);
    rates_layout->addRow("Max speed (px/tick)", max_speed_);
    rates_layout->addRow("Move rate (Hz)", update_rate_);
    rates_layout->addRow("Drag rate (Hz)", drag_rate_);
    rates_layout->addRow("Scroll rate (Hz)", scroll_rate_);

    root_layout->addWidget(status_group);
    root_layout->addWidget(onboarding_group);
    root_layout->addWidget(bindings_group);
    root_layout->addWidget(rates_group);

    WireSignals();
}

void SettingsWindow::SetConfig(const Config& config) {
    suppress_changes_ = true;
    activation_modifier_->SetVirtualKey(config.activation_modifier);
    up_->SetVirtualKey(config.bindings.up);
    down_->SetVirtualKey(config.bindings.down);
    left_->SetVirtualKey(config.bindings.left);
    right_->SetVirtualKey(config.bindings.right);
    left_click_->SetVirtualKey(config.bindings.left_click);
    right_click_->SetVirtualKey(config.bindings.right_click);
    base_speed_->setValue(config.movement.base_speed_px_per_tick);
    acceleration_->setValue(config.movement.acceleration_px_per_tick);
    max_speed_->setValue(config.movement.max_speed_px_per_tick);
    update_rate_->setValue(config.movement.update_rate_hz);
    drag_rate_->setValue(config.movement.drag_update_rate_hz);
    scroll_rate_->setValue(config.movement.scroll_update_rate_hz);
    suppress_changes_ = false;
}

void SettingsWindow::SetRunning(bool running) {
    running_ = running;
    status_label_->setText(QString("Service status: %1").arg(running ? "enabled" : "disabled"));
    toggle_button_->setText(running ? "Disable" : "Enable");
}

void SettingsWindow::SetConfigPath(const QString& config_path) {
    config_path_label_->setText(QString("Config file: %1").arg(config_path));
}

void SettingsWindow::SetInfoMessage(const QString& message, bool error) {
    info_label_->setText(message);
    info_label_->setStyleSheet(error ? "color: #a40000;" : "color: #204a87;");
}

void SettingsWindow::closeEvent(QCloseEvent* event) {
    hide();
    event->ignore();
}

void SettingsWindow::WireSignals() {
    connect(toggle_button_, &QPushButton::clicked, this, [this]() {
        if (running_) {
            emit StopRequested();
        } else {
            emit StartRequested();
        }
    });

    for (auto* button : {activation_modifier_, up_, down_, left_, right_, left_click_, right_click_}) {
        connect(button, &BindingCaptureButton::VirtualKeyChanged, this, [this](int) { EmitConfigEdited(); });
    }

    for (auto* spin_box : {base_speed_, acceleration_, max_speed_}) {
        connect(spin_box, qOverload<double>(&QDoubleSpinBox::valueChanged), this, [this](double) { EmitConfigEdited(); });
    }

    for (auto* spin_box : {update_rate_, drag_rate_, scroll_rate_}) {
        connect(spin_box, qOverload<int>(&QSpinBox::valueChanged), this, [this](int) { EmitConfigEdited(); });
    }
}

Config SettingsWindow::BuildConfig() const {
    return {
        activation_modifier_->VirtualKey(),
        {
            up_->VirtualKey(),
            down_->VirtualKey(),
            left_->VirtualKey(),
            right_->VirtualKey(),
            left_click_->VirtualKey(),
            right_click_->VirtualKey(),
        },
        {
            base_speed_->value(),
            acceleration_->value(),
            max_speed_->value(),
            update_rate_->value(),
            drag_rate_->value(),
            scroll_rate_->value(),
        },
    };
}

void SettingsWindow::EmitConfigEdited() {
    if (suppress_changes_) {
        return;
    }

    emit ConfigEdited(BuildConfig());
}

}  // namespace arrower
