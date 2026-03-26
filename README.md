# ModUp

ModUp is a lightweight Windows monitor swap utility. The current app does not save and restore full display layouts; instead, it enumerates active displays, lets you pick exactly two, and swaps their display targets through the Windows DisplayConfig API.

## Current Behavior

- Enumerates active displays when the app starts.
- Shows each display as a checkbox with its display number, monitor name, GDI name, and orientation.
- Allows up to two displays to be selected at once.
- Applies the swap when you click `Swap Settings`.
- Stores the last selected display numbers in `modup_settings.json` next to the executable.

## Requirements

- Windows 10 or Windows 11
- Visual Studio 2022 Build Tools or Visual Studio with C++ build tools
- C++17 compiler
- `json.hpp` from [nlohmann/json](https://github.com/nlohmann/json) (already included in this repo)

## Build

The repo includes `build.bat`, which initializes the Visual C++ environment, compiles `MoDup.rc` into `MoDup.res`, and then builds the executable.

```cmd
build.bat
```

Equivalent manual commands:

```cmd
rc /fo MoDup.res MoDup.rc
cl /EHsc /std:c++17 MoDup.cpp MoDup.res User32.lib Advapi32.lib Gdi32.lib Shell32.lib Ole32.lib dwmapi.lib Msimg32.lib Setupapi.lib /Fe:MoDup.exe
```

## Run

A prebuilt executable is kept in `bin/MoDup.exe`.

1. Run `bin\MoDup.exe`.
2. The app opens as `Monitor Swap Tool (v3.3)`.
3. On startup it loads the previous selection from `modup_settings.json` if that file exists.

## Usage Flow

1. Launch `MoDup.exe`.
2. Select exactly two displays from the list.
3. Click `Swap Settings`.
4. If the swap succeeds, the app shows a success message, saves the selected display numbers, and refreshes the display list.

## Files

- `modup_settings.json`: runtime state file containing the last selected display numbers.
- `bin/MoDup.exe`: prebuilt Windows executable.
- `display_config.json`: legacy filename from the previous save/restore version; it is not the active runtime settings file for the current app.

## Implementation Notes

- Entry point: `wWinMain` in `MoDup.cpp`
- UI: Win32 + GDI custom button drawing
- Display handling: `QueryDisplayConfig` and `SetDisplayConfig`
- DPI mode: Per-monitor DPI aware v2 via `MoDup.manifest` and `SetProcessDpiAwarenessContext`

## Limitations

- Windows only
- Swaps exactly two active displays at a time
- Does not persist or restore a full monitor layout snapshot

## License

[MIT License](LICENSE)
