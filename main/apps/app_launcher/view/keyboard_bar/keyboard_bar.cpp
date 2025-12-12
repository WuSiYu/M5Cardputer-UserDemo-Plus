/*
 * SPDX-FileCopyrightText: 2025 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
#include "../../app_launcher.h"
#include <mooncake_log.h>
#include <apps/utils/theme.h>
#include <apps/utils/common.h>

#include "assets/Aa.h"
#include "assets/Aa0.h"
#include "assets/alt.h"
#include "assets/alt0.h"
#include "assets/crtl.h"
#include "assets/ctrl0.h"
#include "assets/fn.h"
#include "assets/fn0.h"
#include "assets/opt.h"
#include "assets/opt1.h"

void Launcher::render_keyboard_bar()
{
    // Backgound
    int margin_x = 4;
    int margin_y = 2;

    GetHAL().canvasKeyboardBar.fillScreen(THEME_COLOR_BG);
    GetHAL().canvasKeyboardBar.fillSmoothRoundRect(
        margin_x, margin_y, GetHAL().canvasKeyboardBar.width() - margin_x * 2,
        GetHAL().canvasKeyboardBar.height() - margin_y * 2, 7,
        THEME_COLOR_KB_BAR);

    // render state
    int x      = 7;
    int y      = 20;
    int width  = 22;
    int height = 17;
    int gap_y  = 3;

    if (_data.keybaord_state.caps_lock) {
        GetHAL().canvasKeyboardBar.pushImage(x, y, width, height, image_data_Aa);
    } else {
        GetHAL().canvasKeyboardBar.pushImage(x, y, width, height, image_data_Aa0);
    }

    y = y + height + gap_y;
    if (_data.keybaord_state.fn) {
        GetHAL().canvasKeyboardBar.pushImage(x, y, width, height, image_data_fn);
    } else {
        GetHAL().canvasKeyboardBar.pushImage(x, y, width, height, image_data_fn0);
    }

    y = y + height + gap_y;
    if (_data.keybaord_state.ctrl) {
        GetHAL().canvasKeyboardBar.pushImage(x, y, width, height, image_data_crtl);
    } else {
        GetHAL().canvasKeyboardBar.pushImage(x, y, width, height, image_data_ctrl0);
    }

    y = y + height + gap_y;
    if (_data.keybaord_state.opt) {
        GetHAL().canvasKeyboardBar.pushImage(x, y, width, height, image_data_opt);
    } else {
        GetHAL().canvasKeyboardBar.pushImage(x, y, width, height, image_data_opt1);
    }

    y = y + height + gap_y;
    if (_data.keybaord_state.alt) {
        GetHAL().canvasKeyboardBar.pushImage(x, y, width, height, image_data_alt);
    } else {
        GetHAL().canvasKeyboardBar.pushImage(x, y, width, height, image_data_alt0);
    }

    // Push
    GetHAL().pushCanvasKeyboardBar();
}

void Launcher::start_keyboard_bar()
{
    GetHAL().keyboard.onKeyEvent.connect([this](const Keyboard::KeyEvent_t& keyEvent) {
        bool need_render = false;
        switch (keyEvent.keyCode) {
            case KEY_CAPSLOCK:
                _data.keybaord_state.caps_lock = keyEvent.state;
                need_render                    = true;
                break;
            case KEY_LEFTCTRL:
                _data.keybaord_state.ctrl = keyEvent.state;
                need_render               = true;
                break;
            case KEY_LEFTMETA:
                _data.keybaord_state.opt = keyEvent.state;
                need_render              = true;
                break;
            case KEY_LEFTALT:
                _data.keybaord_state.alt = keyEvent.state;
                need_render              = true;
                break;
            case KEY_LEFTSHIFT:
                _data.keybaord_state.fn = keyEvent.state;
                need_render             = true;
                break;
            default:
                break;
        }

        if (need_render) {
            render_keyboard_bar();
        }
    });

    render_keyboard_bar();
}
