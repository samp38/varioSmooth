/*
 * Piezo beeper, ported from the v1 firmware (audio.c, vario_beep_tick()).
 *
 * Wiring, all pins on one PWM peripheral:
 *
 *   pinA ──[ 220 Ω ]──┬── piezo ── pinB
 *   pinWeak ──[ R ]───┘              (R ~1 kΩ, sets the MEDIUM volume)
 *
 * 3 volume levels as in v1:
 *   - BOOST: pinA and pinB in opposite phase, the voltage swing doubles (v1
 *     used the comparator to produce the inverted signal);
 *   - HIGH: pinA only, pinB held low (v1: digital pot at 0 ohm);
 *   - MEDIUM: pinWeak only, pinA high impedance (v1: digital pot in series).
 *     R and the piezo capacitance round the edges, which carry most of the
 *     sound: a shorter duty cycle does not work, every edge rings the same.
 *
 * Call tick() once per sample with the vario speed index: beep length, silence
 * and tone follow the v1 tables (audio_response.h), silent when sinking.
 * play() starts a short melody that takes over the beeps until it ends.
 */

#pragma once

#include <Arduino.h>

#include "vario.h"

class Audio {
public:
    enum Volume : uint8_t { VOLUME_MEDIUM, VOLUME_HIGH, VOLUME_BOOST, VOLUME_COUNT };

    // Melody note, a {0, 0} note ends the melody
    struct Note {
        uint16_t freqHz;    // 0 = silence
        uint8_t samples;    // duration, in vario samples
    };
    static constexpr uint8_t ms(uint32_t durationMs) {
        return (uint8_t)(durationMs * Vario::SAMPLE_RATE / 1000);
    }
    // v1 melodies
    static const Note STARTUP[];
    static const Note VOLUME_CHANGE[];
    static const Note SHUTDOWN[];

    static constexpr int SILENT = -1;   // any speed index < 0 is silent

    bool begin(uint8_t pinA, uint8_t pinB, uint8_t pinWeak);
    void setVolume(Volume v);
    Volume volume() const { return vol; }
    void nextVolume() { setVolume((Volume)((vol + 1) % VOLUME_COUNT)); }

    void play(const Note *melody);
    bool playing() const { return melody != nullptr; }

    void tick(int speedIndex);
    // Square wave at freqHz, 0 = silence
    void tone(uint32_t freqHz);

private:
    uint8_t pinA = 0, pinB = 0, pinWeak = 0;
    uint8_t pinOut = 0;         // pinA or pinWeak, depending on the volume
    Volume vol = VOLUME_BOOST;
    uint32_t countdown = 0;     // samples until the next beep / silence switch
    bool beeping = false;
    const Note *melody = nullptr;
    uint32_t noteCountdown = 0;
};
