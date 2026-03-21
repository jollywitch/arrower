#include "catch2/catch.hpp"

#include "arrower/input_state.hpp"

TEST_CASE("input state tracks first press and release transitions") {
    arrower::InputState state;

    CHECK(state.OnKeyDown(42));
    CHECK_FALSE(state.OnKeyDown(42));
    CHECK(state.IsDown(42));
    CHECK(state.OnKeyUp(42));
    CHECK_FALSE(state.IsDown(42));
    CHECK_FALSE(state.OnKeyUp(42));
}

