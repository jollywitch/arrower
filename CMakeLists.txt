cmake_minimum_required(VERSION 3.20)

project(arrower VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(arrower_core
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
    add_executable(arrower
        src/main.cpp
        src/windows_keyboard_hook.cpp
        src/windows_mouse_controller.cpp
    )

    target_link_libraries(arrower
        PRIVATE
            arrower_core
            user32
    )

    target_compile_definitions(arrower PRIVATE UNICODE _UNICODE)

    if(MINGW)
        target_link_options(arrower PRIVATE -static -static-libgcc -static-libstdc++)
    endif()
endif()

option(ARROWER_BUILD_TESTS "Build Arrower unit tests" ON)

if(ARROWER_BUILD_TESTS)
    enable_testing()

    add_executable(arrower_tests
        tests/config_tests.cpp
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
