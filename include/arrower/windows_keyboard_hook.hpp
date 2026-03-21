#pragma once

#ifdef _WIN32

#include <atomic>
#include <memory>
#include <thread>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "arrower/config.hpp"
#include "arrower/input_state.hpp"
#include "arrower/movement_engine.hpp"

namespace arrower {

class WindowsMouseController;

class WindowsKeyboardHookApp {
public:
    explicit WindowsKeyboardHookApp(Config config);
    ~WindowsKeyboardHookApp();

    int Run();

private:
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
    void PrintStartupSummary() const;
    static int NormalizeVirtualKey(DWORD vk_code, DWORD scan_code, DWORD flags);
    static LRESULT CALLBACK HookProc(int code, WPARAM w_param, LPARAM l_param);

    Config config_;
    InputState input_state_;
    MovementEngine movement_engine_;
    std::unique_ptr<WindowsMouseController> mouse_;
    void* hook_ = nullptr;
    unsigned long main_thread_id_ = 0;
    std::atomic<bool> running_{false};
    std::atomic<bool> left_button_held_{false};
    std::atomic<bool> right_click_pending_{false};
    std::atomic<bool> scroll_mode_used_{false};
    std::thread movement_thread_;
};

}  // namespace arrower

#endif
