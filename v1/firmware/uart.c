#include "pic24_all.h"

void uart_Config(void) {
    char tmp;
    U2BRG  = 6;     // 8928baud @ FCY = 250kHz, note that you must select "custom baud rate" in the
                   // pickit2 uart application drop-down window for baudrate, and enter the value 8928 
    U2MODEbits.BRGH = 1;
    U2MODEbits.UARTEN = 1;
    U2MODEbits.RTSMD = 1; // no flow control
    U2MODEbits.PDSEL = 0; // 8 bits, no parity
    U2MODEbits.STSEL = 0; // 1 stop bit
    while(U2STAbits.URXDA == 1) {
        tmp = U2RXREG;
    }
    // clear IF flags
    IFS1bits.U2RXIF = 0;
    IFS1bits.U2TXIF = 0;
    // set priority
    IPC7bits.U2RXIP = 1;
    IPC7bits.U2TXIP = 1;
    // disable interrupt
    IEC1bits.U2RXIE = 0;
    IEC1bits.U2TXIE = 0;
    // enable transmit
    U2STAbits.UTXEN = 1;
}

void uart_Print(char *szBuf) {
    while(*szBuf) {
        U2TXREG = *szBuf++;   // transfer data byte to TX reg
        while(!U2STAbits.TRMT);  // wait for xmit to complete
    }
}
