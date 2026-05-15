#include "tray_application.hpp"

#include <vector>

#include <QAction>
#include <QApplication>
#include <QIcon>
#include <QMenu>
#include <QMessageBox>
#include <QStyle>
#include <QSystemTrayIcon>

#include "arrower/app_paths.hpp"
#include "arrower/key_codes.hpp"
#include "settings_window.hpp"

namespace arrower {

namespace {

std::vector<int> BindingValues(const Config& config) {
    return {
        config.activation_modifier,
        config.bindings.up,
        config.bindings.down,
        config.bindings.left,
        config.bindings.right,
        config.bindings.left_click,
        config.bindings.right_click,
    };
}

}  // namespace

TrayApplication::TrayApplication(QObject* parent)
    : QObject(parent), config_path_(DefaultConfigPath()), config_(DefaultConfig()), service_(config_) {}

TrayApplication::~TrayApplication() {
    StopService();
}

void TrayApplication::Initialize() {
    const auto load_result = LoadConfigFile(config_path_);
    if (load_result.success) {
        config_ = load_result.config;
        service_.SetConfig(config_);
    } else {
        config_ = DefaultConfig();
        service_.SetConfig(config_);
    }

    settings_window_ = std::make_unique<SettingsWindow>();
    settings_window_->SetConfig(config_);
    settings_window_->SetConfigPath(QString::fromStdString(config_path_.string()));
    settings_window_->SetInfoMessage(
        load_result.success ? QString::fromStdString(load_result.message)
                            : QString::fromStdString(load_result.message + ". Using defaults until saved."),
        !load_result.success);

    connect(settings_window_.get(), &SettingsWindow::ConfigEdited, this, &TrayApplication::ApplyConfig);
    connect(settings_window_.get(), &SettingsWindow::StartRequested, this, [this]() {
        StartService();
        UpdateUi();
    });
    connect(settings_window_.get(), &SettingsWindow::StopRequested, this, [this]() {
        StopService();
        UpdateUi();
    });

    tray_icon_ = std::make_unique<QSystemTrayIcon>(
        QApplication::style()->standardIcon(QStyle::SP_ComputerIcon));
    tray_menu_ = std::make_unique<QMenu>();
    toggle_action_ = tray_menu_->addAction("Disable");
    settings_action_ = tray_menu_->addAction("Settings");
    tray_menu_->addSeparator();
    quit_action_ = tray_menu_->addAction("Quit");
    tray_icon_->setContextMenu(tray_menu_.get());
    tray_icon_->setToolTip("arrower");
    tray_icon_->show();

    connect(toggle_action_, &QAction::triggered, this, [this]() {
        if (service_.IsRunning()) {
            StopService();
        } else {
            StartService();
        }
        UpdateUi();
    });
    connect(settings_action_, &QAction::triggered, this, &TrayApplication::ShowSettings);
    connect(quit_action_, &QAction::triggered, this, [this]() {
        StopService();
        QApplication::quit();
    });
    connect(tray_icon_.get(), &QSystemTrayIcon::activated, this, [this](QSystemTrayIcon::ActivationReason reason) {
        if (reason == QSystemTrayIcon::Trigger || reason == QSystemTrayIcon::DoubleClick) {
            ShowSettings();
        }
    });

    if (!load_result.success) {
        settings_window_->show();
    }

    StartService();
    UpdateUi();
}

bool TrayApplication::StartService() {
    const bool success = service_.Start();
    if (!success) {
        settings_window_->SetInfoMessage(QString::fromStdString(service_.LastError()), true);
        settings_window_->show();
    }
    return success;
}

void TrayApplication::StopService() {
    service_.Stop();
}

void TrayApplication::UpdateUi() {
    const bool running = service_.IsRunning();
    settings_window_->SetRunning(running);
    toggle_action_->setText(running ? "Disable" : "Enable");
    tray_icon_->setToolTip(QString("arrower (%1)").arg(running ? "enabled" : "disabled"));
}

void TrayApplication::ShowSettings() {
    settings_window_->show();
    settings_window_->raise();
    settings_window_->activateWindow();
}

void TrayApplication::ApplyConfig(const Config& config) {
    QString error;
    if (!ValidateConfig(config, &error)) {
        settings_window_->SetInfoMessage(error, true);
        RestoreUiConfig();
        return;
    }

    const Config previous_config = config_;
    const auto save_result = SaveConfigFile(config_path_, config);
    if (!save_result.success) {
        settings_window_->SetInfoMessage(QString::fromStdString(save_result.message), true);
        RestoreUiConfig();
        return;
    }

    if (!service_.ReloadConfig(config)) {
        SaveConfigFile(config_path_, previous_config);
        service_.SetConfig(previous_config);
        settings_window_->SetInfoMessage(
            QString::fromStdString("Failed to apply config: " + service_.LastError()), true);
        RestoreUiConfig();
        return;
    }

    config_ = config;
    settings_window_->SetInfoMessage(QString::fromStdString(save_result.message), false);
    UpdateUi();
}

bool TrayApplication::ValidateConfig(const Config& config, QString* error) const {
    std::vector<int> bindings = BindingValues(config);
    for (std::size_t i = 0; i < bindings.size(); ++i) {
        if (bindings[i] == 0) {
            *error = "Bindings cannot be empty.";
            return false;
        }
        for (std::size_t j = i + 1; j < bindings.size(); ++j) {
            if (bindings[i] == bindings[j]) {
                *error = QString("Bindings must be unique. Conflict on %1.")
                             .arg(QString::fromStdString(DisplayVirtualKeyName(bindings[i])));
                return false;
            }
        }
    }

    if (config.activation_modifier == VK_ESCAPE) {
        *error = "Activation modifier cannot be Escape because emergency quit uses Ctrl + Alt + Esc.";
        return false;
    }

    if (config.movement.base_speed_px_per_tick < 0.0 || config.movement.acceleration_px_per_tick < 0.0 ||
        config.movement.max_speed_px_per_tick < config.movement.base_speed_px_per_tick ||
        config.movement.update_rate_hz <= 0 || config.movement.drag_update_rate_hz <= 0 ||
        config.movement.scroll_update_rate_hz <= 0) {
        *error = "Movement values are invalid.";
        return false;
    }

    return true;
}

void TrayApplication::RestoreUiConfig() {
    settings_window_->SetConfig(config_);
    UpdateUi();
}

}  // namespace arrower
