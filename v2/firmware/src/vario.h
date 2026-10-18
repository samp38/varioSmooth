/*
 * Vertical speed computation, ported from the v1 firmware (filters.c, vario.c).
 *
 * Call push() at SAMPLE_RATE with the compensated pressure. Same chain as v1:
 *   - 2nd order low-pass IIR on the pressure;
 *   - speed = altitude difference over a window of 10 to 50 samples, shorter
 *     when climbing fast, refreshed every 8 samples (5 above index 50);
 *   - 1st order low-pass IIR on the speed.
 * Differences with v1: floating point (no overflow traps), and the speed is
 * computed on the altitude, in cm/s, instead of the raw pressure. The v1 unit
 * was 8 * Pa/s, i.e. ~0.94 cm/s near sea level and drifting with altitude.
 */

#pragma once

#include <Arduino.h>

class Vario {
public:
    static constexpr uint32_t SAMPLE_RATE = 50;     // Hz

    // Audio scale from v1 (audio_response.h)
    static constexpr int SPEED_THRESHOLD = 10;      // cm/s
    static constexpr int SPEED_STEP = 9;            // cm/s per index
    static constexpr int SPEED_INDEX_MAX = 99;

    // Initializes the filters on a first sample, avoiding the start-up transient
    void begin(int32_t pressurePa);
    void push(int32_t pressurePa);

    float pressure() const { return (float)p1; }    // filtered, Pa
    float altitude() const;                         // m, standard atmosphere (QNH 1013.25 hPa)
    float climb() const { return (float)speed; }    // filtered, cm/s
    // Audio index, < 0 when sinking, clamped to SPEED_INDEX_MAX
    int speedIndex() const { return index; }

private:
    static constexpr int BUFFER_SIZE = 61;

    static double pressureToAltitude(double pressurePa);
    void updateSpeed();

    // Pressure filter state
    double x1 = 0, x2 = 0, p1 = 0, p2 = 0;
    // Filtered altitudes, ring buffer
    double alt[BUFFER_SIZE] = {0};
    int altI = 0;
    // Speed filter state
    double speedIn1 = 0;
    double speed = 0;
    int index = 0;
    int updateCountdown = 0;
};
