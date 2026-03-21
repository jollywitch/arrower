#include "arrower/input_state.hpp"

namespace arrower {

bool InputState::OnKeyDown(int virtual_key) {
    if (virtual_key < 0 || virtual_key >= 256) {
        return false;
    }

    const bool was_down = keys_[virtual_key].exchange(true);
    return !was_down;
}

bool InputState::OnKeyUp(int virtual_key) {
    if (virtual_key < 0 || virtual_key >= 256) {
        return false;
    }

    const bool was_down = keys_[virtual_key].exchange(false);
    return was_down;
}

bool InputState::IsDown(int virtual_key) const {
    if (virtual_key < 0 || virtual_key >= 256) {
        return false;
    }

    return keys_[virtual_key].load();
}

}  // namespace arrower
