#include "pic24_all.h"

#include "i2c.h"
#include "timers.h"
#include "sensor.h"
#include "audio.h"
#include "filters.h"
#include "vario.h"
#include "button.h"

//#define DEBUG
#define ELLE_AIME_LA_BITE

#ifdef DEBUG
    #include <stdio.h>
    #include "uart.h"
    char dbgBuf[50];
#endif

#define NBVALUES 10
s32 values[NBVALUES];
u8 values_i = 0;

_FBS( BSS_OFF & BWRP_OFF )
_FGS( GSS0_OFF & GWRP_OFF )
_FOSCSEL( FNOSC_LPFRCDIV & IESO_OFF )
_FOSC( FCKSM_CSECMD & POSCFREQ_100KHZ & OSCIOFNC_ON & POSCMOD_NONE)
_FWDT( FWDTEN_OFF & WINDIS_OFF & FWPSA_PR128 & WDTPS_PS32768 )
_FPOR( MCLRE_ON & BORV_1_8V & AI2C1SEL_PRI & PWRTEN_OFF & BOREN_BOR3)
_FICD( BKBUG_OFF & COE_OFF & ICS_PGx3 )
_FDS( DSWDTEN_OFF & DSBOR_OFF & RTCCREF_OFF & DSWDTLPRC_OFF & DSWDTPS_DSWDTPS_0 )


volatile u8 tmr1_waiting = 0;
u8 sns_status = 0;

// Raw value of the pressure (coming straight out of the ADC)
u32 Praw = 0;
// compensated pressure value
s32 PressureComp = 0;

#define INTRO_LENGTH 7
u32 intro_freqs[INTRO_LENGTH] = {440, 0,  698, 0,  659, 0,  587};
u32 intro_times[INTRO_LENGTH] = {600, 40, 600, 40, 600, 40, 1200};

// Linear Regression
//void nextFitSpeed(s64 *speed) {
//    u16 i  = 0; // t index
//    u16 i_ = 0; // corresponding index in sample in array
//    u32 X_SUM = 465L;
//    u32 DENOM = 337L;
//    u64 XY_SUM = 0LL;
//    u64 Y_SUM = 0LL;
//    for(i=0;i<MEASURE_BUFFER_SIZE;i++) {
//        i_ = i+pressure_count;
//        if (i_ >= MEASURE_BUFFER_SIZE) {i_ = i+pressure_count - MEASURE_BUFFER_SIZE;}
//        Y_SUM = Y_SUM + filteredSamplesBuffer[i_];
//        XY_SUM = XY_SUM + ((i+1) * filteredSamplesBuffer[i_]);
//    }
//    *speed = 10 + (((s64)X_SUM * (s64)Y_SUM) - (MEASURE_BUFFER_SIZE * (s64)XY_SUM) ) / ((s64)DENOM);
//}

void startElapsedTime(void) {
    tmr1_Start(100000);
    _T1IE = 0;
}

#ifdef DEBUG
void printElapsedTime(void) {
    sprintf(dbgBuf, "Elapsed Time = %u ms\r\n", TMR1);
    tmr1_Stop();
    uart_Print(dbgBuf);
    sns_status = 0;
    sns_TriggerTemperatureSample();
    tmr1_Start(MEASURE_PERIOD_T);
}
#endif

void mcu_Init(void) {
    AD1PCFG = 0xFFFF;            // set all analog-input enabled pins (ANx) to digital mode
    CLKDIVbits.RCDIV = 0;  	 // LPFRC oscillator 500kHz divided by 1
    CONFIG_RB2_AS_DIG_INPUT();  // 6 : gpioDbg
    CONFIG_RA4_AS_DIG_INPUT();   // DEBUG
    CONFIG_RA6_AS_DIG_OUTPUT();  // 14 : Audio output
//    CONFIG_RB14_AS_DIG_OUTPUT();  // 14 : Comparator output
    CONFIG_RB14_AS_DIG_INPUT();

    _DSSR = 0;      // clear RELEASE bit
    _INT0IE = 0;    // only enabled when entering deep sleep mode
    _INT0 = 0;      // Clear Deep Sleep on INT0 edge flag

    #ifdef DEBUG
        uart_Config();
    #endif

    i2c_Config();
    tmr1_Config();
    tmr2_Config();
    tmr3_Config();

    button_Config();
    audio_Config();
}


void tmr1_callback(void) {
    tmr1_Stop();    
    if (tmr1_waiting) {
        tmr1_waiting = 0;
    } else {
        ClrWdt();
        _IDLE = 0;
        if (sns_status) {            
            ///////////////////////////////////           
            sns_ReadADC(&Praw);
            sns_TriggerTemperatureSample();            
            /////////////////////////////////// 2ms
            tmr1_Start(MEASURE_PERIOD_T);
        } else {
            u32 Traw;
            sns_ReadADC(&Traw);
            sns_TriggerPressureSample();
            tmr1_Start(MEASURE_PERIOD_P);
            sns_CompensateTemperature(Traw);
            PressureComp = sns_CompensatePressure(Praw);
            vario_ping(PressureComp);
            //sprintf(dbgBuf,"%lu \r\n",PressureComp);
            //uart_Print(dbgBuf);
        }
        sns_status = !sns_status;        
        Idle();         // Save Power
    }
}


void wait(u32 ms) {
    tmr1_Stop();
    tmr1_waiting = 1;
    tmr1_Start(10 * ms);

    while (tmr1_waiting){
        Nop();
    }
}

int main(void) {    
    mcu_Init();
    ClrWdt();  
    sns_Reset();
    wait(2000);
    audio_Beep(0);
    sns_Config();
    sns_status = 0;
    setPreviousVolume();
#ifdef ELLE_AIME_LA_BITE
    u8 intro_i;
    for (intro_i = 0; intro_i < INTRO_LENGTH; intro_i++) {
        audio_Beep(intro_freqs[intro_i]);
        wait(intro_times[intro_i]);
    }
    audio_Beep(0);
#else
    audio_Beep(220);
    wait(80);
    audio_Beep(1100);
    wait(80);
    audio_Beep(2200);
    wait(80);
    audio_Beep(0);
#endif
    sns_TriggerTemperatureSample();
    tmr1_Start(MEASURE_PERIOD_T);
    vario_init();
    button_Config();
    while(1){        
        Nop();
    }
    return 0;
}
