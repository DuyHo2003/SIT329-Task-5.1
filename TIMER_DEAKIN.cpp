#include "TIMER_DEAKIN.h"
#include "sam.h"      // Ensure SAMD21 definitions are included

volatile bool blinkFlag = false;   // Mode 1
volatile bool countFlag = false;   // Mode 2 & 3

// ----------------- TCC0 Init (Mode 1 Knight Rider) -----------------
void TCC0_init(uint32_t ms_tick) {
    // Enable TCC0 clock
    PM->APBCMASK.reg |= PM_APBCMASK_TCC0;

    // Setup GCLK2 (1 MHz from DFLL48M/48)
    GCLK->GENDIV.reg = GCLK_GENDIV_ID(2) | GCLK_GENDIV_DIV(48);
    while (GCLK->STATUS.bit.SYNCBUSY);

    GCLK->GENCTRL.reg = GCLK_GENCTRL_ID(2) |
                        GCLK_GENCTRL_SRC_DFLL48M |
                        GCLK_GENCTRL_GENEN;
    while (GCLK->STATUS.bit.SYNCBUSY);

    // Route GCLK2 to TCC0
    GCLK->CLKCTRL.reg = GCLK_CLKCTRL_ID_TCC0_TCC1 |
                        GCLK_CLKCTRL_GEN_GCLK2 |
                        GCLK_CLKCTRL_CLKEN;
    while (GCLK->STATUS.bit.SYNCBUSY);

    // Reset TCC0
    TCC0->CTRLA.reg = TCC_CTRLA_SWRST;
    while (TCC0->SYNCBUSY.bit.SWRST);

    // Prescaler ÷64
    TCC0->CTRLA.reg = TCC_CTRLA_PRESCALER(TCC_CTRLA_PRESCALER_DIV64_Val);
    while (TCC0->SYNCBUSY.bit.ENABLE);

    // Normal PWM mode
    TCC0->WAVE.reg = TCC_WAVE_WAVEGEN_NPWM;
    while (TCC0->SYNCBUSY.bit.WAVE);

    // Set period
    uint32_t counts = (ms_tick * 15625UL) / 1000; // 1 MHz / 64 = 15625 Hz
    if (counts > 0xFFFFFF) counts = 0xFFFFFF;
    TCC0->PER.reg = counts;
    while (TCC0->SYNCBUSY.bit.PER);

    // Enable overflow interrupt
    TCC0->INTFLAG.reg = TCC_INTFLAG_MASK;
    TCC0->INTENSET.reg = TCC_INTENSET_OVF;

    // NVIC setup
    NVIC_DisableIRQ(TCC0_IRQn);
    NVIC_ClearPendingIRQ(TCC0_IRQn);
    NVIC_SetPriority(TCC0_IRQn, 3);
    NVIC_EnableIRQ(TCC0_IRQn);

    // Enable TCC0
    TCC0->CTRLA.reg |= TCC_CTRLA_ENABLE;
    while (TCC0->SYNCBUSY.bit.ENABLE);
}

// ----------------- TC3 Init (Mode 2 & 3 Count) -----------------
void TC3_init(uint32_t period_ms) {
    // 1. Enable TC3 clock
    PM->APBCMASK.reg |= PM_APBCMASK_TC3;

    // 2. Route GCLK0 (48 MHz) to TC3
    GCLK->CLKCTRL.reg = GCLK_CLKCTRL_ID_TCC2_TC3 |   // ✅ Correct ID for TC3
                        GCLK_CLKCTRL_GEN_GCLK0 |
                        GCLK_CLKCTRL_CLKEN;
    while (GCLK->STATUS.bit.SYNCBUSY);

    // 3. Reset TC3
    TC3->COUNT16.CTRLA.reg = TC_CTRLA_SWRST;
    while (TC3->COUNT16.STATUS.bit.SYNCBUSY);
    while (TC3->COUNT16.CTRLA.bit.SWRST);

    // 4. Configure mode and prescaler (÷1024)
    TC3->COUNT16.CTRLA.reg = TC_CTRLA_MODE_COUNT16 |
                             TC_CTRLA_PRESCALER_DIV1024;

    // 5. Set compare value
    uint32_t ticks = (SystemCoreClock / 1024 / 1000) * period_ms;
    if (ticks > 0xFFFF) ticks = 0xFFFF;
    TC3->COUNT16.CC[0].reg = (uint16_t)ticks;
    while (TC3->COUNT16.STATUS.bit.SYNCBUSY);

    // 6. Enable compare interrupt
    TC3->COUNT16.INTFLAG.reg = TC_INTFLAG_OVF;
    TC3->COUNT16.INTENSET.reg = TC_INTENSET_OVF;

    // 7. NVIC setup
    NVIC_DisableIRQ(TC3_IRQn);
    NVIC_ClearPendingIRQ(TC3_IRQn);
    NVIC_SetPriority(TC3_IRQn, 3);
    NVIC_EnableIRQ(TC3_IRQn);

    // 8. Enable TC3
    TC3->COUNT16.CTRLA.reg |= TC_CTRLA_ENABLE;
    while (TC3->COUNT16.STATUS.bit.SYNCBUSY);
}

// ----------------- ISR Handlers -----------------
extern "C" void TCC0_Handler(void) {
    blinkFlag = true;
    TCC0->INTFLAG.reg = TCC_INTFLAG_OVF;  // Clear interrupt flag
}

extern "C" void TC3_Handler(void) {
    countFlag = true;
    TC3->COUNT16.INTFLAG.reg = TC_INTFLAG_OVF; // Clear interrupt flag
}
