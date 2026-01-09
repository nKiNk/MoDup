# ModUp (Monitor Configuration Tool)

ModUp is a lightweight Windows utility designed to easily save and restore your multi-monitor display configurations. It is particularly useful for users who frequently switch between different desk setups, docking stations, or KVM switches, where Windows often forgets window positions and monitor arrangements.

## Features

*   **Save Configuration**: Instantly capture the current resolution, position, and orientation of all active monitors.
*   **Restore Configuration**: Apply the saved settings with a single click to restore your preferred layout.
*   **Modern UI**: Features a clean, DPI-aware interface with Windows 11-style rounded buttons and hover effects.
*   **Portable**: Configuration is saved to a local JSON file (`display_config.json`), making it easy to backup or move.

## Requirements

*   Windows 10 or Windows 11
*   Visual Studio 2019 or later (for building from source)
*   C++17 compatible compiler

## Build Instructions

1.  Clone this repository or download the source code.
2.  Ensure you have the `json.hpp` file from [nlohmann/json](https://github.com/nlohmann/json) in the project directory (already included in this repo).
3.  Open the project in Visual Studio or use the command line compiler `cl.exe`.
4.  Compile the `MoDup.cpp` file. The code includes necessary pragmas to link against `dwmapi.lib` and `Msimg32.lib`.

    ```cmd
    cl /EHsc /std:c++17 MoDup.cpp user32.lib gdi32.lib
    ```

## Running the Application

A pre-compiled executable is available in the `bin` folder.

1.  Navigate to the `bin` folder.
2.  Run `MoDup.exe`.
3.  The `display_config.json` file will be created in the same directory where the executable is located.

## Usage

1.  Run `MoDup.exe`.
2.  **Save**: Click the **Save** button to store your current monitor layout to `display_config.json`.
3.  **Restore**: Click the **Restore** button to apply the settings from the JSON file.
4.  **Exit**: Click **Exit** to close the application.

## Technologies Used

*   C++ (Win32 API)
*   [nlohmann/json](https://github.com/nlohmann/json) for JSON serialization
*   GDI for custom UI rendering

## License

[MIT License](LICENSE)
