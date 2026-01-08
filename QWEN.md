# M5Cardputer-UserDemo-Plus QWEN Context

## Project Overview

This is an enhanced official firmware for the M5Cardputer, a portable ESP32-based device. The firmware is written in C++ and utilizes the ESP-IDF framework along with the Arduino core for ESP32.

The project follows a modular architecture, built around the "Mooncake" application framework. This framework is responsible for managing the lifecycle of various applications on the device.

The core components of the project are:

*   **Hardware Abstraction Layer (HAL):** Located in `main/hal/`, this component provides a consistent interface for interacting with the M5Cardputer's hardware, including the display, keyboard, speaker, microphone, and battery.
*   **Mooncake Framework:** Found in `components/mooncake/`, this framework handles application registration, management, and input device handling. It allows for a clean separation between the core system and the individual applications.
*   **Applications:** The various applications for the device are located in the `main/apps/` directory. Each application is packaged as a "packer" and installed into the Mooncake framework. Examples of applications include:
    *   Wi-Fi Scanner (app_wifi_scan)
    *   Web Radio (app_radio) - ported from @cyberwisk's cardputer WebRadio with HTTPS and chunked stream support
    *   Timer/Clock (app_timer)
    *   Infrared Remote (app_ir) 
    *   Keyboard (app_keyboard)
    *   Scales (app_scales) - for mini-scales sensor
    *   Environment IV (app_env_iv) - for ENV IV m5stack sensors
    *   Set Wi-Fi (app_set_wifi) - enhanced to keep WiFi connected in background
    *   Kimchi App (app_kimchi)
    *   Launcher (launcher) - main application selector
*   **Main Entry Point:** The main application logic resides in `main/cardputer.cpp`, which initializes the HAL, the Mooncake framework, and all the installed applications.

## Unique Features

*   Arduino-esp32 added as an ESP-IDF component for easier development
*   Porting of native code to Arduino library to avoid crashes
*   Enhanced WebRadio app with HTTPS and chunked stream support (like qtfm.cn)
*   Ability to play radio in background when pressing HOME (G0), press ESC to fully exit
*   Enhanced SCAN, TIMER (renamed to CLOCK), and SetWiFi Apps
*   System enhancements:
    *   WiFi remains connected in background by default
    *   Uses ESP-IDF's automatic SNTP for time synchronization
    *   Enhanced system bar showing battery voltage and current free heap size
    *   Real functionality added to WiFi icon

## Building and Running

The project is built using the ESP-IDF build system.

**Prerequisites:**
*   ESP-IDF v4.4.6 (as specified in README)

**Build Commands:**
```bash
# Initialize the build environment
. $IDF_PATH/export.sh

# Build the project
idf.py build
```

**Flash the Firmware:**
```bash
# Flash to Cardputer device (Linux)
idf.py -p /dev/ttyACM0 flash

# Flash to Cardputer device (macOS)
idf.py -p /dev/cu.usbmodem11301 flash

# Using the provided flash script
./flash.sh
```

**Monitor Output:**
```bash
# Monitor serial output after flashing
idf.py -p /dev/cu.usbmodem11301 monitor

# Or monitor separately after build
idf.py monitor
```

## Project Architecture

### Target Device
- M5Cardputer (ESP32-S3 based)
- Display: LCD screen
- Input: Built-in keyboard
- Audio: Speaker and microphone
- Connectivity: Wi-Fi and Bluetooth
- Sensors: Various M5 stack modules supported

### Key Configuration Files
- `sdkconfig`: ESP-IDF configuration for the target hardware
- `partitions.csv`: Memory partition layout
- `CMakeLists.txt`: Main CMake build file
- `flash.sh`: Script for convenient building and flashing
- `mc_conf.h`: Mooncake framework configuration

### Memory Layout (from partitions.csv)
- nvs: 0x9000, 0x5000 (for non-volatile storage)
- phy_init: 0xf000, 0x1000 (PHY initialization data)
- factory: 0x10000, 4M (application firmware)
- storage: 1M (file system storage)

## Development Conventions

*   **Language:** The project is written in C++.
*   **Framework:** It uses the ESP-IDF framework with the Arduino core for ESP32.
*   **Application Structure:** New applications should be created as "packers" and installed into the Mooncake framework. Each application should be self-contained in its own directory within `main/apps/`.
*   **Hardware Interaction:** All hardware interactions should be done through the Hardware Abstraction Layer (HAL) to ensure portability and maintain a clean separation of concerns.
*   **Code Style:** The existing code follows a consistent style. Please adhere to the established conventions when adding new code.
*   **Audio Support:** The project includes ESP8266Audio library with modifications to support HTTPS and chunked streams for WebRadio functionality.
*   **Graphics:** Uses LovyanGFX for display rendering and LVGL for GUI elements.

## Testing & Debugging

The main.cpp contains several conditional compilation flags for testing:
*   `ON_HAL_TEST`: Hardware abstraction layer testing
*   `ON_APP_TEST`: Individual application testing
*   `ON_APP_TEST_WITH_LAUNCHER`: Application testing with launcher

## Key Components

*   `components/arduino`: Arduino ESP32 integration
*   `components/ESP8266Audio`: Modified audio library for WebRadio functionality
*   `components/LovyanGFX`: Graphics library for display operations
*   `components/mooncake`: The application framework that manages all apps
*   `components/infrared_tools`: IR functionality support
*   `mc_conf.h`: Mooncake configuration with database key definitions

## Adding New Applications

To add a new application:
1. Create a new directory in `main/apps/` with your app name
2. Implement your app inheriting from APP_BASE
3. Create a packer class inheriting from APP_PACKER_BASE
4. Register the app in the main cardputer.cpp file by adding `mooncake.installApp(new APPS::YourApp_Packer);` in the main app installation section

## Dependencies

*   ESP-IDF v4.4.6
*   Arduino-esp32 as ESP-IDF component
*   Third-party components in the components directory (LovyanGFX, mooncake, ESP8266Audio, etc.)