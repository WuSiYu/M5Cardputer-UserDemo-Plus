/**
 * @file app_radio.cpp
 * @author WuSiYu
 * @brief
 * @version 0.2
 * @date 2025-12-12
 *
 * @copyright Copyright (c) 2025
 *
 */
#include "app_radio.h"
#include <apps/utils/common.h>
#include <mooncake_log.h>

bool AppRadio::__running = false;

AppRadio::AppRadio()
{
    setAppInfo().name = "Radio";
    setAppInfo().userData = new AppIcon_t(image_data_radio_big, image_data_radio_small);
}

AppRadio::~AppRadio()
{
    delete static_cast<AppIcon_t*>(getAppInfo().userData);
}

void AppRadio::onOpen()
{
    mclog::tagInfo(getAppInfo().name, "on open");

    ANIM_APP_OPEN();

    GetHAL().canvas.fillScreen(THEME_COLOR_BG);
    GetHAL().canvas.setTextScroll(true);
    GetHAL().canvas.setTextColor(THEME_COLOR_REPL_TEXT, THEME_COLOR_BG);
    GetHAL().canvas.setFont(FONT_REPL);
    GetHAL().canvas.setTextSize(FONT_SIZE_REPL);
    GetHAL().canvas.setCursor(0, 0);

    // Setup keyboard event handler
    _data._key_event_slot_id = GetHAL().keyboard.onKeyEvent.connect(
        [this](const Keyboard::KeyEvent_t& keyEvent) { handle_key_event(keyEvent); });

    if (!__running)  {
        if (!GetHAL().isWifiConnected()) {
            _data.current_state = state_error;
            _update_state();
        } else {
            _data.current_state = state_setup;
            _update_state();
        }
    } else {
        GetHAL().display.clear();
        gfxSetup(&GetHAL().display);
        meta_mod_bits = 3;
        _data.current_state = state_started;
    }
}

void AppRadio::onRunning()
{
    if (_data.current_state == state_started) {
        _loop();

        if (millis() - _data._last_update_battery_time > 5000) {
            float bat_v = GetHAL().getBatLevel();
            GetHAL().display.setTextColor(TFT_GREEN, TFT_BLACK);
            GetHAL().display.drawRightString((std::to_string(bat_v).substr(0, 5) + "%").c_str(), GetHAL().display.width(), 0, FONT_SMALL);
            GetHAL().display.setTextColor(TFT_WHITE);
            _data._last_update_battery_time = millis();
        }
    }

    if (GetHAL().homeButton.wasClicked())
    {
        GetHAL().speaker.tone(1000, 100);
        mclog::tagInfo(getAppInfo().name, "quit radio");
        close();
    }
}

void AppRadio::onClose()
{
    mclog::tagInfo(getAppInfo().name, "on close");
    GetHAL().canvas.setTextScroll(false);

    // Disconnect keyboard event handler
    if (_data._key_event_slot_id >= 0) {
        GetHAL().keyboard.onKeyEvent.disconnect(_data._key_event_slot_id);
        _data._key_event_slot_id = -1;
    }
}

#define _keyboard GetHAL().keyboard
#define _canvas (&GetHAL().canvas)
#define _canvas_update GetHAL().pushCanvas

void AppRadio::handle_key_event(const Keyboard::KeyEvent_t& key_event)
{
    // Only handle key press events
    if (!key_event.state) {
        return;
    }

    if (_data.current_state == state_error) {
        // Any key to exit
        GetHAL().speaker.tone(1000, 100);
        close();
    }
    else if (_data.current_state == state_started) {
        handle_radio_key_event(key_event);
    }
}


void AppRadio::_update_state()
{
    if (_data.current_state == state_error)
    {
        _canvas->setTextColor(TFT_RED, THEME_COLOR_BG);
        _canvas->printf("WiFi not connected!\n");
        _canvas->setTextColor(THEME_COLOR_REPL_TEXT, THEME_COLOR_BG);
        _canvas->printf("Please use SetWiFi app to connect.\n");
        _canvas->printf("\nPress HOME to exit...");
        _canvas_update();
    }

    if (_data.current_state == state_setup) {
        _setup();
        _data.current_state = state_started;
    }
}
