#include "ms5611.h"

namespace {
constexpr uint8_t ADDR_CSB_LOW = 0x77;
constexpr uint8_t ADDR_CSB_HIGH = 0x76;

constexpr uint8_t CMD_RESET = 0x1E;
constexpr uint8_t CMD_CONVERT_D1_4096 = 0x48;   // pressure
constexpr uint8_t CMD_CONVERT_D2_1024 = 0x54;   // temperature
constexpr uint8_t CMD_ADC_READ = 0x00;
constexpr uint8_t CMD_PROM_READ = 0xA0;         // + 2 * word index

constexpr uint32_t RESET_MS = 3;                // 2.8 ms max
}

bool MS5611::begin(TwoWire &wire) {
    bus = &wire;
    const uint8_t candidates[] = {ADDR_CSB_LOW, ADDR_CSB_HIGH};
    for (uint8_t a : candidates) {
        bus->beginTransmission(a);
        if (bus->endTransmission() == 0) {
            addr = a;
            break;
        }
    }
    if (addr == 0) {
        return false;
    }
    if (!command(CMD_RESET)) {
        return false;
    }
    delay(RESET_MS);
    return readProm();
}

bool MS5611::command(uint8_t cmd) {
    bus->beginTransmission(addr);
    bus->write(cmd);
    return bus->endTransmission() == 0;
}

bool MS5611::readProm() {
    for (uint8_t i = 0; i < 8; i++) {
        if (!command(CMD_PROM_READ + 2 * i)) {
            return false;
        }
        if (bus->requestFrom(addr, (uint8_t)2) != 2) {
            return false;
        }
        c[i] = (uint16_t)bus->read() << 8;
        c[i] |= bus->read();
    }
    // A PROM full of 0x0000 or 0xFFFF would pass some CRC checks
    if (c[1] == 0 || c[1] == 0xFFFF) {
        return false;
    }
    return crc4(c) == (c[7] & 0x000F);
}

// CRC4 from the MS5611 datasheet / application note AN520
uint8_t MS5611::crc4(const uint16_t prom[8]) {
    uint16_t rem = 0;
    for (uint8_t cnt = 0; cnt < 16; cnt++) {
        uint16_t word = prom[cnt >> 1];
        if (cnt == 15) {
            word &= 0xFF00;             // CRC nibble itself is excluded
        }
        rem ^= (cnt & 1) ? (word & 0x00FF) : (word >> 8);
        for (uint8_t bit = 8; bit > 0; bit--) {
            rem = (rem & 0x8000) ? (rem << 1) ^ 0x3000 : (rem << 1);
        }
    }
    return (rem >> 12) & 0x000F;
}

bool MS5611::startPressure() {
    return command(CMD_CONVERT_D1_4096);
}

bool MS5611::startTemperature() {
    return command(CMD_CONVERT_D2_1024);
}

bool MS5611::readAdc(uint32_t &raw) {
    if (!command(CMD_ADC_READ)) {
        return false;
    }
    if (bus->requestFrom(addr, (uint8_t)3) != 3) {
        return false;
    }
    raw = (uint32_t)bus->read() << 16;
    raw |= (uint32_t)bus->read() << 8;
    raw |= bus->read();
    // The sensor returns 0 when no conversion was completed
    return raw != 0;
}

void MS5611::setTemperatureRaw(uint32_t d2) {
    dT = (int32_t)d2 - ((int32_t)c[5] << 8);
    temp = 2000 + (int32_t)(((int64_t)dT * c[6]) >> 23);

    // Second order compensation, below 20 °C
    int32_t t2 = 0;
    off2 = 0;
    sens2 = 0;
    if (temp < 2000) {
        int64_t d = temp - 2000;
        t2 = (int32_t)(((int64_t)dT * dT) >> 31);
        off2 = 5 * d * d / 2;
        sens2 = 5 * d * d / 4;
        if (temp < -1500) {
            int64_t d2c = temp + 1500;
            off2 += 7 * d2c * d2c;
            sens2 += 11 * d2c * d2c / 2;
        }
    }
    temp -= t2;
}

int32_t MS5611::compensatePressure(uint32_t d1) const {
    int64_t off = ((int64_t)c[2] << 16) + (((int64_t)c[4] * dT) >> 7) - off2;
    int64_t sens = ((int64_t)c[1] << 15) + (((int64_t)c[3] * dT) >> 8) - sens2;
    return (int32_t)(((((int64_t)d1 * sens) >> 21) - off) >> 15);
}
