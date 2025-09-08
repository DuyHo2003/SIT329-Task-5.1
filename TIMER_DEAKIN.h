#ifndef TIMER_DEAKIN_H
#define TIMER_DEAKIN_H

#include <stdbool.h>  // <-- Add this
#include "sam.h"

// Flags
extern volatile bool blinkFlag; // Mode1
extern volatile bool countFlag; // Mode2 & 3

// Timer init functions
void TCC0_init(uint32_t period_ms);
void TC5_init(uint32_t period_ms);

#endif
