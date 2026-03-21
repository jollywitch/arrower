#include "arrower/movement_engine.hpp"

#include <algorithm>
#include <cmath>

namespace arrower {

MovementEngine::MovementEngine(MovementConfig config) : config_(config) {}

CursorDelta MovementEngine::Tick(bool up, bool down, bool left, bool right) {
    up_ticks_ = up ? up_ticks_ + 1 : 0;
    down_ticks_ = down ? down_ticks_ + 1 : 0;
    left_ticks_ = left ? left_ticks_ + 1 : 0;
    right_ticks_ = right ? right_ticks_ + 1 : 0;

    const double horizontal = SpeedForTicks(right_ticks_) - SpeedForTicks(left_ticks_);
    const double vertical = SpeedForTicks(down_ticks_) - SpeedForTicks(up_ticks_);

    return {
        static_cast<int>(std::lround(horizontal)),
        static_cast<int>(std::lround(vertical)),
    };
}

void MovementEngine::Reset() {
    up_ticks_ = 0;
    down_ticks_ = 0;
    left_ticks_ = 0;
    right_ticks_ = 0;
}

double MovementEngine::SpeedForTicks(int ticks) const {
    if (ticks <= 0) {
        return 0.0;
    }

    const double speed = config_.base_speed_px_per_tick +
                         config_.acceleration_px_per_tick * static_cast<double>(ticks - 1);
    return std::min(speed, config_.max_speed_px_per_tick);
}

}  // namespace arrower

