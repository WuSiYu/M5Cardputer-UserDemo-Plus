/*
 * SPDX-FileCopyrightText: 2025 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
#include "app_launcher.h"
#include <apps/utils/theme.h>
#include <apps/utils/common.h>
#include <mooncake_log.h>
#include <hal/hal.h>
#include <hal.h>

using namespace mooncake;

void Launcher::onCreate()
{
    setAppInfo().name = "Launcher";
    mclog::tagInfo(getAppInfo().name, "on create");

    // Init
    boot_anim();
    start_menu();
    start_system_bar();
    start_keyboard_bar();

    open();
}

void Launcher::onRunning()
{
    // mclog::tagInfo(getAppInfo().name, "on running");

    update_system_bar();

    // If app is opened and running
    if (_data.running_app_id >= 0) {
        // If running app is closed
        if (GetMooncake().getAppCurrentState(_data.running_app_id) == AppAbility::StateSleeping) {
            _data.running_app_id = -1;
            render_keyboard_bar();
            render_system_bar();
            ANIM_APP_CLOSE();
        }
    } else {
        update_menu();
    }
}
