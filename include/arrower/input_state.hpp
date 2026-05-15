#pragma once

#include <atomic>

namespace arrower {

struct MovementKeys {
    bool up = false;
    bool down = false;
    bool left = false;
    bool right = false;
};

class InputState {
public:
    bool OnKeyDown(int virtual_key);
    bool OnKeyUp(int virtual_key);
    bool IsDown(int virtual_key) const;
    void Reset();

private:
    std::atomic_bool keys_[256]{};
};

}  // namespace arrower
