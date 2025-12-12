/*
 * SPDX-FileCopyrightText: 2025 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
#include "app_clock.h"
#include "assets/timer_big.h"
#include "assets/timer_small.h"
#include <apps/utils/audio/audio.h>
#include <apps/utils/common.h>
#include <apps/utils/theme.h>
#include <mooncake_log.h>
#include <assets.h>

using namespace mooncake;

AppClock::AppClock()
{
    setAppInfo().name     = "Clock";
    setAppInfo().userData = new AppIcon_t(image_data_timer_big, image_data_timer_small);
}

AppClock::~AppClock()
{
    delete static_cast<AppIcon_t*>(getAppInfo().userData);
}

void AppClock::onOpen()
{
    mclog::tagInfo(getAppInfo().name, "on open");
    _time_count    = GetHAL().millis();
    _last_alarm_ms = 0;
    _saved_volume  = GetHAL().speaker.getVolume();

    _key_event_slot_id = GetHAL().keyboard.onKeyEvent.connect(
        [this](const Keyboard::KeyEvent_t& keyEvent) { handle_key_event(keyEvent); });

    render_interface();
    update_time_display();
}

void AppClock::onRunning()
{
    const auto now_ms = GetHAL().millis();
    const auto interval_ms = (_stopwatch_start_ms || _timer_end_ms) ? 80 : UPDATE_INTERVAL;

    if (now_ms - _time_count >= interval_ms) {
        if (_timer_end_ms) {
            render_timer();
        } else if (_stopwatch_start_ms) {
            render_stopwatch();
        } else {
            update_time_display();
        }
        _time_count = now_ms;
    }

    // Gentle reminder every 15 minutes while stopwatch runs
    constexpr int64_t STOPWATCH_ALARM_DELTA_MS = 15 * 60 * 1000;
    if (_stopwatch_start_ms && !_stopwatch_paused_ms) {
        const auto elapsed = now_ms - _stopwatch_start_ms;
        if ((elapsed % STOPWATCH_ALARM_DELTA_MS) < 300 && now_ms - _last_alarm_ms > 100) {
            audio::play_tone(3000, 0.015);
            _last_alarm_ms = now_ms;
        }
    }

    // Louder alert when timer finishes
    if (_timer_end_ms && !_timer_paused_ms && now_ms > _timer_end_ms && now_ms - _last_alarm_ms > 2000) {
        GetHAL().speaker.setVolume(255);

        // style1
        // for (int i = 0; i < 10; i++) {
        //     audio::play_random_tone(48, 0.02);
        //     GetHAL().delay(100);
        // }

        // style2
        GetHAL().speaker.tone(900, 1000);

        _last_alarm_ms = now_ms;
    }

    if (GetHAL().homeButton.wasClicked()) {
        GetHAL().speaker.setVolume(_saved_volume);
        audio::play_random_tone();
        close();
    }
}

void AppClock::onClose()
{
    mclog::tagInfo(getAppInfo().name, "on close");

    if (_key_event_slot_id >= 0) {
        GetHAL().keyboard.onKeyEvent.disconnect(_key_event_slot_id);
        _key_event_slot_id = -1;
    }

    GetHAL().speaker.setVolume(_saved_volume);
}

void AppClock::render_interface()
{
    GetHAL().canvas.fillScreen(THEME_COLOR_BG);
    GetHAL().canvas.setTextColor(TFT_ORANGE, THEME_COLOR_BG);
    GetHAL().canvas.setTextSize(2);
    GetHAL().canvas.setFont(FONT_REPL);
    GetHAL().pushCanvas();
}

void AppClock::update_time_display()
{
    // Check WiFi connection status instead of SNTP adjustment
    if (GetHAL().isWifiConnected()) {
        show_network_time();
    } else {
        show_system_time();
    }
}

void AppClock::show_network_time()
{
    // Get current time from system (assuming SNTP has synchronized it when WiFi was connected)
    time_t now;
    struct tm timeinfo;
    time(&now);
    localtime_r(&now, &timeinfo);

    GetHAL().canvas.fillScreen(THEME_COLOR_BG);

    // Format and display time
    auto time_str = fmt::format("{:02d}:{:02d}:{:02d}", timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);

    GetHAL().canvas.setTextSize(3);
    GetHAL().canvas.setTextColor(TFT_ORANGE, THEME_COLOR_BG);
    GetHAL().canvas.drawString(time_str.c_str(), 5, GetHAL().canvas.height() / 2 - 37);

    // Show time source indicator
    GetHAL().canvas.setCursor(5, 5);
    GetHAL().canvas.setTextSize(1);
    GetHAL().canvas.setTextColor(TFT_GREEN, THEME_COLOR_BG);
    GetHAL().canvas.print("Network Time");

    // Show date
    auto date_str = fmt::format("{:04d}-{:02d}-{:02d}", timeinfo.tm_year + 1900, timeinfo.tm_mon + 1, timeinfo.tm_mday);

    GetHAL().canvas.setTextSize(1);
    GetHAL().canvas.setTextColor(TFT_YELLOW, THEME_COLOR_BG);
    GetHAL().canvas.drawString(date_str.c_str(), 5, GetHAL().canvas.height() - 47);

    // Footer hint (left aligned, small font)
    GetHAL().canvas.setTextColor(TFT_LIGHTGREY, THEME_COLOR_BG);
    GetHAL().canvas.setFont(FONT_SMALL);
    GetHAL().canvas.setTextSize(1);
    const int footer_y0 = GetHAL().canvas.height() - 25;
    GetHAL().canvas.drawString("[t] timer", 5, footer_y0);
    GetHAL().canvas.drawString("[s] stopwatch", 5, footer_y0 + 10);

    // Restore default font
    GetHAL().canvas.setFont(FONT_REPL);

    GetHAL().pushCanvas();
}

void AppClock::show_system_time()
{
    // Calculate system uptime
    uint32_t total_seconds = GetHAL().millis() / 1000;
    uint32_t hours         = total_seconds / 3600;
    uint32_t minutes       = (total_seconds % 3600) / 60;
    uint32_t seconds       = total_seconds % 60;

    GetHAL().canvas.fillScreen(THEME_COLOR_BG);

    // Show time source indicator
    GetHAL().canvas.setCursor(5, 5);
    GetHAL().canvas.setTextSize(1);
    GetHAL().canvas.setTextColor(TFT_YELLOW, THEME_COLOR_BG);
    GetHAL().canvas.print("System Uptime");

    // Format and display uptime
    auto uptime_str = fmt::format("{:02d}:{:02d}:{:02d}", hours, minutes, seconds);

    GetHAL().canvas.setTextSize(2);
    GetHAL().canvas.setTextColor(TFT_ORANGE, THEME_COLOR_BG);
    GetHAL().canvas.drawCenterString(uptime_str.c_str(), GetHAL().canvas.width() / 2,
                                     GetHAL().canvas.height() / 2 - 25);

    // Show WiFi status hint
    GetHAL().canvas.setTextSize(1);
    GetHAL().canvas.setTextColor(TFT_DARKGREY, THEME_COLOR_BG);
    GetHAL().canvas.drawCenterString("Set WiFi for real time", GetHAL().canvas.width() / 2,
                                     GetHAL().canvas.height() - 45);

    // Footer hint (left aligned, small font)
    GetHAL().canvas.setTextColor(TFT_LIGHTGREY, THEME_COLOR_BG);
    GetHAL().canvas.setFont(FONT_SMALL);
    GetHAL().canvas.setTextSize(1);
    const int footer_y0 = GetHAL().canvas.height() - 25;
    GetHAL().canvas.drawString("[t] timer", 5, footer_y0);
    GetHAL().canvas.drawString("[s] stopwatch", 5, footer_y0 + 10);

    // Restore default font
    GetHAL().canvas.setFont(FONT_REPL);

    GetHAL().pushCanvas();
}

void AppClock::render_stopwatch()
{
    const int64_t now_ms    = GetHAL().millis();
    const bool is_paused    = _stopwatch_paused_ms != 0;
    const int64_t elapsed   = (is_paused ? _stopwatch_paused_ms : now_ms) - _stopwatch_start_ms;
    const bool show_colon   = is_paused || ((elapsed / 20) % 5 >= 2);
    const char colon_symbol = show_colon ? ':' : ' ';

    const auto hours   = static_cast<int>(elapsed / 3600000);
    const auto minutes = static_cast<int>((elapsed / 60000) % 60);
    const auto seconds = static_cast<int>((elapsed / 1000) % 60);
    const auto hundred = static_cast<int>((elapsed / 10) % 100);

    auto time_str = fmt::format("{:02d}{}{:02d}{}{:02d}", hours, colon_symbol, minutes, colon_symbol, seconds);
    auto ms_str   = fmt::format(".{:02d}", hundred);

    GetHAL().canvas.fillScreen(THEME_COLOR_BG);
    GetHAL().canvas.setCursor(5, 5);
    GetHAL().canvas.setTextSize(1);
    GetHAL().canvas.setTextColor(is_paused ? TFT_LIGHTGREY : TFT_GREENYELLOW, THEME_COLOR_BG);
    GetHAL().canvas.print(is_paused ? "Stopwatch - paused" : "Stopwatch - running");

    GetHAL().canvas.setTextColor(TFT_ORANGE, THEME_COLOR_BG);
    GetHAL().canvas.setTextSize(2);
    GetHAL().canvas.drawCenterString(time_str.c_str(), GetHAL().canvas.width() / 2 - 15,
                                     GetHAL().canvas.height() / 2 - 25);

    GetHAL().canvas.setTextSize(1);
    GetHAL().canvas.drawCenterString(ms_str.c_str(), GetHAL().canvas.width() / 2 + 65,
                                     GetHAL().canvas.height() / 2 - 25 + 14);

    GetHAL().canvas.setTextColor(TFT_LIGHTGREY, THEME_COLOR_BG);
    GetHAL().canvas.setFont(FONT_SMALL);
    const int footer_y0 = GetHAL().canvas.height() - 35;
    GetHAL().canvas.drawString("[del] reset", 5, footer_y0);
    if (is_paused) {
        GetHAL().canvas.drawString("[ANY] continue", 5, footer_y0 + 10);
    } else {
        GetHAL().canvas.drawString("[ANY] pause", 5, footer_y0 + 10);
    }
    GetHAL().canvas.drawString("[esc] reset & back", 5, footer_y0 + 20);

    // Restore default font
    GetHAL().canvas.setFont(FONT_REPL);

    GetHAL().pushCanvas();
}

void AppClock::render_timer()
{
    const int64_t now_ms  = GetHAL().millis();
    const bool is_paused  = _timer_paused_ms != 0;
    int64_t remaining_ms  = is_paused ? (_timer_end_ms - _timer_paused_ms) : (_timer_end_ms - now_ms);
    const bool is_overdue = remaining_ms < 0;
    if (is_overdue) {
        remaining_ms = -remaining_ms;
    }

    const bool show_colon   = is_paused || ((remaining_ms / 20) % 5 >= 2);
    const char colon_symbol = show_colon ? ':' : ' ';

    const auto hours   = static_cast<int>(remaining_ms / 3600000);
    const auto minutes = static_cast<int>((remaining_ms / 60000) % 60);
    const auto seconds = static_cast<int>((remaining_ms / 1000) % 60);
    const auto hundred = static_cast<int>((remaining_ms / 10) % 100);

    auto time_str = fmt::format("{:02d}{}{:02d}{}{:02d}", hours, colon_symbol, minutes, colon_symbol, seconds);
    auto ms_str   = fmt::format(".{:02d}", hundred);

    GetHAL().canvas.fillScreen(THEME_COLOR_BG);
    GetHAL().canvas.setCursor(5, 5);
    GetHAL().canvas.setTextSize(1);
    if (is_paused) {
        GetHAL().canvas.setTextColor(TFT_LIGHTGREY, THEME_COLOR_BG);
        GetHAL().canvas.print("Timer - paused");
    } else if (is_overdue) {
        GetHAL().canvas.setTextColor(TFT_RED, THEME_COLOR_BG);
        GetHAL().canvas.print("Timer - overflow");
    } else {
        GetHAL().canvas.setTextColor(TFT_GREENYELLOW, THEME_COLOR_BG);
        GetHAL().canvas.print("Timer - running");
    }

    GetHAL().canvas.setTextColor(TFT_ORANGE, THEME_COLOR_BG);
    GetHAL().canvas.setTextSize(2);
    GetHAL().canvas.drawCenterString(time_str.c_str(), GetHAL().canvas.width() / 2 - 15,
                                     GetHAL().canvas.height() / 2 - 25);

    GetHAL().canvas.setTextSize(1);
    GetHAL().canvas.drawCenterString(ms_str.c_str(), GetHAL().canvas.width() / 2 + 65,
                                     GetHAL().canvas.height() / 2 - 25 + 14);

    GetHAL().canvas.setTextColor(TFT_LIGHTGREY, THEME_COLOR_BG);
    GetHAL().canvas.setFont(FONT_SMALL);
    const int footer_y0 = GetHAL().canvas.height() - 35;
    GetHAL().canvas.drawString("right/left +-1m  up/down +-30m", 5, footer_y0);
    if (is_paused) {
        GetHAL().canvas.drawString("[ANY] continue", 5, footer_y0 + 10);
    } else if (is_overdue) {
        GetHAL().canvas.drawString("[ANY] stop", 5, footer_y0 + 10);
    } else {
        GetHAL().canvas.drawString("[ANY] pause", 5, footer_y0 + 10);
    }
    GetHAL().canvas.drawString("[esc] cancel & back", 5, footer_y0 + 20);
    // Restore default font
    GetHAL().canvas.setFont(FONT_REPL);

    GetHAL().pushCanvas();
}

void AppClock::handle_key_event(const Keyboard::KeyEvent_t& keyEvent)
{
    if (!keyEvent.state || keyEvent.isModifier) {
        return;
    }

    const auto now_ms = GetHAL().millis();

    constexpr auto _key_esc = KEY_GRAVE;
    if (keyEvent.keyCode == _key_esc) {
        _stopwatch_start_ms  = 0;
        _stopwatch_paused_ms = 0;
        _timer_end_ms        = 0;
        _timer_paused_ms     = 0;
        GetHAL().speaker.setVolume(_saved_volume);
        update_time_display();
        _time_count = now_ms;
        return;
    }

    auto adjust_timer = [&](int64_t delta_ms) {
        if (!_timer_end_ms) {
            return;
        }
        if (_timer_paused_ms) {
            const int64_t paused_remaining = _timer_end_ms - _timer_paused_ms;
            const int64_t new_remaining    = std::max<int64_t>(paused_remaining + delta_ms, 1000);
            _timer_end_ms                  = _timer_paused_ms + new_remaining;
        } else {
            const int64_t new_remaining = std::max<int64_t>(_timer_end_ms - now_ms + delta_ms, 1000);
            _timer_end_ms               = now_ms + new_remaining;
        }
    };

    if (_stopwatch_start_ms && keyEvent.keyCode == KEY_BACKSPACE) {
        _stopwatch_start_ms = now_ms;  // Reset stopwatch
        if (_stopwatch_paused_ms) {
            _stopwatch_paused_ms = now_ms;
        }
        return;
    }

    // not reqired to press [fn] for arrow keys
    constexpr auto _key_up = KEY_SEMICOLON;
    constexpr auto _key_down = KEY_DOT;
    constexpr auto _key_left = KEY_COMMA;
    constexpr auto _key_right = KEY_SLASH;

    if (_timer_end_ms && (keyEvent.keyCode == _key_up || keyEvent.keyCode == _key_down)) {
        const int64_t delta = (keyEvent.keyCode == _key_up) ? (30 * 60 * 1000) : -(30 * 60 * 1000);
        adjust_timer(delta);
        return;
    }

    if (_timer_end_ms && (keyEvent.keyCode == _key_right || keyEvent.keyCode == _key_left)) {
        const int64_t delta = (keyEvent.keyCode == _key_right) ? (60 * 1000) : -(60 * 1000);
        adjust_timer(delta);
        return;
    }

    if ((_timer_end_ms && _timer_paused_ms) ||
        (keyEvent.keyCode == KEY_T && !_stopwatch_start_ms && !_timer_end_ms)) {
        if (_timer_paused_ms) {
            _timer_end_ms = now_ms + (_timer_end_ms - _timer_paused_ms);
        } else {
            _timer_end_ms = now_ms + 10 * 1000;  // Default 10s timer
        }
        _timer_paused_ms = 0;
        return;
    }

    if ((_stopwatch_start_ms && _stopwatch_paused_ms) ||
        (keyEvent.keyCode == KEY_S && !_stopwatch_start_ms && !_timer_end_ms)) {
        if (_stopwatch_paused_ms) {
            _stopwatch_start_ms = now_ms - (_stopwatch_paused_ms - _stopwatch_start_ms);
        } else {
            _stopwatch_start_ms = now_ms;
        }
        _stopwatch_paused_ms = 0;
        return;
    }

    // Pause current mode when any other key is pressed
    if (_stopwatch_start_ms) {
        _stopwatch_paused_ms = now_ms;
        return;
    }

    if (_timer_end_ms) {
        if (now_ms > _timer_end_ms) {
            _timer_end_ms    = 0;
            _timer_paused_ms = 0;
            GetHAL().speaker.setVolume(_saved_volume);
        } else {
            _timer_paused_ms = now_ms;
        }
    }
}
