#pragma once

#include <optional>
#include <string>

namespace arrower {

std::optional<int> ParseVirtualKeyName(const std::string& name);
std::string VirtualKeyName(int virtual_key);

bool IsCtrlKey(int virtual_key);
bool IsAltKey(int virtual_key);
bool IsDirectionalKey(int virtual_key);

}  // namespace arrower

