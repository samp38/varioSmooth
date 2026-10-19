#include "ble_link.h"

#include <bluefruit.h>

namespace {
BLEUart bleuart;

uint8_t nmeaChecksum(const char *s) {
    uint8_t cs = 0;
    for (; *s; s++) {
        cs ^= (uint8_t)*s;
    }
    return cs;
}
}

namespace BleLink {

void begin(const char *name) {
    // Larger MTU: a whole $LK8EX1 sentence fits in one notification
    Bluefruit.configPrphBandwidth(BANDWIDTH_MAX);
    // No connection LED: the final board has none, and on the Feather it
    // would stay lit in System OFF (~1 mA) if powered off while lit
    Bluefruit.autoConnLed(false);
    Bluefruit.begin();
    Bluefruit.setName(name);
    Bluefruit.setTxPower(0);

    bleuart.begin();

    Bluefruit.Advertising.addFlags(BLE_GAP_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE);
    Bluefruit.Advertising.addTxPower();
    Bluefruit.Advertising.addService(bleuart);
    Bluefruit.ScanResponse.addName();
    Bluefruit.Advertising.restartOnDisconnect(true);
    Bluefruit.Advertising.setInterval(32, 244);     // 20 ms fast, 152.5 ms slow (units of 0.625 ms)
    Bluefruit.Advertising.setFastTimeout(30);       // s
    Bluefruit.Advertising.start(0);                 // advertise forever
}

bool connected() {
    return Bluefruit.connected() && bleuart.notifyEnabled();
}

void sendLK8EX1(int32_t pressurePa, int climbCmS, int temperatureC, int battery) {
    if (!connected()) {
        return;
    }
    // 9999 means "not available" for the vario field
    climbCmS = constrain(climbCmS, -9998, 9998);

    char body[64];
    snprintf(body, sizeof(body), "LK8EX1,%ld,99999,%d,%d,%d,",
             (long)pressurePa, climbCmS, temperatureC, battery);
    char sentence[72];
    int len = snprintf(sentence, sizeof(sentence), "$%s*%02X\r\n", body, nmeaChecksum(body));
    bleuart.write((const uint8_t *)sentence, len);
}

}
