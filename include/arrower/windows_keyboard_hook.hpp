#pragma once

#ifdef _WIN32

#include <atomic>
#include <condition_variable>
#include <mutex>
#include <memory>
#include <string>
#include <thread>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "arrower/config.hpp"
#include "arrower/input_state.hpp"
#include "arrower/movement_engine.hpp"

namespace arrower {

class WindowsMouseController;

class WindowsKeyboardHookService {
public:
    explicit WindowsKeyboardHookService(Config config);
    ~WindowsKeyboardHookService();

    bool Start();
    bool Stop();
    bool ReloadConfig(const Config& config);
    void SetConfig(const Config& config);
    Config CurrentConfig() const;
    bool IsRunning() const;
    std::string LastError() const;

private:
    void ServiceThreadMain(Config config);
    bool InstallHook();
    void UninstallHook();
    void StartMovementThread();
    void StopMovementThread();
    void HandleKeyDown(int virtual_key);
    void HandleKeyUp(int virtual_key);
    bool ShouldSuppress(int virtual_key) const;
    bool IsEmergencyQuitPressed() const;
    bool IsConfiguredKeyDown(int configured_virtual_key) const;
    bool IsControlModeActive() const;
    void EnsureLeftButtonReleased();
    void RequestQuit();
    void SetLastError(const std::string& error);
    static int NormalizeVirtualKey(DWORD vk_code, DWORD scan_code, DWORD flags);
    static LRESULT CALLBACK HookProc(int code, WPARAM w_param, LPARAM l_param);

    mutable std::mutex state_mutex_;
    std::condition_variable startup_cv_;
    Config config_;
    Config active_config_;
    std::string last_error_;
    bool startup_complete_ = false;
    bool startup_succeeded_ = false;
    InputState input_state_;
    MovementEngine movement_engine_;
    std::unique_ptr<WindowsMouseController> mouse_;
    void* hook_ = nullptr;
    std::atomic<void*> stop_event_{nullptr};
    std::atomic<unsigned long> hook_thread_id_{0};
    std::atomic<bool> running_{false};
    std::atomic<bool> left_button_held_{false};
    std::atomic<bool> right_click_pending_{false};
    std::atomic<bool> scroll_mode_used_{false};
    std::thread hook_thread_;
    std::thread movement_thread_;
};

}  // namespace arrower

#endif
