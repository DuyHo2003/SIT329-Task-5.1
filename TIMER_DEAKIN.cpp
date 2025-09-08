#include "TIMER_DEAKIN.h"
#include "sam.h"      // Ensure SAMD21 definitions are included

volatile bool blinkFlag = false;   // Mode 1
volatile bool countFlag = false;   // Mode 2 & 3

// ----------------- TCC0 Init (Mode 1 Knight Rider) -----------------

void TCC0_init(uint32_t ms_tick) {
    // -----------------------
    // 1. Enable APB clock for TCC0
    // -----------------------
    PM->APBCMASK.reg |= PM_APBCMASK_TCC0;

    // -----------------------
    // 2. Configure GCLK2 for TCC0 (safe clock source)
    //    Divide 48 MHz DFLL by 48 → 1 MHz timer clock
    // -----------------------
    GCLK->GENDIV.reg = GCLK_GENDIV_ID(2) | GCLK_GENDIV_DIV(48);
    while (GCLK->STATUS.bit.SYNCBUSY);

    GCLK->GENCTRL.reg = GCLK_GENCTRL_ID(2) |
                        GCLK_GENCTRL_SRC_DFLL48M |
                        GCLK_GENCTRL_GENEN;
    while (GCLK->STATUS.bit.SYNCBUSY);

    // Route GCLK2 to TCC0/TCC1
    GCLK->CLKCTRL.reg = GCLK_CLKCTRL_ID_TCC0_TCC1 |
                        GCLK_CLKCTRL_GEN_GCLK2 |
                        GCLK_CLKCTRL_CLKEN;
    while (GCLK->STATUS.bit.SYNCBUSY);

    // -----------------------
    // 3. Reset TCC0
    // -----------------------
    TCC0->CTRLA.reg = TCC_CTRLA_SWRST;
    while (TCC0->SYNCBUSY.bit.SWRST);

    // -----------------------
    // 4. Configure prescaler = 64 (1 MHz / 64 = 15.625 kHz)
    // -----------------------
    TCC0->CTRLA.reg = TCC_CTRLA_PRESCALER(TCC_CTRLA_PRESCALER_DIV64_Val);
    while (TCC0->SYNCBUSY.bit.ENABLE); // wait for prescaler sync

    // -----------------------
    // 5. Set waveform generation mode to Normal PWM (NPWM)
    // -----------------------
    TCC0->WAVE.reg = TCC_WAVE_WAVEGEN_NPWM;
    while (TCC0->SYNCBUSY.bit.WAVE);

    // -----------------------
    // 6. Set timer period based on ms_tick
    // -----------------------
    uint32_t counts = (ms_tick * 15625UL) / 1000; // Convert ms to counts
    if (counts > 0xFFFFFF) counts = 0xFFFFFF;     // Max 24-bit for TCC0
    TCC0->PER.reg = counts;
    while (TCC0->SYNCBUSY.bit.PER);

    // -----------------------
    // 7. Enable overflow interrupt BEFORE enabling timer
    // -----------------------
    TCC0->INTFLAG.reg = TCC_INTFLAG_MASK;   // Clear pending flags
    TCC0->INTENSET.reg = TCC_INTENSET_OVF;  // Enable overflow interrupt

    // -----------------------
    // 8. NVIC configuration for TCC0
    // -----------------------
    NVIC_DisableIRQ(TCC0_IRQn);
    NVIC_ClearPendingIRQ(TCC0_IRQn);
    NVIC_SetPriority(TCC0_IRQn, 3);
    NVIC_EnableIRQ(TCC0_IRQn);

    // -----------------------
    // 9. Enable TCC0
    // -----------------------
    TCC0->CTRLA.reg |= TCC_CTRLA_ENABLE;
    while (TCC0->SYNCBUSY.bit.ENABLE); // Wait until timer is enabled
}

// ----------------- TC5 Init (Mode 2 & 3 Count) -----------------
void TC5_init(uint32_t period_ms) {
    // Enable TC5 clock
    PM->APBCMASK.reg |= PM_APBCMASK_TC5;

    // GCLK for TC4/TC5
    GCLK->CLKCTRL.reg = GCLK_CLKCTRL_ID_TC4_TC5 | 
                        GCLK_CLKCTRL_GEN_GCLK0 | 
                        GCLK_CLKCTRL_CLKEN;
    while (GCLK->STATUS.bit.SYNCBUSY);

    // Reset TC5
    TC5->COUNT16.CTRLA.reg = TC_CTRLA_SWRST;
    while (TC5->COUNT16.STATUS.bit.SYNCBUSY);

    // Set prescaler and period
    TC5->COUNT16.CTRLA.reg = TC_CTRLA_MODE_COUNT16 | TC_CTRLA_PRESCALER_DIV1024;
    TC5->COUNT16.CC[0].reg = (SystemCoreClock / 1024 / 1000) * period_ms;
    while (TC5->COUNT16.STATUS.bit.SYNCBUSY);

    // Enable overflow interrupt
    TC5->COUNT16.INTENSET.reg = TC_INTENSET_OVF;

    // Enable TC5
    TC5->COUNT16.CTRLA.reg |= TC_CTRLA_ENABLE;
    while (TC5->COUNT16.STATUS.bit.SYNCBUSY);
}

// ----------------- ISR Handlers -----------------
extern "C" void TCC0_Handler(void) {
    blinkFlag = true;
    TCC0->INTFLAG.reg = TCC_INTFLAG_OVF;  // Clear interrupt flag
}

extern "C" void TC5_Handler(void) {
    countFlag = true;
    TC5->COUNT16.INTFLAG.reg = TC_INTFLAG_OVF; // Clear interrupt flag
}
