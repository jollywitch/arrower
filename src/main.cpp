#ifdef _WIN32

#include <filesystem>
#include <iostream>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "arrower/config.hpp"
#include "arrower/windows_keyboard_hook.hpp"

namespace {

std::filesystem::path ExecutableDirectory() {
    wchar_t path_buffer[MAX_PATH];
    const DWORD length = GetModuleFileNameW(nullptr, path_buffer, MAX_PATH);
    if (length == 0 || length == MAX_PATH) {
        return std::filesystem::current_path();
    }

    return std::filesystem::path(path_buffer).parent_path();
}

}  // namespace

int main() {
    const auto executable_dir = ExecutableDirectory();
    const auto config_path = executable_dir / "config.json";

    const auto config_result = arrower::LoadConfigFile(config_path);
    if (!config_result.success) {
        std::cerr << "arrower: " << config_result.message << '\n';
        return 1;
    }

    std::cout << "arrower startup\n";
    std::cout << "config: " << config_path.string() << '\n';
    std::cout << config_result.message << '\n';
    std::cout << arrower::DescribeConfig(config_result.config) << '\n';
    std::cout << "emergency_quit=Ctrl+Alt+Esc\n";

    arrower::WindowsKeyboardHookApp app(config_result.config);
    return app.Run();
}

#else

int main() {
    return 1;
}

#endif
