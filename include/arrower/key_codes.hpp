#pragma once

#include <optional>
#include <string>

namespace arrower {

std::optional<int> ParseVirtualKeyName(const std::string& name);
std::string VirtualKeyName(int virtual_key);
std::string DisplayVirtualKeyName(int virtual_key);

bool IsCtrlKey(int virtual_key);
bool IsAltKey(int virtual_key);
bool IsShiftKey(int virtual_key);
bool IsModifierKey(int virtual_key);
bool IsDirectionalKey(int virtual_key);

}  // namespace arrower
