/*
 * SPDX-FileCopyrightText: 2025 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
#include "app_set_wifi.h"
#include "assets/set_wifi_big.h"
#include "assets/set_wifi_small.h"
#include <apps/utils/audio/audio.h>
#include <apps/utils/common.h>
#include <apps/utils/theme.h>
#include <mooncake_log.h>
#include <assets.h>

using namespace mooncake;

AppSetWiFi::AppSetWiFi()
{
    setAppInfo().name     = "SetWiFi";
    setAppInfo().userData = new AppIcon_t(image_data_set_wifi_big, image_data_set_wifi_small);
}

AppSetWiFi::~AppSetWiFi()
{
    delete static_cast<AppIcon_t*>(getAppInfo().userData);
}

void AppSetWiFi::onCreate()
{
    mclog::tagInfo(getAppInfo().name, "on create");

    load_saved_wifi_settings();
    if (!_wifi_ssid.empty() && !_wifi_password.empty()) {
        mclog::tagInfo(getAppInfo().name, "found saved wifi settings, attempting auto connect");
        _current_state = STATE_CONNECTING;
        render_interface();
        GetHAL().wifiConnectBackground(_wifi_ssid, _wifi_password);
        mclog::tagInfo(getAppInfo().name, "auto connect initiated");
    }
}

void AppSetWiFi::onOpen()
{
    mclog::tagInfo(getAppInfo().name, "on open");

    // Reset status
    _wifi_ssid.clear();
    _wifi_password.clear();
    _input_buffer.clear();
    _current_state     = STATE_INIT;
    _connection_result = false;

    load_saved_wifi_settings();

    // Setup keyboard event handler
    _key_event_slot_id = GetHAL().keyboard.onKeyEvent.connect(
        [this](const Keyboard::KeyEvent_t& keyEvent) { handle_key_event(keyEvent); });

    render_interface();
}

void AppSetWiFi::onRunning()
{
    // Update cursor blinking
    update_cursor();

    // Process state machine
    process_state_machine();

    // Close app when home button clicked
    if (GetHAL().homeButton.wasClicked()) {
        audio::play_random_tone();
        close();
    }
}

void AppSetWiFi::onClose()
{
    mclog::tagInfo(getAppInfo().name, "on close");

    // Disconnect keyboard event handler
    if (_key_event_slot_id >= 0) {
        GetHAL().keyboard.onKeyEvent.disconnect(_key_event_slot_id);
    }
}

void AppSetWiFi::render_interface()
{
    GetHAL().canvas.fillScreen(THEME_COLOR_BG);
    GetHAL().canvas.setTextScroll(true);
    GetHAL().canvas.setBaseColor(THEME_COLOR_BG);
    GetHAL().canvas.setTextColor(TFT_WHITE, THEME_COLOR_BG);
    GetHAL().canvas.setTextSize(1);
    GetHAL().canvas.setCursor(0, 0);

    show_ssid_prompt();
}

void AppSetWiFi::render_input_prompt()
{
    render_current_input_line();
}

void AppSetWiFi::render_current_input_line()
{
    // Get current cursor position - this is where we'll redraw the input line
    int cursor_y = GetHAL().canvas.getCursorY();

    // Clear the entire current line with extra space to ensure old cursor is removed
    GetHAL().canvas.fillRect(0, cursor_y, GetHAL().canvas.width(), 15, THEME_COLOR_BG);

    // Set cursor to beginning of line and redraw prompt + input + cursor
    GetHAL().canvas.setCursor(0, cursor_y);
    GetHAL().canvas.setTextColor(TFT_WHITE, THEME_COLOR_BG);
    GetHAL().canvas.print(">>> ");
    GetHAL().canvas.print(_input_buffer.c_str());

    // Draw blinking cursor
    if (_cursor_state) {
        GetHAL().canvas.print("_");
    } else {
        GetHAL().canvas.print(" ");
    }

    GetHAL().pushCanvas();
}

void AppSetWiFi::handle_key_event(const Keyboard::KeyEvent_t& keyEvent)
{
    // Only handle key press events for input states, skip modifiers
    if (!keyEvent.state || keyEvent.isModifier) {
        return;
    }

    // Only handle input in waiting states and failed state
    if (_current_state != STATE_WAIT_SSID && _current_state != STATE_WAIT_PASSWORD && _current_state != STATE_FAILED) {
        return;
    }

    switch (keyEvent.keyCode) {
        case KEY_ENTER:
            handle_enter_key();
            break;

        case KEY_BACKSPACE:
            handle_backspace();
            break;

        case KEY_SPACE:
            if (_input_buffer.length() < INPUT_BUFFER_SIZE - 1) {
                _input_buffer += ' ';
                render_input_prompt();
            }
            break;

        default:
            // Add regular characters
            if (keyEvent.keyName && strlen(keyEvent.keyName) == 1 && _input_buffer.length() < INPUT_BUFFER_SIZE - 1) {
                _input_buffer += keyEvent.keyName;
                render_input_prompt();
            }
            break;
    }
}

void AppSetWiFi::handle_enter_key()
{
    // Get ssid
    if (_current_state == STATE_WAIT_SSID) {
        if (_input_buffer.empty()) {
            return;
        }

        // Clear the current input line and redraw without cursor
        int cursor_y = GetHAL().canvas.getCursorY();
        GetHAL().canvas.fillRect(0, cursor_y, GetHAL().canvas.width(), 15, THEME_COLOR_BG);
        GetHAL().canvas.setCursor(0, cursor_y);
        GetHAL().canvas.setTextColor(TFT_WHITE, THEME_COLOR_BG);
        GetHAL().canvas.print(">>> ");
        GetHAL().canvas.print(_input_buffer.c_str());
        GetHAL().canvas.println();  // Move to next line

        _wifi_ssid = _input_buffer;
        mclog::tagInfo(getAppInfo().name, "WiFi SSID set: \"{}\"", _wifi_ssid);
        _input_buffer.clear();
        _current_state = STATE_WAIT_PASSWORD;
        GetHAL().pushCanvas();  // Ensure screen is updated before showing next prompt
        show_password_prompt();
    }

    // Get password
    else if (_current_state == STATE_WAIT_PASSWORD) {
        if (_input_buffer.empty()) {
            return;
        }

        // Clear the current input line and redraw without cursor
        int cursor_y = GetHAL().canvas.getCursorY();
        GetHAL().canvas.fillRect(0, cursor_y, GetHAL().canvas.width(), 15, THEME_COLOR_BG);
        GetHAL().canvas.setCursor(0, cursor_y);
        GetHAL().canvas.setTextColor(TFT_WHITE, THEME_COLOR_BG);
        GetHAL().canvas.print(">>> ");
        GetHAL().canvas.print(_input_buffer.c_str());
        GetHAL().canvas.println();  // Move to next line

        _wifi_password = _input_buffer;
        mclog::tagInfo(getAppInfo().name, "WiFi password set: \"{}\"", _wifi_password);
        _input_buffer.clear();
        _current_state = STATE_CONNECTING;
        GetHAL().pushCanvas();  // Ensure screen is updated before showing connection status
        show_connection_status();
    }

    // Retry connection
    else if (_current_state == STATE_FAILED) {
        // Clear screen and restart from SSID input
        GetHAL().canvas.fillScreen(THEME_COLOR_BG);
        GetHAL().canvas.setCursor(0, 0);
        _wifi_ssid.clear();
        _wifi_password.clear();
        _input_buffer.clear();
        show_ssid_prompt();
    }
}

void AppSetWiFi::handle_backspace()
{
    if (!_input_buffer.empty()) {
        _input_buffer.pop_back();

        // Clear a larger area to ensure old cursor is completely removed
        int cursor_y = GetHAL().canvas.getCursorY();
        GetHAL().canvas.fillRect(0, cursor_y, GetHAL().canvas.width(), 15, THEME_COLOR_BG);

        // Redraw the input line
        render_current_input_line();
    }
}

void AppSetWiFi::update_cursor()
{
    if (GetHAL().millis() - _cursor_update_time > CURSOR_BLINK_PERIOD) {
        _cursor_state       = !_cursor_state;
        _cursor_update_time = GetHAL().millis();
        if (_current_state == STATE_WAIT_SSID || _current_state == STATE_WAIT_PASSWORD) {
            render_input_prompt();
        }
    }
}

void AppSetWiFi::process_state_machine()
{
    if (_current_state == STATE_CONNECTING) {
        connect_to_wifi();
        _current_state = _connection_result ? STATE_SUCCESS : STATE_FAILED;
        show_connection_result();
    }
}

void AppSetWiFi::show_ssid_prompt()
{
    GetHAL().canvas.setTextColor(TFT_ORANGE, THEME_COLOR_BG);
    GetHAL().canvas.println("WiFi SSID:");
    GetHAL().canvas.setTextColor(TFT_WHITE, THEME_COLOR_BG);
    GetHAL().canvas.print(">>> ");
    GetHAL().pushCanvas();

    _current_state = STATE_WAIT_SSID;

    // Auto-fill saved SSID if available
    auto_fill_saved_ssid();
}

void AppSetWiFi::show_password_prompt()
{
    GetHAL().canvas.setTextColor(TFT_ORANGE, THEME_COLOR_BG);
    GetHAL().canvas.println("WiFi Password:");
    GetHAL().canvas.setTextColor(TFT_WHITE, THEME_COLOR_BG);
    GetHAL().canvas.print(">>> ");
    GetHAL().pushCanvas();

    // Auto-fill saved password if available
    auto_fill_saved_password();
}

void AppSetWiFi::show_connection_status()
{
    GetHAL().canvas.setTextColor(TFT_ORANGE, THEME_COLOR_BG);
    GetHAL().canvas.printf("WiFi config:\n- %s\n- %s\n", _wifi_ssid.c_str(), _wifi_password.c_str());
    GetHAL().canvas.println("Connecting...");
    GetHAL().canvas.setTextColor(TFT_WHITE, THEME_COLOR_BG);
    GetHAL().pushCanvas();
}

void AppSetWiFi::connect_to_wifi()
{
    mclog::tagInfo(getAppInfo().name, "attempting WiFi connection to \"{}\"", _wifi_ssid);

    // Init and connect
    GetHAL().wifiInit();
    _connection_result = GetHAL().wifiConnect(_wifi_ssid, _wifi_password);

    mclog::tagInfo(getAppInfo().name, "WiFi connection result: {}", _connection_result ? "success" : "failed");
}

void AppSetWiFi::show_connection_result()
{
    if (_connection_result) {
        GetHAL().canvas.setTextColor(TFT_GREEN, THEME_COLOR_BG);
        GetHAL().canvas.println("Connected successfully!");

        // Save WiFi settings on successful connection
        save_wifi_settings();

        GetHAL().canvas.setTextColor(TFT_CYAN, THEME_COLOR_BG);
        GetHAL().canvas.println("WiFi settings saved");
        GetHAL().canvas.setTextColor(TFT_WHITE, THEME_COLOR_BG);
        GetHAL().canvas.println("Press Home to exit");
    } else {
        GetHAL().canvas.setTextColor(TFT_RED, THEME_COLOR_BG);
        GetHAL().canvas.println("Connection failed!");
        GetHAL().canvas.setTextColor(TFT_WHITE, THEME_COLOR_BG);
        GetHAL().canvas.println("Press Enter to retry");
        GetHAL().canvas.println("Press Home to exit");
    }

    GetHAL().pushCanvas();
}

void AppSetWiFi::load_saved_wifi_settings()
{
    // Load saved SSID and password from settings
    _wifi_ssid     = GetHAL().getSettings().GetString("wifi_ssid", "");
    _wifi_password = GetHAL().getSettings().GetString("wifi_password", "");

    if (!_wifi_ssid.empty()) {
        mclog::tagInfo(getAppInfo().name, "loaded saved WiFi SSID: \"{}\"", _wifi_ssid);
    }
    if (!_wifi_password.empty()) {
        mclog::tagInfo(getAppInfo().name, "loaded saved WiFi password (length: {})", _wifi_password.length());
    }
}

void AppSetWiFi::save_wifi_settings()
{
    // Save SSID and password to settings
    GetHAL().getSettings().SetString("wifi_ssid", _wifi_ssid);
    GetHAL().getSettings().SetString("wifi_password", _wifi_password);

    mclog::tagInfo(getAppInfo().name, "saved WiFi settings - SSID: \"{}\"", _wifi_ssid);
}

void AppSetWiFi::auto_fill_saved_ssid()
{
    // Auto-fill the input buffer with saved SSID if available
    if (!_wifi_ssid.empty()) {
        _input_buffer = _wifi_ssid;
        render_input_prompt();
        mclog::tagInfo(getAppInfo().name, "auto-filled SSID: \"{}\"", _wifi_ssid);
    }
}

void AppSetWiFi::auto_fill_saved_password()
{
    // Auto-fill the input buffer with saved password if available
    if (!_wifi_password.empty()) {
        _input_buffer = _wifi_password;
        render_input_prompt();
        mclog::tagInfo(getAppInfo().name, "auto-filled password (length: {})", _wifi_password.length());
    }
}
