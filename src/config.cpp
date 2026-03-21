#include "arrower/config.hpp"

#include <fstream>
#include <sstream>

#include "arrower/key_codes.hpp"
#include "picojson.h"

namespace arrower {

namespace {

ConfigLoadResult ErrorResult(const std::string& message) {
    return {false, false, DefaultConfig(), message};
}

double RequireNumber(const picojson::value& value, const char* key, std::string& error) {
    const auto& object = value.get<picojson::object>();
    const auto it = object.find(key);
    if (it == object.end() || !it->second.is<double>()) {
        error = std::string("Missing or invalid numeric key: ") + key;
        return 0.0;
    }

    return it->second.get<double>();
}

int RequireInteger(const picojson::value& value, const char* key, std::string& error) {
    const double number = RequireNumber(value, key, error);
    if (!error.empty()) {
        return 0;
    }

    return static_cast<int>(number);
}

int RequireKeyCode(const picojson::value& value, const char* key, std::string& error) {
    const auto& object = value.get<picojson::object>();
    const auto it = object.find(key);
    if (it == object.end() || !it->second.is<std::string>()) {
        error = std::string("Missing or invalid key binding: ") + key;
        return 0;
    }

    const auto parsed = ParseVirtualKeyName(it->second.get<std::string>());
    if (!parsed.has_value()) {
        error = std::string("Unknown virtual key name for ") + key + ": " + it->second.get<std::string>();
        return 0;
    }

    return *parsed;
}

}  // namespace

Config DefaultConfig() {
    return {
        *ParseVirtualKeyName("RightCtrl"),
        {
            *ParseVirtualKeyName("Up"),
            *ParseVirtualKeyName("Down"),
            *ParseVirtualKeyName("Left"),
            *ParseVirtualKeyName("Right"),
            *ParseVirtualKeyName("OemPeriod"),
            *ParseVirtualKeyName("Oem2"),
        },
        {
            6.0,
            0.8,
            28.0,
            120,
            24,
            18,
        },
    };
}

ConfigLoadResult ParseConfigText(const std::string& text) {
    picojson::value root;
    const std::string parse_error = picojson::parse(root, text);
    if (!parse_error.empty()) {
        return ErrorResult("Failed to parse config JSON: " + parse_error);
    }

    if (!root.is<picojson::object>()) {
        return ErrorResult("Config root must be a JSON object");
    }

    std::string error;
    const auto& root_object = root.get<picojson::object>();

    const auto activation_it = root_object.find("activation_modifier");
    if (activation_it == root_object.end() || !activation_it->second.is<std::string>()) {
        return ErrorResult("Missing or invalid activation_modifier");
    }

    const auto activation_modifier = ParseVirtualKeyName(activation_it->second.get<std::string>());
    if (!activation_modifier.has_value()) {
        return ErrorResult("Unknown activation_modifier: " + activation_it->second.get<std::string>());
    }

    const auto bindings_it = root_object.find("bindings");
    if (bindings_it == root_object.end() || !bindings_it->second.is<picojson::object>()) {
        return ErrorResult("Missing or invalid bindings object");
    }

    const auto movement_it = root_object.find("movement");
    if (movement_it == root_object.end() || !movement_it->second.is<picojson::object>()) {
        return ErrorResult("Missing or invalid movement object");
    }

    Bindings bindings{};
    bindings.up = RequireKeyCode(bindings_it->second, "up", error);
    bindings.down = RequireKeyCode(bindings_it->second, "down", error);
    bindings.left = RequireKeyCode(bindings_it->second, "left", error);
    bindings.right = RequireKeyCode(bindings_it->second, "right", error);
    bindings.left_click = RequireKeyCode(bindings_it->second, "left_click", error);
    bindings.right_click = RequireKeyCode(bindings_it->second, "right_click", error);
    if (!error.empty()) {
        return ErrorResult(error);
    }

    MovementConfig movement{};
    movement.base_speed_px_per_tick = RequireNumber(movement_it->second, "base_speed_px_per_tick", error);
    movement.acceleration_px_per_tick = RequireNumber(movement_it->second, "acceleration_px_per_tick", error);
    movement.max_speed_px_per_tick = RequireNumber(movement_it->second, "max_speed_px_per_tick", error);
    movement.update_rate_hz = RequireInteger(movement_it->second, "update_rate_hz", error);
    movement.drag_update_rate_hz = RequireInteger(movement_it->second, "drag_update_rate_hz", error);
    movement.scroll_update_rate_hz = RequireInteger(movement_it->second, "scroll_update_rate_hz", error);
    if (!error.empty()) {
        return ErrorResult(error);
    }

    if (movement.base_speed_px_per_tick < 0.0 || movement.acceleration_px_per_tick < 0.0 ||
        movement.max_speed_px_per_tick < movement.base_speed_px_per_tick || movement.update_rate_hz <= 0 ||
        movement.drag_update_rate_hz <= 0 || movement.scroll_update_rate_hz <= 0) {
        return ErrorResult("Movement settings must be non-negative, max >= base, and all update rates > 0");
    }

    return {true, false, { *activation_modifier, bindings, movement }, "Loaded config.json"};
}

ConfigLoadResult LoadConfigFile(const std::filesystem::path& path) {
    std::ifstream input(path);
    if (!input.is_open()) {
        return {true, true, DefaultConfig(), "Config file not found. Using built-in defaults."};
    }

    std::ostringstream buffer;
    buffer << input.rdbuf();
    auto result = ParseConfigText(buffer.str());
    if (result.success) {
        result.message = "Loaded config from " + path.string();
    }

    return result;
}

std::string DescribeConfig(const Config& config) {
    std::ostringstream output;
    output << "activation_modifier=" << VirtualKeyName(config.activation_modifier)
           << ", movement=[up=" << VirtualKeyName(config.bindings.up)
           << ", down=" << VirtualKeyName(config.bindings.down)
           << ", left=" << VirtualKeyName(config.bindings.left)
           << ", right=" << VirtualKeyName(config.bindings.right)
           << "], clicks=[left=" << VirtualKeyName(config.bindings.left_click)
           << ", right=" << VirtualKeyName(config.bindings.right_click)
           << "], speed=[base=" << config.movement.base_speed_px_per_tick
           << ", accel=" << config.movement.acceleration_px_per_tick
           << ", max=" << config.movement.max_speed_px_per_tick
           << ", hz=" << config.movement.update_rate_hz
           << ", drag_hz=" << config.movement.drag_update_rate_hz
           << ", scroll_hz=" << config.movement.scroll_update_rate_hz << "]";
    return output.str();
}

}  // namespace arrower
