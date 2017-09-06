#include "pic24_all.h"
#include "i2c.h"

// MCP4542 has a 7 bits resolution between 0 and Rmax (10kOhms here)
#define I2C_ID_MCP4542 ((u8)0b01011000)
#define I2C_MCP4542_WRITE_VOLATILE ((u8)0b00000000)
#define I2C_MCP4542_READ_VOLATILE  ((u8)0b00001100)
#define I2C_MCP4542_WRITE_NON_VOLATILE ((u8)0b01110000)
#define I2C_MCP4542_READ_NON_VOLATILE  ((u8)0b01111100)

#define VOL_BOOST  ((u8)0b01111111) // level 4
#define VOL_HIGH   ((u8)0b00000000) // level 3
#define VOL_MEDIUM ((u8)0b00000110) // level 2
#define VOL_LOW    ((u8)0b00101111) // level 1

u8 volState = 0;
u8 volState_previous = 0;
/// OCM 1 config
void audio_Config(void) {
    OC1CONbits.OCTSEL = 0;  // use TMR2
    OC1CONbits.OCSIDL = 0;  // Continue in Idle Mode    
    CM1CONbits.COE = 0;     // output goes not (yet) to pin 17
    CM1CONbits.CPOL = 1;    // inverted
    CM1CONbits.CLPWR = 0;   // no operation in low power mode
    CM1CONbits.EVPOL = 0;   // no interrupt/event
    CM1CONbits.CREF = 0;    // + input connected to C1INA (pin 8)
    CM1CONbits.CCH = 3;     // - input connected to VBG/2
    CM1CONbits.CEN = 0;     // Disable

    Nop();Nop();Nop();Nop();Nop();
    _C1EVT = 0; //clear C1 event flag
    _CMIF = 0;  //clear interrupt flag
}

void comparatorStop() {
    CM1CONbits.CEN = 0;     // Disable
    CM1CONbits.COE = 0;     // output goes not to pin 17
    _RB14 = 0;
}

void comparatorStart() {
    CM1CONbits.COE = 1;     // output goes to pin 17
    CM1CONbits.CEN = 1;     // Enable
}

void audio_Beep(u32 freq){
    u16 tmrTicks;
    OC1CONbits.OCM = 0; // disable ocm
    tmr2_Stop();

    if (freq) {
        tmrTicks = (u16)((FCY>>3)/freq);
        
        // square wave:
//        OC1R = (tmrTicks >> 1);
        OC1RS = (tmrTicks >> 1);

        tmr2_Start(tmrTicks);

        OC1CONbits.OCM = 6; // PWM Mode

    } // else : the silencious beep
}

void setBoostVolume(void) {
    i2c_Write2(I2C_ID_MCP4542,I2C_MCP4542_WRITE_VOLATILE,VOL_BOOST);
    i2c_Write2(I2C_ID_MCP4542,I2C_MCP4542_WRITE_NON_VOLATILE,(u8)4);
    CONFIG_RB14_AS_DIG_OUTPUT();    
    comparatorStart();
}
void setHighVolume(void) {
    comparatorStop();
    CONFIG_RB14_AS_DIG_INPUT();
    i2c_Write2(I2C_ID_MCP4542,I2C_MCP4542_WRITE_VOLATILE,VOL_HIGH);
    i2c_Write2(I2C_ID_MCP4542,I2C_MCP4542_WRITE_NON_VOLATILE,(u8)3);
}
void setMediumVolume(void) {
    comparatorStop();
    CONFIG_RB14_AS_DIG_INPUT();
    i2c_Write2(I2C_ID_MCP4542,I2C_MCP4542_WRITE_VOLATILE,VOL_MEDIUM);
    i2c_Write2(I2C_ID_MCP4542,I2C_MCP4542_WRITE_NON_VOLATILE,(u8)2);
}
void setLowVolume(void) {
    comparatorStop();
    CONFIG_RB14_AS_DIG_INPUT();
    i2c_Write2(I2C_ID_MCP4542,I2C_MCP4542_WRITE_VOLATILE,VOL_LOW);
    i2c_Write2(I2C_ID_MCP4542,I2C_MCP4542_WRITE_NON_VOLATILE,(u8)1);
}

u8 getVolume(void) {
    u8 temp[2];
    i2c_RcvBuf(I2C_ID_MCP4542, I2C_MCP4542_READ_NON_VOLATILE, 2, temp);
    return (u8)temp[1];
}

void setNextVolume(void) {
//    if(volState == 4) {
//     setLowVolume();
//        volState = 1;
//    }
    if(volState == 4) {
        setMediumVolume();
        volState = 2;
    }
    else if(volState == 2) {
        setHighVolume();
        volState = 3;
    }
    else if(volState == 3) {
            setBoostVolume();
            volState = 4;
        }
    else {
        setBoostVolume();
        volState = 4;
    }
    //setLowVolume();
}

void setPreviousVolume(void) {
    volState = getVolume() - 1;
    if(volState == 1) {volState = 4;}
    setNextVolume();
}
