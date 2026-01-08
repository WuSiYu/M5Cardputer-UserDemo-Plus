# GEMINI.md

## Project Overview

This project is an enhanced firmware for the M5Cardputer, a portable ESP32-based device. The firmware is written in C++ and utilizes the ESP-IDF framework along with the Arduino core for ESP32.

The project follows a modular architecture, built around the "Mooncake" application framework. This framework is responsible for managing the lifecycle of various applications on the device.

The core components of the project are:

*   **Hardware Abstraction Layer (HAL):** Located in `main/hal/`, this component provides a consistent interface for interacting with the M5Cardputer's hardware, including the display, keyboard, speaker, microphone, and battery.
*   **Mooncake Framework:** Found in `components/mooncake/`, this framework handles application registration, management, and input device handling. It allows for a clean separation between the core system and the individual applications.
*   **Applications:** The various applications for the device are located in the `main/apps/` directory. Each application is packaged as a "packer" and installed into the Mooncake framework. Examples of applications include a Wi-Fi scanner, a web radio, a REPL, and a keyboard tester.
*   **Main Entry Point:** The main application logic resides in `main/cardputer.cpp`, which initializes the HAL, the Mooncake framework, and all the installed applications.

## Building and Running

The project is built using the ESP-IDF build system.

**Prerequisites:**

*   ESP-IDF v4.4.6

**Build Command:**

To build the project, run the following command from the root directory:

```bash
. $IDF_PATH/export.sh
```

```bash
idf.py build
```

**Flash on Cardputer Device Command"

```bash
idf.py -p /dev/cu.usbmodem11301 flash
```

## Development Conventions

*   **Language:** The project is written in C++.
*   **Framework:** It uses the ESP-IDF framework with the Arduino core for ESP32.
*   **Application Structure:** New applications should be created as "packers" and installed into the Mooncake framework. Each application should be self-contained in its own directory within `main/apps/`.
*   **Hardware Interaction:** All hardware interactions should be done through the Hardware Abstraction Layer (HAL) to ensure portability and maintain a clean separation of concerns.
*   **Code Style:** The existing code follows a consistent style. Please adhere to the established conventions when adding new code.
