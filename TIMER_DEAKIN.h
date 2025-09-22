#ifndef TIMER_DEAKIN_H
#define TIMER_DEAKIN_H

#include <stdbool.h>
#include "sam.h"

// Flags
extern volatile bool blinkFlag;   // Mode 1 (Knight Rider)
extern volatile bool countFlag;   // Mode 2 & 3

// Timer init functions
void TCC0_init(uint32_t period_ms);
void TC3_init(uint32_t period_ms);

#endif
