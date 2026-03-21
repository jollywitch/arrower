#pragma once

#include <filesystem>
#include <string>

namespace arrower {

struct Bindings {
    int up;
    int down;
    int left;
    int right;
    int left_click;
    int right_click;
};

struct MovementConfig {
    double base_speed_px_per_tick;
    double acceleration_px_per_tick;
    double max_speed_px_per_tick;
    int update_rate_hz;
    int drag_update_rate_hz;
    int scroll_update_rate_hz;
};

struct Config {
    int activation_modifier;
    Bindings bindings;
    MovementConfig movement;
};

struct ConfigLoadResult {
    bool success;
    bool used_defaults;
    Config config;
    std::string message;
};

Config DefaultConfig();
ConfigLoadResult ParseConfigText(const std::string& text);
ConfigLoadResult LoadConfigFile(const std::filesystem::path& path);
std::string DescribeConfig(const Config& config);

}  // namespace arrower
