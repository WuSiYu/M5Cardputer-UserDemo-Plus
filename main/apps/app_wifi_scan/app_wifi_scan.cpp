/*
 * SPDX-FileCopyrightText: 2025 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
#include "app_wifi_scan.h"
#include "assets/scan_big.h"
#include "assets/scan_small.h"
#include <apps/utils/audio/audio.h>
#include <apps/utils/common.h>
#include <apps/utils/theme.h>
#include <mooncake_log.h>
#include <assets.h>

using namespace mooncake;

AppWifiScan::AppWifiScan()
{
    setAppInfo().name     = "Scan";
    setAppInfo().userData = new AppIcon_t(image_data_scan_big, image_data_scan_small);
    _time_count           = 0;
    _scan_interval_ms     = 5000;
    _short_interval       = false;
    _page                 = 0;
    _is_scanning          = false;
    _is_first_scan        = true;
}

AppWifiScan::~AppWifiScan()
{
    delete static_cast<AppIcon_t*>(getAppInfo().userData);
}

void AppWifiScan::onOpen()
{
    mclog::tagInfo(getAppInfo().name, "on open");

    GetHAL().wifiInit();
    _key_event_slot_id = GetHAL().keyboard.onKeyEvent.connect(
        [this](const Keyboard::KeyEvent_t& keyEvent) { handle_key_event(keyEvent); });

    // Force immediate first scan
    _time_count = GetHAL().millis() - _scan_interval_ms;
    render_page_scanning();
}

void AppWifiScan::onRunning()
{
    const auto now_ms = GetHAL().millis();

    // Kick off scan when interval elapses and no scan in progress
    if (!_is_scanning && (now_ms - _time_count > _scan_interval_ms)) {
        if (GetHAL().wifiScanStartAsync()) {
            _is_scanning = true;
            if (_is_first_scan) {
                _is_first_scan = false;
            } else {
                render_page_result();
            }
        } else {
            _time_count = now_ms;  // avoid tight loop if start fails
        }
    }

    // Collect results when scan completes
    if (_is_scanning) {
        if (GetHAL().wifiScanCollect(_scan_result)) {
            normalize_page();
            _time_count = now_ms;
            _is_scanning = false;
            render_page_result();
        }
    }

    // Close app when home button clicked
    if (GetHAL().homeButton.wasClicked()) {
        audio::play_random_tone();
        close();
    }
}

void AppWifiScan::onClose()
{
    mclog::tagInfo(getAppInfo().name, "on close");

    if (_key_event_slot_id >= 0) {
        GetHAL().keyboard.onKeyEvent.disconnect(_key_event_slot_id);
        _key_event_slot_id = -1;
    }
}

void AppWifiScan::render_page_scanning()
{
    GetHAL().canvas.setBaseColor(THEME_COLOR_BG);
    GetHAL().canvas.setTextScroll(false);
    GetHAL().canvas.setTextSize(1);
    GetHAL().canvas.fillScreen(THEME_COLOR_BG);
    GetHAL().canvas.setCursor(10, 5);
    GetHAL().canvas.setTextColor(TFT_ORANGE, THEME_COLOR_BG);
    GetHAL().canvas.println("Scanning WiFi...");
    GetHAL().pushCanvas();
}

void AppWifiScan::render_page_result()
{
    // Clear screen and display results
    GetHAL().canvas.fillScreen(THEME_COLOR_BG);
    GetHAL().canvas.setFont(FONT_BASIC);
    GetHAL().canvas.setCursor(0, 0);
    GetHAL().canvas.setTextSize(1);

    if (_scan_result.empty()) {
        GetHAL().canvas.setTextColor(TFT_RED, THEME_COLOR_BG);
        GetHAL().canvas.println("No WiFi networks found");
    } else {
        const int start_idx = _page * PAGE_SIZE;
        const int end_idx   = std::min<int>(_scan_result.size(), start_idx + PAGE_SIZE);

        int line = 0;
        for (int i = start_idx; i < end_idx; ++i) {
            const auto& result = _scan_result[i];

            // Color by signal strength
            if (result.rssi > -65) {
                GetHAL().canvas.setTextColor(TFT_GREEN, THEME_COLOR_BG);
            } else if (result.rssi > -90) {
                GetHAL().canvas.setTextColor(TFT_YELLOW, THEME_COLOR_BG);
            } else {
                GetHAL().canvas.setTextColor(TFT_RED, THEME_COLOR_BG);
            }

            GetHAL().canvas.setCursor(0, line * FONT_HEIGHT - 1);
            GetHAL().canvas.printf("%d %02d %s\n", result.rssi, result.channel,
                                    result.ssid.substr(0, 18).c_str());
            line++;
        }
    }

    // Footer info
    GetHAL().canvas.setTextColor(TFT_LIGHTGREY, THEME_COLOR_BG);
    GetHAL().canvas.setFont(FONT_SMALL);
    GetHAL().canvas.setTextSize(1);
    const auto total_pages = std::max<int>(1, (_scan_result.size() + PAGE_SIZE - 1) / PAGE_SIZE);
    auto header_str        = fmt::format("RSSI  CH  SSID    {}{}ms APs:", _is_scanning ? "*" : " ", _scan_interval_ms);
    GetHAL().canvas.drawCenterString(header_str.c_str(), 91, GetHAL().canvas.height() - 10);
    auto page_str = fmt::format("{}/{}", _page + 1, total_pages);
    GetHAL().canvas.drawCenterString(page_str.c_str(), GetHAL().canvas.width() - 15, 0);

    // Restore default font
    GetHAL().canvas.setFont(FONT_BASIC);
    GetHAL().canvas.drawCenterString(std::to_string(_scan_result.size()).c_str(),
                                     GetHAL().canvas.width() - 15, GetHAL().canvas.height() - 17);

    GetHAL().pushCanvas();
}

void AppWifiScan::handle_key_event(const Keyboard::KeyEvent_t& keyEvent)
{
    if (!keyEvent.state || keyEvent.isModifier) {
        return;
    }

    auto is_prev_page = keyEvent.keyCode == KEY_LEFT || keyEvent.keyCode == KEY_UP ||
                        keyEvent.keyCode == KEY_SEMICOLON || keyEvent.keyCode == KEY_COMMA;
    auto is_next_page = keyEvent.keyCode == KEY_RIGHT || keyEvent.keyCode == KEY_DOWN ||
                        keyEvent.keyCode == KEY_DOT || keyEvent.keyCode == KEY_SLASH;

    if (is_prev_page || is_next_page) {
        const int total_pages = std::max<int>(1, (_scan_result.size() + PAGE_SIZE - 1) / PAGE_SIZE);
        if (is_prev_page && _page > 0) {
            _page--;
        } else if (is_next_page && _page < total_pages - 1) {
            _page++;
        }
        render_page_result();
        return;
    }

    // Toggle interval on any other key
    _short_interval   = !_short_interval;
    _scan_interval_ms = _short_interval ? 2000 : 5000;
    _time_count       = 0;  // force next scan soon
    render_page_result();
}

void AppWifiScan::normalize_page()
{
    const int total_pages = std::max<int>(1, (_scan_result.size() + PAGE_SIZE - 1) / PAGE_SIZE);
    if (_page >= total_pages) {
        _page = total_pages - 1;
    }
    if (_page < 0) {
        _page = 0;
    }
}
