#include "vario.h"

#include <math.h>

namespace {
// Pressure low-pass, v1 coefficients (integer form divided by 2048):
// y = (16 x + 32 x1 + 16 x2 + 3553 y1 - 1569 y2) / 2048, unity DC gain
constexpr double PB0 = 16.0 / 2048, PB1 = 32.0 / 2048, PB2 = 16.0 / 2048;
constexpr double PA1 = 3553.0 / 2048, PA2 = -1569.0 / 2048;

// Speed low-pass (maxflat Wn = 0.01), v1 coefficients:
// y = (73 (x + x1) + 854 y1) / 1000, unity DC gain
constexpr double SB0 = 73.0 / 1000, SA1 = 854.0 / 1000;
}

void Vario::begin(int32_t pressurePa) {
    x1 = x2 = p1 = p2 = pressurePa;
    double a = pressureToAltitude(pressurePa);
    for (double &v : alt) {
        v = a;
    }
    altI = 0;
    speedIn1 = speed = 0;
    index = 0;
    updateCountdown = 0;
}

void Vario::push(int32_t pressurePa) {
    double x = pressurePa;
    double y = PB0 * x + PB1 * x1 + PB2 * x2 + PA1 * p1 + PA2 * p2;
    x2 = x1;
    x1 = x;
    p2 = p1;
    p1 = y;

    altI = (altI + 1) % BUFFER_SIZE;
    alt[altI] = pressureToAltitude(y);

    if (updateCountdown-- == 0) {
        updateSpeed();
        updateCountdown = (abs(index) < 50) ? 8 : 5;
    }
}

void Vario::updateSpeed() {
    int window;
    if (index < 5) {
        window = 50;
    } else if (index < 10) {
        window = 30;
    } else if (index < 30) {
        window = 25;
    } else {
        window = 10;
    }

    int i = altI - window;
    if (i < 0) {
        i += BUFFER_SIZE;
    }
    double raw = (alt[altI] - alt[i]) * 100.0 * SAMPLE_RATE / window;

    speed = SB0 * (raw + speedIn1) + SA1 * speed;
    speedIn1 = raw;

    // Truncation toward zero, as the v1 integer division
    index = (int)((speed - SPEED_THRESHOLD) / SPEED_STEP);
    if (index > SPEED_INDEX_MAX) {
        index = SPEED_INDEX_MAX;
    }
}

float Vario::altitude() const {
    return (float)alt[altI];
}

double Vario::pressureToAltitude(double pressurePa) {
    return 44330.77 * (1.0 - pow(pressurePa / 101325.0, 0.190263));
}
