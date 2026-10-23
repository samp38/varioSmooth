/*
 * Debug output, chosen at build time with DEBUG_OUTPUT:
 *   0 = none (power measurements, flight)
 *   1 = USB serial (Feather): keeps HFCLK running
 *   2 = SEGGER RTT over SWD (helmet board, no USB), read with the probe:
 *       probe-rs attach --chip nRF52840_xxAA .pio/build/<env>/firmware.elf
 *
 * RTT stays in the core's default non-blocking mode (NO_BLOCK_SKIP): without
 * a probe, what does not fit in the 1 KB buffer is dropped and the firmware
 * runs on. Do not copy the Arduino menu setting (BLOCK_IF_FIFO_FULL), which
 * hangs when nothing reads the buffer. An attached probe draws ~1 mA.
 */

#pragma once

#include <Arduino.h>

#ifndef DEBUG_OUTPUT
#define DEBUG_OUTPUT 0
#endif

constexpr bool DEBUG_ENABLED = DEBUG_OUTPUT != 0;

// print / printf / println like Serial; unused when DEBUG_OUTPUT is 0
extern Print &dbg;

void debugBegin();
