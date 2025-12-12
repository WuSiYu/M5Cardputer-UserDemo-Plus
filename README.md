# Cardputer ADV User Demo

User demo source code of [Cardputer ADV](https://docs.m5stack.com/en/products/sku/K132-Adv).

**Official firmware enhancements**

2025/12: I recently brought an Adv device, so I migrated my "enhanced work" (and some more) from last year based on the latest official Adv firmware, here are the details:

- add arduino-esp32 as a ESP-IDF component for easier development
  - **IMPORTANT**: you need to add/active `list(APPEND COMPONENT_REQUIRES arduino-esp32)` line in `components/M5Unified/CMakeLists.txt` before compile
- porting @cyberwisk's **cardputer WebRadio** as an App
  - using a modded version of ESP8266Audio to support https and chunked stream (like qtfm.cn)
  - ability to play radio in background when pressing HOME (G0), press ESC to fully exit
  - more radio stations, you can modify the radio list at (main/apps/app_radio/M5Cardputer_WebRadio.cpp)
- enhance SCAN, CLOCK, SetWiFi App
  - SetWiFi: now support **auto-connect saved wifi in background on boot**
  - CLOCK: add timer and stopwatch feature
  - SCAN: non-blocking scan, multi page switchable
- system enhance
  - DIRAM usage optimazation, about **30KB more free RAM** for heap
  - enhance system bar, show battery voltage and current free heep size
  - adjust screen brightness with [-]/[=] buttons in App Menu
  - print cpu usage every 10s in serial log (disable it in hal.cpp)
- *(TODO) add SCALES and ENV IV App for the mini-scales and ENV IV m5stack sensors*
- *(TODO) add UART Terminal App*


## Build

### Fetch Dependencies

```bash
python3 ./fetch_repos.py
```

### Tool Chains

[ESP-IDF v5.4.2](https://docs.espressif.com/projects/esp-idf/en/v5.4.2/esp32s3/index.html)

### Build

```bash
idf.py build
```

### Flash

```bash
idf.py flash
```

## Acknowledgments

This project references the following open-source libraries and resources:

- https://github.com/adafruit/Adafruit_TCA8418
- https://github.com/m5stack/M5Unified.git
- https://github.com/pikasTech/PikaPython
- https://github.com/jgromes/RadioLib
- https://github.com/raysan5/raylib
- https://github.com/mikalhart/TinyGPSPlus
- https://github.com/m5stack/M5GFX.git
- https://github.com/Forairaaaaa/mooncake_log
- https://github.com/hhuysqt/esp32s3-keyboard
- https://github.com/78/xiaozhi-esp32
- https://github.com/Forairaaaaa/mooncake
- https://github.com/Forairaaaaa/smooth_ui_toolkit
