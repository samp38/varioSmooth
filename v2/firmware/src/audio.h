/*
 * Piezo beeper, ported from the v1 firmware (audio.c, vario_beep_tick()).
 *
 * The piezo sits between two pins driven in opposite phase by one PWM
 * peripheral: the voltage swing doubles, like the v1 "boost" volume where the
 * comparator produced the inverted signal. No volume control yet: full volume.
 *
 * Call tick() once per sample with the vario speed index: beep length, silence
 * and tone follow the v1 tables (audio_response.h), silent when sinking.
 */

#pragma once

#include <Arduino.h>

class Audio {
public:
    bool begin(uint8_t pinA, uint8_t pinB);
    void tick(int speedIndex);
    // Square wave at freqHz, 0 = silence
    void tone(uint32_t freqHz);

private:
    uint8_t pinA = 0, pinB = 0;
    uint32_t countdown = 0;     // samples until the next beep / silence switch
    bool beeping = false;
};
