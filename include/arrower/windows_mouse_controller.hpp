#pragma once

#ifdef _WIN32

namespace arrower {

class WindowsMouseController {
public:
    void MoveBy(int dx, int dy) const;
    void LeftDown() const;
    void LeftUp() const;
    void LeftClick() const;
    void RightClick() const;
    void ScrollVertical(int wheel_delta) const;

private:
    void SendMouseEvent(unsigned long flags, long dx = 0, long dy = 0) const;
};

}  // namespace arrower

#endif
