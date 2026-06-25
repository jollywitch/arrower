# arrower

`arrower` is a Windows tray utility that lets you control the mouse with the keyboard while holding `Right Ctrl`.

## Default controls

- `Right Ctrl` + `Arrow keys`: move cursor
- `Right Ctrl` + `.`: hold left button for drag, or tap for a normal left click
- `Right Ctrl` + `/`: right click
- `Right Ctrl` + `/` + `Up`/`Down`: scroll vertically
- `Ctrl` + `Alt` + `Esc`: emergency quit

The default movement loop runs at `120 Hz` with:

- base speed: `6 px/tick`
- acceleration: `0.8 px/tick`
- max speed: `28 px/tick`
- drag rate: `24 Hz`
- scroll rate: `18 Hz`

## Build

### Windows / MSVC with Qt 6 Widgets

```bash
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

The Windows app target requires `Qt 6 Widgets`. Release builds use the Visual Studio C++ toolchain with the Qt MSVC package.

### Tests

```bash
cmake -S . -B build -DARROWER_BUILD_TESTS=ON
cmake --build build --target arrower_tests
ctest --test-dir build --output-on-failure
```

On non-Windows hosts, only the cross-platform core and tests are built.

## Release artifacts

Windows releases publish:

- `arrower-<version>-windows-x64-setup.exe`: recommended per-user installer
- `arrower-<version>-windows-x64.zip`: portable build with Qt and runtime DLLs

The standalone `arrower.exe` is not intended to be distributed by itself because the Qt and runtime files must be available beside it.

Tags starting with `test-` publish a draft pre-release so maintainers can download and verify the installer before making a public release.

## Config

`arrower` looks for `config.json` next to the executable. The tray UI reads and writes this file directly; if the file is missing, built-in defaults are used.

Example:

```json
{
  "activation_modifier": "RightCtrl",
  "bindings": {
    "up": "Up",
    "down": "Down",
    "left": "Left",
    "right": "Right",
    "left_click": "OemPeriod",
    "right_click": "Oem2"
  },
  "movement": {
    "base_speed_px_per_tick": 6.0,
    "acceleration_px_per_tick": 0.8,
    "max_speed_px_per_tick": 28.0,
    "update_rate_hz": 120,
    "drag_update_rate_hz": 24,
    "scroll_update_rate_hz": 18
  }
}
```
