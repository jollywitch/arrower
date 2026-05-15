#include "arrower/app_paths.hpp"

#ifdef _WIN32

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#endif

namespace arrower {

std::filesystem::path ExecutableDirectory() {
#ifdef _WIN32
    wchar_t path_buffer[MAX_PATH];
    const DWORD length = GetModuleFileNameW(nullptr, path_buffer, MAX_PATH);
    if (length == 0 || length == MAX_PATH) {
        return std::filesystem::current_path();
    }

    return std::filesystem::path(path_buffer).parent_path();
#else
    return std::filesystem::current_path();
#endif
}

std::filesystem::path DefaultConfigPath() {
    return ExecutableDirectory() / "config.json";
}

}  // namespace arrower
