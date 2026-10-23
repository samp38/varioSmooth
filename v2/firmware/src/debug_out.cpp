#include "debug_out.h"

#if DEBUG_OUTPUT == 2

#include <SEGGER_RTT.h>

namespace {
class RttPrint : public Print {
public:
    size_t write(uint8_t c) override { return write(&c, 1); }
    size_t write(const uint8_t *buffer, size_t size) override {
        return SEGGER_RTT_Write(0, buffer, size);
    }
};

RttPrint rtt;
}

Print &dbg = rtt;

void debugBegin() {
    SEGGER_RTT_Init();
}

#else

Print &dbg = Serial;

void debugBegin() {
    if (DEBUG_ENABLED) {
        // No wait for the USB serial (re-enumerated at every wake-up): the
        // start-up info is printed at the end of the warm-up
        Serial.begin(115200);
    }
}

#endif
