/*
 * SPDX-FileCopyrightText: 2025 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
#pragma once
#include <mooncake.h>
#include <cstdint>
#include <hal/hal.h>

/**
 * @brief
 *
 */
class AppWifiScan : public mooncake::AppAbility {
public:
    AppWifiScan();
    ~AppWifiScan();

    void onOpen() override;
    void onRunning() override;
    void onClose() override;

private:
    std::vector<Hal::ScanResult_t> _scan_result;
    uint32_t _time_count = 0;
    uint32_t _scan_interval_ms = 5000;
    bool _short_interval       = false;
    int _page                  = 0;
    bool _is_scanning          = false;
    bool _is_first_scan        = true;
    int _key_event_slot_id     = -1;

    static constexpr int PAGE_SIZE = 6;

    void render_page_scanning();
    void render_page_result();
    void handle_key_event(const Keyboard::KeyEvent_t& keyEvent);
    void normalize_page();
};
