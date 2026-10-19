/*
 * Push button, polled once per vario sample: the 20 ms period filters the
 * bounces. Pressed = low level, internal pull-up.
 *
 * As in v1 (button.c): short press on release, long press (> 0.5 s) reported
 * as soon as it is reached, without waiting for the release.
 */

#pragma once

#include <Arduino.h>

class Button {
public:
    enum Event { NONE, SHORT_PRESS, LONG_PRESS };

    void begin(uint8_t pin);
    Event poll();
    bool pressed() const { return digitalRead(pin) == LOW; }

private:
    uint8_t pin = 0;
    uint32_t pressedSamples = 0;
    // Set while a press that must not count is held down (power-on press,
    // or the rest of a long press)
    bool ignore = false;
};
