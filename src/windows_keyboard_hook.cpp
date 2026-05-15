#ifdef _WIN32

#include "arrower/windows_keyboard_hook.hpp"

#include <chrono>
#include <thread>

#include "arrower/key_codes.hpp"
#include "arrower/windows_mouse_controller.hpp"

namespace arrower {

namespace {

constexpr UINT kServiceStopMessage = WM_APP + 1;
constexpr int kGenericCtrl = VK_CONTROL;
constexpr int kGenericAlt = VK_MENU;
constexpr DWORD kWheelDelta = 120;

WindowsKeyboardHookService* g_service = nullptr;

bool PhysicalKeyDown(int virtual_key) {
    return (GetAsyncKeyState(virtual_key) & 0x8000) != 0;
}

KBDLLHOOKSTRUCT ReadHookStruct(LPARAM value) {
    return *reinterpret_cast<const KBDLLHOOKSTRUCT*>(value);
}

}  // namespace

WindowsKeyboardHookService::WindowsKeyboardHookService(Config config)
    : config_(config), active_config_(config), movement_engine_(config.movement) {}

WindowsKeyboardHookService::~WindowsKeyboardHookService() {
    Stop();
}

bool WindowsKeyboardHookService::Start() {
    std::unique_lock<std::mutex> lock(state_mutex_);
    if (running_.load()) {
        return true;
    }

    startup_complete_ = false;
    startup_succeeded_ = false;
    last_error_.clear();

    const Config config = config_;
    hook_thread_ = std::thread([this, config]() {
        ServiceThreadMain(config);
    });

    startup_cv_.wait(lock, [this]() { return startup_complete_; });
    const bool success = startup_succeeded_;
    lock.unlock();

    if (!success && hook_thread_.joinable()) {
        hook_thread_.join();
    }

    return success;
}

bool WindowsKeyboardHookService::Stop() {
    bool stop_requested = false;
    const auto stop_event = reinterpret_cast<HANDLE>(stop_event_.load());
    if (stop_event != nullptr) {
        if (SetEvent(stop_event)) {
            stop_requested = true;
        } else {
            SetLastError("Failed to signal keyboard hook stop event. SetEvent error: " +
                         std::to_string(GetLastError()));
        }
    }

    const DWORD thread_id = hook_thread_id_.load();
    if (thread_id != 0) {
        if (PostThreadMessageW(thread_id, kServiceStopMessage, 0, 0)) {
            stop_requested = true;
        } else if (!stop_requested) {
            SetLastError("Failed to stop keyboard hook thread. PostThreadMessageW error: " +
                         std::to_string(GetLastError()));
            return false;
        }
    }

    if (!stop_requested && running_.load()) {
        SetLastError("Failed to stop keyboard hook thread: thread is running but no stop target is available.");
        return false;
    }

    if (hook_thread_.joinable()) {
        hook_thread_.join();
    }

    return true;
}

bool WindowsKeyboardHookService::ReloadConfig(const Config& config) {
    const bool was_running = IsRunning();
    if (!Stop()) {
        return false;
    }
    SetConfig(config);
    if (!was_running) {
        return true;
    }
    return Start();
}

void WindowsKeyboardHookService::SetConfig(const Config& config) {
    std::lock_guard<std::mutex> lock(state_mutex_);
    config_ = config;
}

Config WindowsKeyboardHookService::CurrentConfig() const {
    std::lock_guard<std::mutex> lock(state_mutex_);
    return config_;
}

bool WindowsKeyboardHookService::IsRunning() const {
    return running_.load();
}

std::string WindowsKeyboardHookService::LastError() const {
    std::lock_guard<std::mutex> lock(state_mutex_);
    return last_error_;
}

void WindowsKeyboardHookService::ServiceThreadMain(Config config) {
    {
        std::lock_guard<std::mutex> lock(state_mutex_);
        active_config_ = config;
        startup_complete_ = false;
        startup_succeeded_ = false;
    }

    input_state_.Reset();
    movement_engine_ = MovementEngine(active_config_.movement);
    mouse_ = std::make_unique<WindowsMouseController>();
    left_button_held_.store(false);
    right_click_pending_.store(false);
    scroll_mode_used_.store(false);

    const HANDLE stop_event = CreateEventW(nullptr, TRUE, FALSE, nullptr);
    if (stop_event == nullptr) {
        SetLastError("Failed to create keyboard hook stop event. CreateEventW error: " +
                     std::to_string(GetLastError()));
        {
            std::lock_guard<std::mutex> lock(state_mutex_);
            startup_complete_ = true;
            startup_succeeded_ = false;
        }
        startup_cv_.notify_all();
        mouse_.reset();
        return;
    }
    stop_event_.store(stop_event);

    MSG queue_probe{};
    PeekMessageW(&queue_probe, nullptr, WM_USER, WM_USER, PM_NOREMOVE);

    hook_thread_id_.store(GetCurrentThreadId());
    g_service = this;

    if (!InstallHook()) {
        {
            std::lock_guard<std::mutex> lock(state_mutex_);
            startup_complete_ = true;
            startup_succeeded_ = false;
        }
        startup_cv_.notify_all();
        hook_thread_id_.store(0);
        g_service = nullptr;
        stop_event_.store(nullptr);
        CloseHandle(stop_event);
        mouse_.reset();
        return;
    }

    running_.store(true);
    {
        std::lock_guard<std::mutex> lock(state_mutex_);
        startup_complete_ = true;
        startup_succeeded_ = true;
    }
    startup_cv_.notify_all();

    StartMovementThread();

    MSG msg{};
    bool stop_requested = false;
    while (!stop_requested) {
        const DWORD wait_result = MsgWaitForMultipleObjects(1, &stop_event, FALSE, INFINITE, QS_ALLINPUT);
        if (wait_result == WAIT_OBJECT_0) {
            break;
        }

        if (wait_result != WAIT_OBJECT_0 + 1) {
            SetLastError("Keyboard hook message loop wait failed. MsgWaitForMultipleObjects result: " +
                         std::to_string(wait_result));
            break;
        }

        while (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE)) {
            if (msg.message == kServiceStopMessage) {
                stop_requested = true;
                break;
            }

            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
    }

    running_.store(false);
    EnsureLeftButtonReleased();
    StopMovementThread();
    UninstallHook();
    mouse_.reset();
    hook_thread_id_.store(0);
    g_service = nullptr;
    stop_event_.store(nullptr);
    CloseHandle(stop_event);
}

bool WindowsKeyboardHookService::InstallHook() {
    HHOOK hook = SetWindowsHookExW(WH_KEYBOARD_LL, HookProc, GetModuleHandleW(nullptr), 0);
    if (hook == nullptr) {
        SetLastError("Failed to install keyboard hook");
        return false;
    }

    hook_ = hook;
    return true;
}

void WindowsKeyboardHookService::UninstallHook() {
    if (hook_ != nullptr) {
        UnhookWindowsHookEx(reinterpret_cast<HHOOK>(hook_));
        hook_ = nullptr;
    }
}

void WindowsKeyboardHookService::StartMovementThread() {
    movement_thread_ = std::thread([this]() {
        const auto tick_interval =
            std::chrono::duration<double>(1.0 / static_cast<double>(active_config_.movement.update_rate_hz));
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

            if (IsConfiguredKeyDown(active_config_.bindings.right_click)) {
                const auto now = std::chrono::steady_clock::now();
                const auto scroll_interval = std::chrono::duration<double>(
                    1.0 / static_cast<double>(active_config_.movement.scroll_update_rate_hz));
                if (now - last_scroll_tick >= scroll_interval) {
                    if (IsConfiguredKeyDown(active_config_.bindings.up)) {
                        scroll_mode_used_.store(true);
                        mouse_->ScrollVertical(static_cast<int>(kWheelDelta));
                        last_scroll_tick = now;
                    } else if (IsConfiguredKeyDown(active_config_.bindings.down)) {
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
                    1.0 / static_cast<double>(active_config_.movement.drag_update_rate_hz));
                if (now - last_drag_tick < drag_interval) {
                    std::this_thread::sleep_for(tick_interval);
                    continue;
                }
                last_drag_tick = now;
            } else {
                last_drag_tick = now;
            }

            const auto delta = movement_engine_.Tick(
                IsConfiguredKeyDown(active_config_.bindings.up),
                IsConfiguredKeyDown(active_config_.bindings.down),
                IsConfiguredKeyDown(active_config_.bindings.left),
                IsConfiguredKeyDown(active_config_.bindings.right));

            if ((delta.dx != 0 || delta.dy != 0) && mouse_ != nullptr) {
                mouse_->MoveBy(delta.dx, delta.dy);
            }

            std::this_thread::sleep_for(tick_interval);
        }
    });
}

void WindowsKeyboardHookService::StopMovementThread() {
    if (movement_thread_.joinable()) {
        movement_thread_.join();
    }
}

void WindowsKeyboardHookService::HandleKeyDown(int virtual_key) {
    const bool first_press = input_state_.OnKeyDown(virtual_key);

    if (!IsControlModeActive() || !first_press) {
        return;
    }

    if (virtual_key == active_config_.bindings.left_click) {
        mouse_->LeftDown();
        left_button_held_.store(true);
    } else if (virtual_key == active_config_.bindings.right_click) {
        right_click_pending_.store(true);
        scroll_mode_used_.store(false);
    }
}

void WindowsKeyboardHookService::HandleKeyUp(int virtual_key) {
    const bool was_down = input_state_.OnKeyUp(virtual_key);
    if (!was_down) {
        return;
    }

    if (virtual_key == active_config_.bindings.left_click || virtual_key == active_config_.activation_modifier) {
        EnsureLeftButtonReleased();
    }

    if (virtual_key == active_config_.bindings.right_click) {
        const bool right_click_pending = right_click_pending_.exchange(false);
        const bool scroll_mode_used = scroll_mode_used_.exchange(false);
        if (right_click_pending && !scroll_mode_used && IsConfiguredKeyDown(active_config_.activation_modifier) &&
            mouse_ != nullptr) {
            mouse_->RightClick();
        }
    }
}

bool WindowsKeyboardHookService::ShouldSuppress(int virtual_key) const {
    if (IsEmergencyQuitPressed()) {
        return true;
    }

    if (virtual_key == active_config_.activation_modifier) {
        return true;
    }

    if (!IsControlModeActive()) {
        return false;
    }

    if (virtual_key == active_config_.bindings.right_click) {
        return true;
    }

    if (IsConfiguredKeyDown(active_config_.bindings.right_click) &&
        (virtual_key == active_config_.bindings.up || virtual_key == active_config_.bindings.down)) {
        return true;
    }

    return virtual_key == active_config_.bindings.up || virtual_key == active_config_.bindings.down ||
           virtual_key == active_config_.bindings.left || virtual_key == active_config_.bindings.right ||
           virtual_key == active_config_.bindings.left_click;
}

bool WindowsKeyboardHookService::IsEmergencyQuitPressed() const {
    return (IsConfiguredKeyDown(*ParseVirtualKeyName("LeftCtrl")) ||
            IsConfiguredKeyDown(*ParseVirtualKeyName("RightCtrl"))) &&
           (IsConfiguredKeyDown(*ParseVirtualKeyName("LeftAlt")) ||
            IsConfiguredKeyDown(*ParseVirtualKeyName("RightAlt"))) &&
           IsConfiguredKeyDown(*ParseVirtualKeyName("Escape"));
}

bool WindowsKeyboardHookService::IsConfiguredKeyDown(int configured_virtual_key) const {
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

bool WindowsKeyboardHookService::IsControlModeActive() const {
    return IsConfiguredKeyDown(active_config_.activation_modifier);
}

void WindowsKeyboardHookService::EnsureLeftButtonReleased() {
    if (left_button_held_.exchange(false) && mouse_ != nullptr) {
        mouse_->LeftUp();
    }
}

void WindowsKeyboardHookService::RequestQuit() {
    EnsureLeftButtonReleased();
    const DWORD thread_id = hook_thread_id_.load();
    if (thread_id != 0) {
        PostThreadMessageW(thread_id, kServiceStopMessage, 0, 0);
    }
}

void WindowsKeyboardHookService::SetLastError(const std::string& error) {
    std::lock_guard<std::mutex> lock(state_mutex_);
    last_error_ = error;
}

int WindowsKeyboardHookService::NormalizeVirtualKey(DWORD vk_code, DWORD scan_code, DWORD flags) {
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

    return static_cast<int>(vk_code);
}

LRESULT CALLBACK WindowsKeyboardHookService::HookProc(int code, WPARAM w_param, LPARAM l_param) {
    if (code < 0 || g_service == nullptr) {
        return CallNextHookEx(nullptr, code, w_param, l_param);
    }

    const auto info = ReadHookStruct(l_param);
    const int virtual_key = NormalizeVirtualKey(info.vkCode, info.scanCode, info.flags);

    const bool is_key_down = w_param == WM_KEYDOWN || w_param == WM_SYSKEYDOWN;
    const bool is_key_up = w_param == WM_KEYUP || w_param == WM_SYSKEYUP;

    if (is_key_down) {
        g_service->HandleKeyDown(virtual_key);
        if (g_service->IsEmergencyQuitPressed()) {
            g_service->RequestQuit();
            return 1;
        }
    } else if (is_key_up) {
        g_service->HandleKeyUp(virtual_key);
    }

    if (g_service->ShouldSuppress(virtual_key)) {
        return 1;
    }

    return CallNextHookEx(nullptr, code, w_param, l_param);
}

}  // namespace arrower

#endif
