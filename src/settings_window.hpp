#pragma once

#include <QWidget>

#include "arrower/config.hpp"

class QLabel;
class QPushButton;
class QDoubleSpinBox;
class QSpinBox;

namespace arrower {

class BindingCaptureButton;

class SettingsWindow : public QWidget {
    Q_OBJECT

public:
    explicit SettingsWindow(QWidget* parent = nullptr);

    void SetConfig(const Config& config);
    void SetRunning(bool running);
    void SetConfigPath(const QString& config_path);
    void SetInfoMessage(const QString& message, bool error);

signals:
    void ConfigEdited(const arrower::Config& config);
    void StartRequested();
    void StopRequested();

protected:
    void closeEvent(QCloseEvent* event) override;

private:
    void WireSignals();
    Config BuildConfig() const;
    void EmitConfigEdited();

    bool suppress_changes_ = false;
    QLabel* status_label_ = nullptr;
    QLabel* info_label_ = nullptr;
    QLabel* config_path_label_ = nullptr;
    QPushButton* toggle_button_ = nullptr;
    BindingCaptureButton* activation_modifier_ = nullptr;
    BindingCaptureButton* up_ = nullptr;
    BindingCaptureButton* down_ = nullptr;
    BindingCaptureButton* left_ = nullptr;
    BindingCaptureButton* right_ = nullptr;
    BindingCaptureButton* left_click_ = nullptr;
    BindingCaptureButton* right_click_ = nullptr;
    QDoubleSpinBox* base_speed_ = nullptr;
    QDoubleSpinBox* acceleration_ = nullptr;
    QDoubleSpinBox* max_speed_ = nullptr;
    QSpinBox* update_rate_ = nullptr;
    QSpinBox* drag_rate_ = nullptr;
    QSpinBox* scroll_rate_ = nullptr;
    bool running_ = false;
};

}  // namespace arrower
