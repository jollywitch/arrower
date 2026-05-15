cmake_minimum_required(VERSION 3.20)

project(arrower VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(arrower_core
    src/app_paths.cpp
    src/config.cpp
    src/input_state.cpp
    src/key_codes.cpp
    src/movement_engine.cpp
)

target_include_directories(arrower_core
    PUBLIC
        ${PROJECT_SOURCE_DIR}/include
        ${PROJECT_SOURCE_DIR}/third_party/picojson
)

if(WIN32)
    find_package(Qt6 COMPONENTS Widgets QUIET)

    if(Qt6_FOUND)
        set(CMAKE_AUTOMOC ON)
        add_executable(arrower WIN32
            src/main.cpp
            src/binding_capture_button.hpp
            src/binding_capture_button.cpp
            src/settings_window.hpp
            src/settings_window.cpp
            src/tray_application.hpp
            src/tray_application.cpp
            src/windows_keyboard_hook.cpp
            src/windows_mouse_controller.cpp
        )

        target_include_directories(arrower PRIVATE ${PROJECT_SOURCE_DIR}/src)

        target_link_libraries(arrower
            PRIVATE
                arrower_core
                Qt6::Widgets
                user32
        )

        target_compile_definitions(arrower PRIVATE UNICODE _UNICODE)

        if(MINGW)
            target_link_options(arrower PRIVATE -static -static-libgcc -static-libstdc++)
        endif()
    else()
        message(WARNING "Qt6 Widgets not found; skipping the arrower tray UI target.")
    endif()
endif()

option(ARROWER_BUILD_TESTS "Build Arrower unit tests" ON)

if(ARROWER_BUILD_TESTS)
    enable_testing()

    add_executable(arrower_tests
        tests/config_tests.cpp
        tests/key_codes_tests.cpp
        tests/movement_engine_tests.cpp
        tests/input_state_tests.cpp
    )

    target_include_directories(arrower_tests
        PRIVATE
            ${PROJECT_SOURCE_DIR}/third_party
    )

    target_link_libraries(arrower_tests PRIVATE arrower_core)

    add_test(NAME arrower_tests COMMAND arrower_tests)
endif()
