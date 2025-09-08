#ifndef GPIO_DEAKIN_H
#define GPIO_DEAKIN_H

#include <sam.h>
#include <Arduino.h>   // For PinMode, HIGH, LOW

// ----------------- GPIO Interface -----------------
// Configure a pin as INPUT or OUTPUT
void Config_GPIO(char port, uint8_t pin, PinMode mode);

// Write HIGH or LOW to a pin
void Write_GPIO(char port, uint8_t pin, uint8_t value);

// Read the current state of a pin (returns HIGH or LOW)
uint8_t Read_GPIO(char port, uint8_t pin);

#endif // GPIO_DEAKIN_H
