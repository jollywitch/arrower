#pragma once

#include "arrower/config.hpp"

namespace arrower {

struct CursorDelta {
    int dx;
    int dy;
};

class MovementEngine {
public:
    explicit MovementEngine(MovementConfig config);

    CursorDelta Tick(bool up, bool down, bool left, bool right);
    void Reset();

private:
    double SpeedForTicks(int ticks) const;

    MovementConfig config_;
    int up_ticks_ = 0;
    int down_ticks_ = 0;
    int left_ticks_ = 0;
    int right_ticks_ = 0;
};

}  // namespace arrower

