/*
 * MS5611 barometric sensor driver (I2C).
 *
 * Pressure is returned in Pa (= 0.01 mbar, the sensor's native resolution),
 * temperature in 0.01 °C. Second order temperature compensation and the
 * PROM CRC check from the datasheet are implemented (they were missing in v1).
 *
 * Usage: start a conversion, wait the conversion time, read the ADC.
 * The I2C version has no end-of-conversion signal: timing is up to the caller.
 */

#pragma once

#include <Arduino.h>
#include <Wire.h>

class MS5611 {
public:
    // Values for delay(): with the 1024 Hz FreeRTOS tick, delay(n) can return
    // up to one tick (~1 ms) early.
    // Pressure, OSR 4096: 9.04 ms max conversion time
    static constexpr uint32_t PRESSURE_CONVERSION_MS = 11;
    // Temperature, OSR 1024: 2.28 ms max (v1 used OSR 512, 1.17 ms max)
    static constexpr uint32_t TEMPERATURE_CONVERSION_MS = 4;

    // Probes 0x77 (CSB low) then 0x76 (CSB high), resets the sensor,
    // reads the PROM and checks its CRC. Returns false on any failure.
    bool begin(TwoWire &wire = Wire);
    uint8_t address() const { return addr; }

    bool startPressure();
    bool startTemperature();
    // Returns false on I2C error or if no conversion was ready (raw value 0)
    bool readAdc(uint32_t &raw);

    // Call with a fresh D2 (temperature) value before each compensatePressure().
    // The raw pressure drifts by ~2 mbar/°C (typical coefficients): a stale
    // temperature shows up as a false vertical speed, so measure it every
    // sample, as v1 did.
    void setTemperatureRaw(uint32_t d2);
    int32_t compensatePressure(uint32_t d1) const;
    int32_t temperature() const { return temp; }

private:
    bool command(uint8_t cmd);
    bool readProm();
    static uint8_t crc4(const uint16_t prom[8]);

    TwoWire *bus = nullptr;
    uint8_t addr = 0;
    uint16_t c[8] = {0};    // PROM: c[0] factory data, c[1..6] coefficients, c[7] CRC
    int32_t dT = 0;
    int32_t temp = 2000;
    int64_t off2 = 0;
    int64_t sens2 = 0;
};
