#ifdef _WIN32

#include "arrower/windows_mouse_controller.hpp"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

namespace arrower {

void WindowsMouseController::MoveBy(int dx, int dy) const {
    SendMouseEvent(MOUSEEVENTF_MOVE, dx, dy);
}

void WindowsMouseController::LeftDown() const {
    SendMouseEvent(MOUSEEVENTF_LEFTDOWN);
}

void WindowsMouseController::LeftUp() const {
    SendMouseEvent(MOUSEEVENTF_LEFTUP);
}

void WindowsMouseController::LeftClick() const {
    LeftDown();
    LeftUp();
}

void WindowsMouseController::RightClick() const {
    SendMouseEvent(MOUSEEVENTF_RIGHTDOWN);
    SendMouseEvent(MOUSEEVENTF_RIGHTUP);
}

void WindowsMouseController::ScrollVertical(int wheel_delta) const {
    INPUT input{};
    input.type = INPUT_MOUSE;
    input.mi.dwFlags = MOUSEEVENTF_WHEEL;
    input.mi.mouseData = static_cast<DWORD>(wheel_delta);
    SendInput(1, &input, sizeof(INPUT));
}

void WindowsMouseController::SendMouseEvent(unsigned long flags, long dx, long dy) const {
    INPUT input{};
    input.type = INPUT_MOUSE;
    input.mi.dx = dx;
    input.mi.dy = dy;
    input.mi.dwFlags = flags;
    SendInput(1, &input, sizeof(INPUT));
}

}  // namespace arrower

#endif
