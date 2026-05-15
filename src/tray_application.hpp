#pragma once

#include <QObject>

#include <memory>

#include "arrower/config.hpp"
#include "arrower/windows_keyboard_hook.hpp"

class QAction;
class QMenu;
class QSystemTrayIcon;

namespace arrower {

class SettingsWindow;

class TrayApplication : public QObject {
    Q_OBJECT

public:
    explicit TrayApplication(QObject* parent = nullptr);
    ~TrayApplication() override;

    void Initialize();

private:
    bool StartService();
    void StopService();
    void UpdateUi();
    void ShowSettings();
    void ApplyConfig(const Config& config);
    bool ValidateConfig(const Config& config, QString* error) const;
    void RestoreUiConfig();

    std::filesystem::path config_path_;
    Config config_;
    WindowsKeyboardHookService service_;
    std::unique_ptr<QSystemTrayIcon> tray_icon_;
    std::unique_ptr<QMenu> tray_menu_;
    QAction* toggle_action_ = nullptr;
    QAction* settings_action_ = nullptr;
    QAction* quit_action_ = nullptr;
    std::unique_ptr<SettingsWindow> settings_window_;
};

}  // namespace arrower
