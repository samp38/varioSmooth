#include "audio.h"

#include <HardwarePWM.h>

const Audio::Note Audio::STARTUP[] = {{220, ms(80)}, {1100, ms(80)}, {2200, ms(80)}, {0, 0}};
const Audio::Note Audio::VOLUME_CHANGE[] = {
    {220, ms(60)}, {0, ms(60)}, {220, ms(60)}, {0, ms(60)}, {220, ms(60)}, {0, 0}};
const Audio::Note Audio::SHUTDOWN[] = {{2200, ms(80)}, {1100, ms(80)}, {220, ms(80)}, {0, 0}};

namespace {
// PWM3: Tone() uses PWM2, analogWrite() takes any module nobody owns
HardwarePWM &pwm = HwPWM3;
constexpr uint32_t PWM_TOKEN = 0x50495A4F;     // "OZIP"
// 16 MHz / 4: 15-bit counter top down to 123 Hz, tones here are 233 to 930 Hz
constexpr uint8_t PWM_CLOCK_DIV = PWM_PRESCALER_PRESCALER_DIV_4;
constexpr uint32_t PWM_CLOCK_HZ = 16000000 / 4;

struct Level {
    bool weak;      // through pinWeak and its resistor
    bool bridge;    // pinB in opposite phase instead of low
};
const Level LEVELS[Audio::VOLUME_COUNT] = {
    {true, false},  // MEDIUM
    {false, false}, // HIGH
    {false, true},  // BOOST
};

constexpr uint32_t BEEP_DESCENT_PERIOD = 500;

// v1 audio_response.h, by speed index: beep period (beep = 1/2, silence = 1/3)
const uint16_t BEEP_LEN[Vario::SPEED_INDEX_MAX + 1] = {
    2160, 2150, 2140, 2130, 2100, 2070, 1960, 1867, 1787, 1717,
    1657, 1603, 1556, 1513, 1475, 1439, 1407, 1377, 1348, 1322,
    1296, 1272, 1249, 1227, 1206, 1185, 1165, 1145, 1126, 1107,
    1089, 1071, 1054, 1037, 1020, 1003,  987,  971,  956,  940,
     925,  910,  896,  881,  867,  853,  840,  826,  813,  800,
     787,  775,  762,  750,  738,  726,  715,  703,  692,  681,
     670,  659,  649,  638,  628,  618,  608,  598,  589,  579,
     570,  561,  552,  543,  534,  526,  517,  509,  501,  493,
     485,  477,  470,  462,  455,  448,  440,  433,  426,  420,
     413,  406,  400,  393,  387,  381,  375,  369,  363,  357,
};

// v1 audio_response.h, by speed index: tone, played at 2/3 of this value (Hz)
const uint16_t AUDIO_FREQ[Vario::SPEED_INDEX_MAX + 1] = {
     350,  360,  370,  380,  400,  420,  474,  520,  560,  597,
     630,  660,  688,  714,  737,  760,  781,  801,  819,  838,
     854,  870,  886,  900,  914,  928,  941,  954,  966,  977,
     989, 1000, 1011, 1021, 1031, 1041, 1050, 1059, 1069, 1078,
    1086, 1094, 1103, 1111, 1118, 1126, 1134, 1141, 1148, 1155,
    1162, 1169, 1175, 1181, 1188, 1194, 1200, 1206, 1212, 1218,
    1224, 1230, 1235, 1240, 1246, 1251, 1256, 1261, 1267, 1272,
    1276, 1281, 1286, 1290, 1295, 1300, 1304, 1309, 1314, 1318,
    1322, 1326, 1330, 1335, 1339, 1343, 1346, 1351, 1355, 1358,
    1363, 1366, 1370, 1374, 1377, 1381, 1384, 1388, 1391, 1395,
};
}

bool Audio::begin(uint8_t a, uint8_t b, uint8_t weak) {
    pinA = a;
    pinB = b;
    pinWeak = weak;
    pinMode(pinWeak, INPUT);
    if (!pwm.takeOwnership(PWM_TOKEN) || !pwm.addPin(pinB) || !pwm.addPin(pinA)) {
        return false;
    }
    pwm.setClockDiv(PWM_CLOCK_DIV);
    pinOut = pinA;
    setVolume(vol);
    countdown = 0;
    beeping = false;
    return true;
}

void Audio::setVolume(Volume v) {
    vol = v;
    uint8_t out = LEVELS[vol].weak ? pinWeak : pinA;
    if (out != pinOut) {
        // The other pin goes high impedance, so it does not short the resistor
        if (pwm.pin2channel(pinOut) >= 0) {
            pwm.removePin(pinOut);
        }
        pinMode(pinOut, INPUT);
        pwm.addPin(out);
        pinOut = out;
    }
    tone(0);
}

void Audio::tone(uint32_t freqHz) {
    if (freqHz == 0) {
        // Both pins low: no voltage across the piezo
        pwm.writePin(pinOut, 0);
        pwm.writePin(pinB, 0);
        return;
    }
    uint16_t top = (uint16_t)min<uint32_t>(PWM_CLOCK_HZ / freqHz, 0x7FFF);
    pwm.setMaxValue(top);
    pwm.writePin(pinOut, top / 2);
    if (LEVELS[vol].bridge) {
        // Opposite polarity: pinB is the complement of pinOut
        pwm.writePin(pinB, top / 2, true);
    } else {
        pwm.writePin(pinB, 0);
    }
}

void Audio::play(const Note *m) {
    melody = m;
    noteCountdown = 0;
}

void Audio::tick(int speedIndex) {
    if (melody) {
        if (noteCountdown-- != 0) {
            return;
        }
        if (melody->samples != 0) {
            tone(melody->freqHz);
            noteCountdown = melody->samples - 1;
            melody++;
            return;
        }
        // End of the melody: back to the vario beeps, starting with a beep
        melody = nullptr;
        countdown = 0;
        beeping = false;
    }

    if (countdown-- != 0) {
        return;
    }
    uint32_t period, freq;
    if (speedIndex < 0) {
        period = BEEP_DESCENT_PERIOD;
        freq = 0;
    } else {
        int i = min(speedIndex, Vario::SPEED_INDEX_MAX);
        period = BEEP_LEN[i];
        freq = AUDIO_FREQ[i];
    }
    // v1 timing, in samples (integer division as in v1)
    if (beeping) {
        tone(0);
        countdown = period * Vario::SAMPLE_RATE / 15000;
        beeping = false;
    } else {
        tone(freq * 2 / 3);
        countdown = period * Vario::SAMPLE_RATE / 10000;
        beeping = true;
    }
}
