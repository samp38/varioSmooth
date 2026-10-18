/*
 * Bluetooth LE link to the flight app (XCTrack).
 *
 * Nordic UART Service, NMEA-like $LK8EX1 sentences on the TX characteristic.
 * The phone provides GPS; we only send pressure, vario and temperature.
 * LK8000 needs an HM-10 style service (FFE0/FFE1) instead: to be added.
 */

#pragma once

#include <Arduino.h>

namespace BleLink {

constexpr int BATTERY_UNKNOWN = 999;

void begin(const char *name);
bool connected();

// pressurePa: Pa, climbCmS: cm/s, temperatureC: °C,
// battery: volts (< 1000), 1000 + percent, or BATTERY_UNKNOWN
void sendLK8EX1(int32_t pressurePa, int climbCmS, int temperatureC, int battery);

}
