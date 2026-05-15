#pragma once

#include <filesystem>

namespace arrower {

std::filesystem::path ExecutableDirectory();
std::filesystem::path DefaultConfigPath();

}  // namespace arrower
