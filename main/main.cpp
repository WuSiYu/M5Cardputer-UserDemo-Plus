/*
 * SPDX-FileCopyrightText: 2025 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
#include <smooth_ui_toolkit.h>
#include <M5Unified.hpp>
#include <mooncake_log.h>
#include <mooncake.h>
#include <apps.h>
#include <hal.h>

using namespace mooncake;
using namespace smooth_ui_toolkit;

extern "C" void app_main(void)
{
    // Setup logger
    mclog::set_level(mclog::level_debug);
    mclog::set_time_format(mclog::time_format_unix_milliseconds);

    // HAL init
    GetHAL().init();

    // Setup ui hal
    ui_hal::on_delay([](uint32_t ms) { GetHAL().delay(ms); });
    ui_hal::on_get_tick([]() { return GetHAL().millis(); });

    // Install apps
    GetMooncake().installApp(std::make_unique<Launcher>());
    GetMooncake().installApp(std::make_unique<AppWifiScan>());
    GetMooncake().installApp(std::make_unique<AppClock>());
    GetMooncake().installApp(std::make_unique<AppRadio>());
    GetMooncake().installApp(std::make_unique<AppRecord>());
    GetMooncake().installApp(std::make_unique<AppChat>());
    GetMooncake().installApp(std::make_unique<AppRemote>());
    GetMooncake().installApp(std::make_unique<AppREPL>());
    GetMooncake().installApp(std::make_unique<AppSetWiFi>());
    GetMooncake().installApp(std::make_unique<AppKeyboard>());
    GetMooncake().installApp(std::make_unique<AppImu>());
    GetMooncake().installApp(std::make_unique<AppSdcard>());
    GetMooncake().installApp(std::make_unique<AppStringIRToolKit>());
    GetMooncake().installApp(std::make_unique<AppLoraChat>());
    GetMooncake().installApp(std::make_unique<AppGPS>());
    // GetMooncake().installApp(std::make_unique<AppDummy>());

    // Main loop
    audio::set_keyboard_sfx_enable(true);
    while (1) {
        GetHAL().feedTheDog();
        GetHAL().update();
        GetMooncake().update();
    }
}
