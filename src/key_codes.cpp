#include "arrower/key_codes.hpp"

#include <algorithm>
#include <cctype>
#include <unordered_map>

namespace arrower {

namespace {

std::string Normalize(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    return value;
}

const std::unordered_map<std::string, int>& NameToKeyMap() {
    static const std::unordered_map<std::string, int> map = {
        {"left", 0x25},
        {"up", 0x26},
        {"right", 0x27},
        {"down", 0x28},
        {"escape", 0x1B},
        {"leftctrl", 0xA2},
        {"rightctrl", 0xA3},
        {"leftalt", 0xA4},
        {"rightalt", 0xA5},
        {"oemperiod", 0xBE},
        {"oem2", 0xBF},
    };
    return map;
}

}  // namespace

std::optional<int> ParseVirtualKeyName(const std::string& name) {
    const auto it = NameToKeyMap().find(Normalize(name));
    if (it == NameToKeyMap().end()) {
        return std::nullopt;
    }

    return it->second;
}

std::string VirtualKeyName(int virtual_key) {
    switch (virtual_key) {
        case 0x25:
            return "Left";
        case 0x26:
            return "Up";
        case 0x27:
            return "Right";
        case 0x28:
            return "Down";
        case 0x1B:
            return "Escape";
        case 0xA2:
            return "LeftCtrl";
        case 0xA3:
            return "RightCtrl";
        case 0xA4:
            return "LeftAlt";
        case 0xA5:
            return "RightAlt";
        case 0xBE:
            return "OemPeriod";
        case 0xBF:
            return "Oem2";
        default:
            return "VK_" + std::to_string(virtual_key);
    }
}

bool IsCtrlKey(int virtual_key) {
    return virtual_key == 0xA2 || virtual_key == 0xA3;
}

bool IsAltKey(int virtual_key) {
    return virtual_key == 0xA4 || virtual_key == 0xA5;
}

bool IsDirectionalKey(int virtual_key) {
    return virtual_key >= 0x25 && virtual_key <= 0x28;
}

}  // namespace arrower

