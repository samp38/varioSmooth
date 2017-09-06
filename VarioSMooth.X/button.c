#include "pic24_all.h"
#include "timers.h"
#include "audio.h"
#include "vario.h"
#include "uart.h"
#include <stdio.h>

#define SW1             _RB7            //switch state
#define SW1_PRESSED()   (SW1==0)        //switch test
#define SW1_RELEASED()  (SW1==1)        //switch test

#define LONG_PRESS_MIN_PERIOD 500 // ticks
#define MIN_PRESS_PERIOD 10 // minimum tick count - otherwise it's considered as a boucing button
#define PRESS_TIMEOUT 600      // TIMEOUT
#define ANTI_BOUNCE_PEROD 70    // to avoid bounces that occure when pushing button



void button_Config(void) {
    CONFIG_RB7_AS_DIG_INPUT();
    _CNIF = 0;  // clear CN Interrupt Flag
    ENABLE_RB7_CN_INTERRUPT();
    _CNIP = 1; // CN interruptions priority
    _CNIE = 1; //enable the Change Notification general interrupt
}

void wait3(u32 ticks) {
    _T3IF = 0;
    tmr3_Start();
    while(tmr3_Get() < ticks) {
        Nop();
    }
    tmr3_Stop();
}

void shutdown(void) {
    audio_Beep(2200);
    wait3(80);
//    audio_Beep(0);
//    wait3(100);
    audio_Beep(1100);
    wait3(80);
//    audio_Beep(0);
//    wait3(50);
    audio_Beep(220);
    wait3(80);
    audio_Beep(0);

    tmr1_Stop();
    tmr2_Stop();
    CONFIG_RB14_AS_DIG_INPUT();
    comparatorStop();
    _INT0EP = 1;    // set INT0 Interrupt Priority
    _INT0IE = 1;    // Enable INT0 Interrupt
    _DSEN = 1;      // Enable Deep Sleep
    Sleep();        // Go to Deep Sleep
    Nop();
}

void buttonJingle(void) {
    audio_Beep(220);
    wait3(50);
    audio_Beep(0);
    wait3(50);
    audio_Beep(220);
    wait3(50);
    audio_Beep(0);
    wait3(50);
    audio_Beep(220);
    wait3(50);
    audio_Beep(0);
}

void tmr3_callback(void) {
    _T3IF = 0;
    _T3IE = 0;
    vario_reset_shutdown_counter();
    tmr3_Stop();
    setNextVolume();
    buttonJingle();
    
}

 void __attribute__((interrupt, no_auto_psv)) _CNInterrupt(void) {
    _CNIF = 0;              // Clear Change Notification Interrupt Flag
    DISABLE_RB7_CN_INTERRUPT(); // Disable CN Interrupt for bounces

    // anti-bounce
    wait3(ANTI_BOUNCE_PEROD);
    tmr3_Start();           // Start Timer for press duration measurement 

    if (SW1_PRESSED()){
        while(SW1_PRESSED() && tmr3_Get() < PRESS_TIMEOUT) {Nop();}
        if (tmr3_Get() > LONG_PRESS_MIN_PERIOD) {
            // Long press -> Entering deep sleep mode
            shutdown();
        } else {
            // Short press -> set new Volume
            vario_reset_shutdown_counter();
            setNextVolume();
            buttonJingle();
        }
    }
    tmr3_Stop();
    ENABLE_RB7_CN_INTERRUPT();
}
 