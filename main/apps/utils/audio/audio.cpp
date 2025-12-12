/*
 * SPDX-FileCopyrightText: 2025 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
#include <vector>
#include <cmath>
#include <cstdlib>
#include <cstdint>
#include <algorithm>
#include <hal/hal.h>
#include <mooncake_log.h>
#include <random>

namespace audio {

const int c_major_scale[] = {60, 62, 64, 65, 67, 69, 71};  // C大调音阶（C D E F G A B）

void play_tone(int frequency, double durationSec)
{
    if (GetHAL().speaker.getVolume() <= 0) {
        return;
    }

    const int sample_rate = GetHAL().speaker.config().sample_rate;
    const int samples     = static_cast<int>(sample_rate * durationSec);
    std::vector<int16_t> buffer(samples * 2);  // 双声道

    const int fade_len    = 200;  // 淡出长度（采样点）
    const float amplitude = 32767.0f / 5;

    for (int i = 0; i < samples; ++i) {
        float amp = amplitude;

        // 应用结尾淡出（fade-out）
        if (i >= samples - fade_len) {
            float fade_factor = static_cast<float>(samples - i) / fade_len;
            amp *= fade_factor;
        }

        int16_t value     = static_cast<int16_t>(amp * sin(2.0 * M_PI * frequency * i / sample_rate));
        buffer[i * 2]     = value;  // 左声道
        buffer[i * 2 + 1] = value;  // 右声道
    }

    GetHAL().speaker.playRaw(buffer.data(), buffer.size());
}

void play_melody(const std::vector<int>& midiList, double durationSec = 0.1)
{
    if (GetHAL().speaker.getVolume() <= 0) {
        return;
    }

    const int sample_rate      = GetHAL().speaker.config().sample_rate;
    const int samples_per_note = static_cast<int>(sample_rate * durationSec);
    const int fade_len         = 200;  // 每个音符结尾的淡出长度
    const float amplitude      = 32767.0f / 5;

    std::vector<int16_t> buffer;                             // 大 buffer 存放整首旋律
    buffer.reserve(midiList.size() * samples_per_note * 2);  // 双声道预留空间

    for (int midiNote : midiList) {
        for (int i = 0; i < samples_per_note; ++i) {
            float amp = amplitude;

            // 应用淡出（仅每个音符的结尾）
            if (i >= samples_per_note - fade_len) {
                float fade_factor = static_cast<float>(samples_per_note - i) / fade_len;
                amp *= fade_factor;
            }

            int16_t sample = 0;
            if (midiNote >= 0) {
                double freq = 440.0 * pow(2.0, (midiNote - 69) / 12.0);
                sample      = static_cast<int16_t>(amp * sin(2.0 * M_PI * freq * i / sample_rate));
            }

            buffer.push_back(sample);  // 左声道
            buffer.push_back(sample);  // 右声道
        }
    }

    GetHAL().speaker.playRaw(buffer.data(), buffer.size());
}

void play_tone_from_midi(int midi, double durationSec)
{
    if (GetHAL().speaker.getVolume() <= 0) {
        return;
    }

    double freq = 440.0 * std::pow(2.0, (midi - 69) / 12.0);
    play_tone(static_cast<int>(freq), durationSec);
}

void play_random_tone(int semitoneShift = 0, double durationSec = 0.15)
{
    if (GetHAL().speaker.getVolume() <= 0) {
        return;
    }

    // using simple random method to save DIRAM
    int rnd = static_cast<int>(GetHAL().millis() % 0xFFFFFFFF);
    rnd = (rnd * 1103515245 + 12345) & 0x7FFFFFFF;
    int index = rnd % (sizeof(c_major_scale) / sizeof(c_major_scale[0]));
    int midi  = c_major_scale[index] + semitoneShift;

    play_tone_from_midi(midi, durationSec);
}

/* -------------------------------------------------------------------------- */
/*                                  Keyboard                                  */
/* -------------------------------------------------------------------------- */
static void _keyboard_sfx_on_key_event(const Keyboard::KeyEvent_t& event)
{
    if (!event.state) {
        return;
    }

    auto old_volume = GetHAL().speaker.getVolume();
    GetHAL().speaker.setVolume(90);

    int semitoneShift = 48;
    switch (event.keyCode) {
        case KEY_1:
            play_tone_from_midi(c_major_scale[0] + semitoneShift, 0.02);
            break;
        case KEY_2:
            play_tone_from_midi(c_major_scale[1] + semitoneShift, 0.02);
            break;
        case KEY_3:
            play_tone_from_midi(c_major_scale[2] + semitoneShift, 0.02);
            break;
        case KEY_4:
            play_tone_from_midi(c_major_scale[3] + semitoneShift, 0.02);
            break;
        case KEY_5:
            play_tone_from_midi(c_major_scale[4] + semitoneShift, 0.02);
            break;
        case KEY_6:
            play_tone_from_midi(c_major_scale[5] + semitoneShift, 0.02);
            break;
        case KEY_7:
            play_tone_from_midi(c_major_scale[6] + semitoneShift, 0.02);
            break;
        default:
            play_random_tone(semitoneShift, 0.02);
    }
    GetHAL().speaker.setVolume(old_volume);
}

void set_keyboard_sfx_enable(bool enable)
{
    static size_t slot_id  = 0;
    static bool is_enabled = false;

    mclog::tagInfo("audio", "set keyboard sfx enable: {}", enable);
    if (enable) {
        if (is_enabled) {
            return;
        }
        is_enabled = true;
        slot_id    = GetHAL().keyboard.onKeyEvent.connect(_keyboard_sfx_on_key_event);
    } else {
        if (!is_enabled) {
            return;
        }
        is_enabled = false;
        GetHAL().keyboard.onKeyEvent.disconnect(slot_id);
    }
}

}  // namespace audio