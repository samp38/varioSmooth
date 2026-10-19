#include "button.h"

#include "vario.h"

namespace {
constexpr uint32_t DEBOUNCE_SAMPLES = 2;                        // 40 ms
constexpr uint32_t LONG_PRESS_SAMPLES = Vario::SAMPLE_RATE / 2; // 0.5 s, as in v1
}

void Button::begin(uint8_t p) {
    pin = p;
    pinMode(pin, INPUT_PULLUP);
    // The press that woke the board up must not change the volume
    ignore = pressed();
    pressedSamples = 0;
}

Button::Event Button::poll() {
    if (pressed()) {
        if (ignore) {
            return NONE;
        }
        if (++pressedSamples == LONG_PRESS_SAMPLES) {
            ignore = true;
            pressedSamples = 0;     // no short press on release
            return LONG_PRESS;
        }
        return NONE;
    }

    uint32_t n = pressedSamples;
    pressedSamples = 0;
    ignore = false;
    // Shorter: a bounce or a glitch
    return (n >= DEBOUNCE_SAMPLES) ? SHORT_PRESS : NONE;
}
