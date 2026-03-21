#include "catch2/catch.hpp"

#include "arrower/movement_engine.hpp"

TEST_CASE("movement accelerates until capped") {
    arrower::MovementEngine engine({6.0, 0.8, 28.0, 120});

    const auto tick1 = engine.Tick(false, false, false, true);
    const auto tick2 = engine.Tick(false, false, false, true);
    const auto tick40 = [&engine]() {
        arrower::CursorDelta last{};
        for (int i = 0; i < 38; ++i) {
            last = engine.Tick(false, false, false, true);
        }
        return last;
    }();

    CHECK(tick1.dx == 6);
    CHECK(tick2.dx == 7);
    CHECK(tick40.dx == 28);
}

TEST_CASE("releasing movement resets acceleration") {
    arrower::MovementEngine engine({6.0, 0.8, 28.0, 120});

    engine.Tick(false, false, false, true);
    engine.Tick(false, false, false, true);
    CHECK(engine.Tick(false, false, false, false).dx == 0);

    const auto reset_tick = engine.Tick(false, false, false, true);
    CHECK(reset_tick.dx == 6);
}

