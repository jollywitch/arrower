#define CATCH_CONFIG_MAIN
#include "catch2/catch.hpp"

#include "arrower/config.hpp"
#include "arrower/key_codes.hpp"

TEST_CASE("missing config falls back to defaults") {
    const auto result = arrower::LoadConfigFile("does-not-exist.json");

    REQUIRE(result.success);
    REQUIRE(result.used_defaults);
    CHECK(result.config.activation_modifier == *arrower::ParseVirtualKeyName("RightCtrl"));
    CHECK(result.config.movement.update_rate_hz == 120);
    CHECK(result.config.movement.drag_update_rate_hz == 24);
    CHECK(result.config.movement.scroll_update_rate_hz == 18);
}

TEST_CASE("valid config text parses bindings and movement") {
    const std::string text = R"({
        "activation_modifier": "RightCtrl",
        "bindings": {
            "up": "Up",
            "down": "Down",
            "left": "Left",
            "right": "Right",
            "left_click": "OemPeriod",
            "right_click": "Oem2"
        },
        "movement": {
            "base_speed_px_per_tick": 7.0,
            "acceleration_px_per_tick": 0.5,
            "max_speed_px_per_tick": 20.0,
            "update_rate_hz": 90,
            "drag_update_rate_hz": 35,
            "scroll_update_rate_hz": 12
        }
    })";

    const auto result = arrower::ParseConfigText(text);

    REQUIRE(result.success);
    CHECK_FALSE(result.used_defaults);
    CHECK(result.config.movement.base_speed_px_per_tick == Approx(7.0));
    CHECK(result.config.movement.acceleration_px_per_tick == Approx(0.5));
    CHECK(result.config.movement.max_speed_px_per_tick == Approx(20.0));
    CHECK(result.config.movement.update_rate_hz == 90);
    CHECK(result.config.movement.drag_update_rate_hz == 35);
    CHECK(result.config.movement.scroll_update_rate_hz == 12);
}

TEST_CASE("invalid key name fails clearly") {
    const std::string text = R"({
        "activation_modifier": "Bogus",
        "bindings": {
            "up": "Up",
            "down": "Down",
            "left": "Left",
            "right": "Right",
            "left_click": "OemPeriod",
            "right_click": "Oem2"
        },
        "movement": {
            "base_speed_px_per_tick": 6.0,
            "acceleration_px_per_tick": 0.8,
            "max_speed_px_per_tick": 28.0,
            "update_rate_hz": 120,
            "drag_update_rate_hz": 24,
            "scroll_update_rate_hz": 18
        }
    })";

    const auto result = arrower::ParseConfigText(text);

    CHECK_FALSE(result.success);
    CHECK(result.message.find("activation_modifier") != std::string::npos);
}

TEST_CASE("config serializes and parses round-trip") {
    const auto original = arrower::DefaultConfig();
    const auto serialized = arrower::SerializeConfig(original);
    const auto reparsed = arrower::ParseConfigText(serialized);

    REQUIRE(reparsed.success);
    CHECK(reparsed.config.activation_modifier == original.activation_modifier);
    CHECK(reparsed.config.bindings.left_click == original.bindings.left_click);
    CHECK(reparsed.config.bindings.right_click == original.bindings.right_click);
    CHECK(reparsed.config.movement.drag_update_rate_hz == original.movement.drag_update_rate_hz);
    CHECK(reparsed.config.movement.scroll_update_rate_hz == original.movement.scroll_update_rate_hz);
}
