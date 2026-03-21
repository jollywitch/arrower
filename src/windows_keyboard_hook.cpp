#ifdef _WIN32

#include "arrower/windows_keyboard_hook.hpp"

#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <mutex>
#include <sstream>
#include <thread>

#include "arrower/key_codes.hpp"
#include "arrower/windows_mouse_controller.hpp"

namespace arrower {

namespace {

constexpr UINT kQuitMessage = WM_APP + 1;
WindowsKeyboardHookApp* g_app = nullptr;
constexpr int kGenericCtrl = VK_CONTROL;
constexpr int kGenericAlt = VK_MENU;
constexpr DWORD kWheelDelta = 120;

std::filesystem::path LogFilePath() {
    wchar_t path_buffer[MAX_PATH];
    const DWORD length = GetModuleFileNameW(nullptr, path_buffer, MAX_PATH);
    if (length == 0 || length == MAX_PATH) {
        return std::filesystem::current_path() / "arrower.log";
    }

    return std::filesystem::path(path_buffer).parent_path() / "arrower.log";
}

void AppendLogLine(const std::string& line) {
    static std::mutex log_mutex;
    const std::lock_guard<std::mutex> lock(log_mutex);
    std::ofstream output(LogFilePath(), std::ios::app);
    output << line << '\n';
}

bool PhysicalKeyDown(int virtual_key) {
    return (GetAsyncKeyState(virtual_key) & 0x8000) != 0;
}

KBDLLHOOKSTRUCT ReadHookStruct(LPARAM value) {
    return *reinterpret_cast<const KBDLLHOOKSTRUCT*>(value);
}

}  // namespace

WindowsKeyboardHookApp::WindowsKeyboardHookApp(Config config)
    : config_(config), movement_engine_(config.movement), mouse_(std::make_unique<WindowsMouseController>()) {}

WindowsKeyboardHookApp::~WindowsKeyboardHookApp() {
    EnsureLeftButtonReleased();
    StopMovementThread();
    UninstallHook();
}

int WindowsKeyboardHookApp::Run() {
    main_thread_id_ = GetCurrentThreadId();
    g_app = this;
    AppendLogLine("Run: starting application");

    if (!InstallHook()) {
        AppendLogLine("Run: failed to install keyboard hook");
        std::cerr << "Failed to install keyboard hook\n";
        return 1;
    }

    running_.store(true);
    StartMovementThread();

    MSG msg{};
    while (GetMessageW(&msg, nullptr, 0, 0) > 0) {
        if (msg.message == kQuitMessage) {
            break;
        }

        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    running_.store(false);
    StopMovementThread();
    UninstallHook();
    g_app = nullptr;
    return 0;
}

bool WindowsKeyboardHookApp::InstallHook() {
    HHOOK hook = SetWindowsHookExW(WH_KEYBOARD_LL, HookProc, GetModuleHandleW(nullptr), 0);
    if (hook == nullptr) {
        AppendLogLine("InstallHook: SetWindowsHookExW returned null");
        return false;
    }

    hook_ = hook;
    AppendLogLine("InstallHook: keyboard hook installed");
    return true;
}

void WindowsKeyboardHookApp::UninstallHook() {
    if (hook_ != nullptr) {
        UnhookWindowsHookEx(reinterpret_cast<HHOOK>(hook_));
        hook_ = nullptr;
        AppendLogLine("UninstallHook: keyboard hook removed");
    }
}

void WindowsKeyboardHookApp::StartMovementThread() {
    movement_thread_ = std::thread([this]() {
        const auto tick_interval = std::chrono::duration<double>(1.0 / static_cast<double>(config_.movement.update_rate_hz));
        auto last_drag_tick = std::chrono::steady_clock::now();
        auto last_scroll_tick = std::chrono::steady_clock::now();

        while (running_.load()) {
            if (!IsControlModeActive()) {
                EnsureLeftButtonReleased();
                movement_engine_.Reset();
                last_drag_tick = std::chrono::steady_clock::now();
                last_scroll_tick = std::chrono::steady_clock::now();
                std::this_thread::sleep_for(tick_interval);
                continue;
            }

            if (IsConfiguredKeyDown(config_.bindings.right_click)) {
                const auto now = std::chrono::steady_clock::now();
                const auto scroll_interval = std::chrono::duration<double>(
                    1.0 / static_cast<double>(config_.movement.scroll_update_rate_hz));
                if (now - last_scroll_tick >= scroll_interval) {
                    if (IsConfiguredKeyDown(config_.bindings.up)) {
                        scroll_mode_used_.store(true);
                        mouse_->ScrollVertical(static_cast<int>(kWheelDelta));
                        last_scroll_tick = now;
                    }
                    if (IsConfiguredKeyDown(config_.bindings.down)) {
                        scroll_mode_used_.store(true);
                        mouse_->ScrollVertical(-static_cast<int>(kWheelDelta));
                        last_scroll_tick = now;
                    }
                }

                movement_engine_.Reset();
                std::this_thread::sleep_for(tick_interval);
                continue;
            }

            const bool dragging = left_button_held_.load();
            const auto now = std::chrono::steady_clock::now();
            if (dragging) {
                const auto drag_interval = std::chrono::duration<double>(
                    1.0 / static_cast<double>(config_.movement.drag_update_rate_hz));
                if (now - last_drag_tick < drag_interval) {
                    std::this_thread::sleep_for(tick_interval);
                    continue;
                }
                last_drag_tick = now;
            } else {
                last_drag_tick = now;
            }

            const auto delta = movement_engine_.Tick(
                IsConfiguredKeyDown(config_.bindings.up),
                IsConfiguredKeyDown(config_.bindings.down),
                IsConfiguredKeyDown(config_.bindings.left),
                IsConfiguredKeyDown(config_.bindings.right));

            if (delta.dx != 0 || delta.dy != 0) {
                mouse_->MoveBy(delta.dx, delta.dy);
            }

            std::this_thread::sleep_for(tick_interval);
        }
    });
}

void WindowsKeyboardHookApp::StopMovementThread() {
    running_.store(false);
    if (movement_thread_.joinable()) {
        movement_thread_.join();
    }
}

void WindowsKeyboardHookApp::HandleKeyDown(int virtual_key) {
    const bool first_press = input_state_.OnKeyDown(virtual_key);

    if (!IsControlModeActive() || !first_press) {
        return;
    }

    if (virtual_key == config_.bindings.left_click) {
        AppendLogLine("HandleKeyDown: left button down");
        mouse_->LeftDown();
        left_button_held_.store(true);
    } else if (virtual_key == config_.bindings.right_click) {
        right_click_pending_.store(true);
        scroll_mode_used_.store(false);
    }
}

void WindowsKeyboardHookApp::HandleKeyUp(int virtual_key) {
    const bool was_down = input_state_.OnKeyUp(virtual_key);
    if (!was_down) {
        return;
    }

    if (virtual_key == config_.bindings.left_click || virtual_key == config_.activation_modifier) {
        EnsureLeftButtonReleased();
    }

    if (virtual_key == config_.bindings.right_click) {
        const bool right_click_pending = right_click_pending_.exchange(false);
        const bool scroll_mode_used = scroll_mode_used_.exchange(false);
        if (right_click_pending && !scroll_mode_used && IsConfiguredKeyDown(config_.activation_modifier)) {
            AppendLogLine("HandleKeyUp: right click fired");
            mouse_->RightClick();
        }
    }
}

bool WindowsKeyboardHookApp::ShouldSuppress(int virtual_key) const {
    if (IsEmergencyQuitPressed()) {
        return true;
    }

    if (virtual_key == config_.activation_modifier) {
        return true;
    }

    if (!IsControlModeActive()) {
        return false;
    }

    if (virtual_key == config_.bindings.right_click) {
        return true;
    }

    if (IsConfiguredKeyDown(config_.bindings.right_click) &&
        (virtual_key == config_.bindings.up || virtual_key == config_.bindings.down)) {
        return true;
    }

    return virtual_key == config_.bindings.up || virtual_key == config_.bindings.down ||
           virtual_key == config_.bindings.left || virtual_key == config_.bindings.right ||
           virtual_key == config_.bindings.left_click;
}

bool WindowsKeyboardHookApp::IsEmergencyQuitPressed() const {
    return (IsConfiguredKeyDown(*ParseVirtualKeyName("LeftCtrl")) ||
            IsConfiguredKeyDown(*ParseVirtualKeyName("RightCtrl"))) &&
           (IsConfiguredKeyDown(*ParseVirtualKeyName("LeftAlt")) ||
            IsConfiguredKeyDown(*ParseVirtualKeyName("RightAlt"))) &&
           IsConfiguredKeyDown(*ParseVirtualKeyName("Escape"));
}

bool WindowsKeyboardHookApp::IsConfiguredKeyDown(int configured_virtual_key) const {
    if (PhysicalKeyDown(configured_virtual_key) || input_state_.IsDown(configured_virtual_key)) {
        return true;
    }

    if (configured_virtual_key == VK_CONTROL) {
        return PhysicalKeyDown(kGenericCtrl) || input_state_.IsDown(kGenericCtrl);
    }

    if (configured_virtual_key == VK_MENU) {
        return PhysicalKeyDown(kGenericAlt) || input_state_.IsDown(kGenericAlt);
    }

    return false;
}

bool WindowsKeyboardHookApp::IsControlModeActive() const {
    return IsConfiguredKeyDown(config_.activation_modifier);
}

void WindowsKeyboardHookApp::EnsureLeftButtonReleased() {
    if (left_button_held_.exchange(false)) {
        AppendLogLine("EnsureLeftButtonReleased: left button up");
        mouse_->LeftUp();
    }
}

void WindowsKeyboardHookApp::RequestQuit() {
    running_.store(false);
    EnsureLeftButtonReleased();
    AppendLogLine("RequestQuit: emergency quit requested");
    PostThreadMessageW(main_thread_id_, kQuitMessage, 0, 0);
}

void WindowsKeyboardHookApp::PrintStartupSummary() const {
    std::cout << "mouse_mode=hold " << VirtualKeyName(config_.activation_modifier) << '\n';
    std::cout << "bindings=arrows move, " << VirtualKeyName(config_.bindings.left_click)
              << " left click, " << VirtualKeyName(config_.bindings.right_click) << " right click\n";
}

int WindowsKeyboardHookApp::NormalizeVirtualKey(DWORD vk_code, DWORD scan_code, DWORD flags) {
    // Some layouts/IMEs report the right control physical key with a non-control
    // virtual-key code but keep the standard control scan code.
    if (scan_code == 0x1D) {
        return (flags & LLKHF_EXTENDED) ? VK_RCONTROL : VK_LCONTROL;
    }

    if (scan_code == 0x38) {
        return (flags & LLKHF_EXTENDED) ? VK_RMENU : VK_LMENU;
    }

    if (vk_code == VK_CONTROL) {
        return (flags & LLKHF_EXTENDED) ? VK_RCONTROL : VK_LCONTROL;
    }
    if (vk_code == VK_MENU) {
        return (flags & LLKHF_EXTENDED) ? VK_RMENU : VK_LMENU;
    }
    return vk_code;
}

LRESULT CALLBACK WindowsKeyboardHookApp::HookProc(int code, WPARAM w_param, LPARAM l_param) {
    if (code < 0 || g_app == nullptr) {
        return CallNextHookEx(nullptr, code, w_param, l_param);
    }

    const auto info = ReadHookStruct(l_param);
    const int virtual_key = static_cast<int>(NormalizeVirtualKey(info.vkCode, info.scanCode, info.flags));

    const bool is_key_down = w_param == WM_KEYDOWN || w_param == WM_SYSKEYDOWN;
    const bool is_key_up = w_param == WM_KEYUP || w_param == WM_SYSKEYUP;
    const bool control_mode_active = g_app->IsControlModeActive();
    const bool suppress = g_app->ShouldSuppress(virtual_key);

    {
        std::ostringstream line;
        line << "HookProc:"
             << " w_param=" << static_cast<unsigned long long>(w_param)
             << " raw_vk=" << info.vkCode
             << " normalized_vk=" << virtual_key
             << " scan=" << info.scanCode
             << " flags=0x" << std::hex << info.flags << std::dec
             << " key_name=" << VirtualKeyName(virtual_key)
             << " ctrl_mode=" << (control_mode_active ? "1" : "0")
             << " suppress=" << (suppress ? "1" : "0");
        AppendLogLine(line.str());
    }

    if (is_key_down) {
        g_app->HandleKeyDown(virtual_key);
        if (g_app->IsEmergencyQuitPressed()) {
            g_app->RequestQuit();
            return 1;
        }
    } else if (is_key_up) {
        g_app->HandleKeyUp(virtual_key);
    }

    if (suppress) {
        return 1;
    }

    return CallNextHookEx(nullptr, code, w_param, l_param);
}

}  // namespace arrower

#endif
