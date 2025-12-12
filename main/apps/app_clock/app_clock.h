/*
 * SPDX-FileCopyrightText: 2025 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
#pragma once
#include <mooncake.h>
#include <cstdint>
#include <hal/hal.h>
#include <ctime>

/**
 * @brief
 *
 */
class AppClock : public mooncake::AppAbility {
public:
    AppClock();
    ~AppClock();

    void onOpen() override;
    void onRunning() override;
    void onClose() override;

private:
    static constexpr uint32_t UPDATE_INTERVAL = 1000;  // Update every 1 second
    uint32_t _time_count                      = 0;
    uint32_t _last_alarm_ms                   = 0;
    int _key_event_slot_id                    = -1;
    int _saved_volume                         = 0;

    int64_t _stopwatch_start_ms = 0;
    int64_t _stopwatch_paused_ms = 0;
    int64_t _timer_end_ms       = 0;
    int64_t _timer_paused_ms    = 0;

    void render_interface();
    void update_time_display();
    void show_network_time();
    void show_system_time();
    void render_stopwatch();
    void render_timer();
    void handle_key_event(const Keyboard::KeyEvent_t& keyEvent);
};
