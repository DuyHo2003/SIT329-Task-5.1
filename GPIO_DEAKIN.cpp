#include "GPIO_DEAKIN.h"

// ----------------- Configure GPIO Pin -----------------
void Config_GPIO(char port, uint8_t pin, PinMode mode) {
    if (port == 'A') {
        if (mode == OUTPUT) {
            PORT->Group[0].DIRSET.reg = (1u << pin);   // Set as output
            PORT->Group[0].PINCFG[pin].reg = 0;       // Clear other config
        } else { // INPUT
            PORT->Group[0].DIRCLR.reg = (1u << pin);  // Set as input
            PORT->Group[0].PINCFG[pin].bit.INEN = 1;  // Enable input buffer
            PORT->Group[0].PINCFG[pin].bit.PULLEN = 1;// Enable pull-up/down
            PORT->Group[0].OUTSET.reg = (1u << pin);  // Pull-up HIGH
        }
    } 
    else if (port == 'B') {
        if (mode == OUTPUT) {
            PORT->Group[1].DIRSET.reg = (1u << pin);
            PORT->Group[1].PINCFG[pin].reg = 0;
        } else { // INPUT
            PORT->Group[1].DIRCLR.reg = (1u << pin);
            PORT->Group[1].PINCFG[pin].bit.INEN = 1;
            PORT->Group[1].PINCFG[pin].bit.PULLEN = 1;
            PORT->Group[1].OUTSET.reg = (1u << pin);
        }
    }
}

// ----------------- Write GPIO Pin -----------------
void Write_GPIO(char port, uint8_t pin, uint8_t value) {
    if (port == 'A') {
        if (value == HIGH) PORT->Group[0].OUTSET.reg = (1u << pin);
        else               PORT->Group[0].OUTCLR.reg = (1u << pin);
    } 
    else if (port == 'B') {
        if (value == HIGH) PORT->Group[1].OUTSET.reg = (1u << pin);
        else               PORT->Group[1].OUTCLR.reg = (1u << pin);
    }
}

// ----------------- Read GPIO Pin -----------------
uint8_t Read_GPIO(char port, uint8_t pin) {
    if (port == 'A') return (PORT->Group[0].IN.reg & (1u << pin)) ? HIGH : LOW;
    else             return (PORT->Group[1].IN.reg & (1u << pin)) ? HIGH : LOW;
}
