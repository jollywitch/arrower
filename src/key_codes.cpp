#include "arrower/key_codes.hpp"

#include <algorithm>
#include <cstdlib>
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
        {"leftshift", 0xA0},
        {"rightshift", 0xA1},
        {"shift", 0x10},
        {"control", 0x11},
        {"alt", 0x12},
        {"tab", 0x09},
        {"space", 0x20},
        {"pageup", 0x21},
        {"pagedown", 0x22},
        {"home", 0x24},
        {"end", 0x23},
        {"insert", 0x2D},
        {"delete", 0x2E},
        {"oemperiod", 0xBE},
        {"oem2", 0xBF},
        {"oemcomma", 0xBC},
        {"oemminus", 0xBD},
        {"oemplus", 0xBB},
        {"oemsemicolon", 0xBA},
        {"oemquotes", 0xDE},
        {"oem4", 0xDB},
        {"oem6", 0xDD},
        {"oem5", 0xDC},
        {"oem3", 0xC0},
    };
    return map;
}

}  // namespace

std::optional<int> ParseVirtualKeyName(const std::string& name) {
    if (name.size() > 3 && Normalize(name.substr(0, 3)) == "vk_") {
        return std::atoi(name.substr(3).c_str());
    }

    if (name.size() == 1) {
        const unsigned char ch = static_cast<unsigned char>(name.front());
        if (std::isdigit(ch) || (std::toupper(ch) >= 'A' && std::toupper(ch) <= 'Z')) {
            return static_cast<int>(std::toupper(ch));
        }
    }

    const auto it = NameToKeyMap().find(Normalize(name));
    if (it == NameToKeyMap().end()) {
        if (name.size() > 1 && std::toupper(static_cast<unsigned char>(name[0])) == 'F') {
            const int function_index = std::atoi(name.substr(1).c_str());
            if (function_index >= 1 && function_index <= 24) {
                return 0x6F + function_index;
            }
        }
        return std::nullopt;
    }

    return it->second;
}

std::string VirtualKeyName(int virtual_key) {
    switch (virtual_key) {
        case 0x09:
            return "Tab";
        case 0x10:
            return "Shift";
        case 0x11:
            return "Control";
        case 0x12:
            return "Alt";
        case 0x20:
            return "Space";
        case 0x21:
            return "PageUp";
        case 0x22:
            return "PageDown";
        case 0x23:
            return "End";
        case 0x24:
            return "Home";
        case 0x25:
            return "Left";
        case 0x26:
            return "Up";
        case 0x27:
            return "Right";
        case 0x28:
            return "Down";
        case 0x2D:
            return "Insert";
        case 0x2E:
            return "Delete";
        case 0x1B:
            return "Escape";
        case 0xA0:
            return "LeftShift";
        case 0xA1:
            return "RightShift";
        case 0xA2:
            return "LeftCtrl";
        case 0xA3:
            return "RightCtrl";
        case 0xA4:
            return "LeftAlt";
        case 0xA5:
            return "RightAlt";
        case 0xBA:
            return "OemSemicolon";
        case 0xBB:
            return "OemPlus";
        case 0xBC:
            return "OemComma";
        case 0xBD:
            return "OemMinus";
        case 0xBE:
            return "OemPeriod";
        case 0xBF:
            return "Oem2";
        case 0xC0:
            return "Oem3";
        case 0xDB:
            return "Oem4";
        case 0xDC:
            return "Oem5";
        case 0xDD:
            return "Oem6";
        case 0xDE:
            return "OemQuotes";
        default:
            if ((virtual_key >= '0' && virtual_key <= '9') || (virtual_key >= 'A' && virtual_key <= 'Z')) {
                return std::string(1, static_cast<char>(virtual_key));
            }
            if (virtual_key >= 0x70 && virtual_key <= 0x87) {
                return "F" + std::to_string(virtual_key - 0x6F);
            }
            return "VK_" + std::to_string(virtual_key);
    }
}

std::string DisplayVirtualKeyName(int virtual_key) {
    switch (virtual_key) {
        case 0xA2:
            return "Left Ctrl";
        case 0xA3:
            return "Right Ctrl";
        case 0xA4:
            return "Left Alt";
        case 0xA5:
            return "Right Alt";
        case 0xA0:
            return "Left Shift";
        case 0xA1:
            return "Right Shift";
        case 0xBE:
            return ".";
        case 0xBF:
            return "/";
        case 0xBC:
            return ",";
        case 0xBA:
            return ";";
        case 0xBB:
            return "=";
        case 0xBD:
            return "-";
        default:
            return VirtualKeyName(virtual_key);
    }
}

bool IsCtrlKey(int virtual_key) {
    return virtual_key == 0xA2 || virtual_key == 0xA3;
}

bool IsAltKey(int virtual_key) {
    return virtual_key == 0xA4 || virtual_key == 0xA5;
}

bool IsShiftKey(int virtual_key) {
    return virtual_key == 0xA0 || virtual_key == 0xA1 || virtual_key == 0x10;
}

bool IsModifierKey(int virtual_key) {
    return IsCtrlKey(virtual_key) || IsAltKey(virtual_key) || IsShiftKey(virtual_key) ||
           virtual_key == 0x11 || virtual_key == 0x12;
}

bool IsDirectionalKey(int virtual_key) {
    return virtual_key >= 0x25 && virtual_key <= 0x28;
}

}  // namespace arrower
