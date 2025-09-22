#include "sam.h"
#include "GPIO_DEAKIN.h"
#include "TIMER_DEAKIN.h"

// LEDs
typedef struct { char port; uint8_t pin; } LedPin;
#define NUM_LEDS 6
LedPin leds[NUM_LEDS] = { {'A',10},{'A',11},{'B',10},{'B',11},{'A',20},{'A',21} };

// Buttons
#define BUTTON1 6 // PA06
#define BUTTON2 7 // PA07

// States & modes
typedef enum { STOPPED, RUNNING, RESET } State;
typedef enum { MODE1, MODE2, MODE3 } Mode;

volatile State currentState = STOPPED;
volatile Mode currentMode = MODE1;
volatile uint8_t counter = 0;

// ----------------- LED Control -----------------
void updateLEDs(uint8_t value) {
    if(currentMode == MODE1) {
        for(int i=0;i<NUM_LEDS;i++)
            Write_GPIO(leds[i].port, leds[i].pin, value?HIGH:LOW);
    } else {
        for(int i=0;i<NUM_LEDS;i++)
            Write_GPIO(leds[i].port, leds[i].pin, (value>>i)&0x01);
    }
}

void led_init(void) {
    for(int i=0;i<NUM_LEDS;i++){
        Config_GPIO(leds[i].port, leds[i].pin, OUTPUT);
        Write_GPIO(leds[i].port, leds[i].pin, LOW);
    }
}

// ----------------- Button Init -----------------
void button_init(void){
    // Set inputs with pull-downs
    PORT->Group[0].DIRCLR.reg = (1<<BUTTON1)|(1<<BUTTON2);
    PORT->Group[0].PINCFG[BUTTON1].bit.INEN = 1;
    PORT->Group[0].PINCFG[BUTTON1].bit.PULLEN = 1;
    PORT->Group[0].PINCFG[BUTTON2].bit.INEN = 1;
    PORT->Group[0].PINCFG[BUTTON2].bit.PULLEN = 1;
    PORT->Group[0].OUTCLR.reg = (1<<BUTTON1)|(1<<BUTTON2); // pull-down

    // EIC mapping
    PORT->Group[0].PINCFG[BUTTON1].bit.PMUXEN = 1;
    PORT->Group[0].PINCFG[BUTTON2].bit.PMUXEN = 1;
    PORT->Group[0].PMUX[BUTTON1>>1].reg = (MUX_PA06A_EIC_EXTINT6 << 0) | (MUX_PA07A_EIC_EXTINT7 << 4);

    PM->APBAMASK.reg |= PM_APBAMASK_EIC;
    GCLK->CLKCTRL.reg = GCLK_CLKCTRL_ID_EIC | GCLK_CLKCTRL_GEN_GCLK0 | GCLK_CLKCTRL_CLKEN;
    while(GCLK->STATUS.bit.SYNCBUSY);

    // Configure EIC for rising edge (pull-down: trigger when pressed)
    EIC->CONFIG[0].reg &= ~(EIC_CONFIG_SENSE6_Msk | EIC_CONFIG_SENSE7_Msk);
    EIC->CONFIG[0].reg |= (EIC_CONFIG_SENSE6_RISE << 0) |
                           (EIC_CONFIG_SENSE7_RISE << 4);

    EIC->INTENSET.reg = (1<<6)|(1<<7);
    EIC->CTRL.bit.ENABLE = 1;
    while(EIC->STATUS.bit.SYNCBUSY);

    NVIC_EnableIRQ(EIC_IRQn);
}

// ----------------- Button ISR -----------------
extern "C" void EIC_Handler(void){
    if((PORT->Group[0].IN.reg & (1<<BUTTON1))!=0){ // rising edge, pressed
        if(currentState==STOPPED) currentState=RUNNING;
        else if(currentState==RUNNING) currentState=STOPPED;
        else if(currentState==RESET){ counter=0; updateLEDs(counter); currentState=STOPPED; }
    }
    if((PORT->Group[0].IN.reg & (1<<BUTTON2))!=0){ // rising edge, pressed
        currentMode=(Mode)((currentMode+1)%3);
        counter=(currentMode==MODE3)?63:0;
    }
    EIC->INTFLAG.reg |= (1<<6)|(1<<7); // this could avoid you accidentally clearing other flags.
}

// ----------------- Main -----------------
int main(void){
    SystemInit();
    led_init();
    button_init();
    updateLEDs(0);

    TCC0_init(200); // 200ms Knight Rider
    TC3_init(500);  // Mode2/3 counting

    __enable_irq();

    uint8_t knightPos = 0;
    int8_t dir = 1;

    while(1){
        if(currentState==RUNNING){
            if(currentMode==MODE1 && blinkFlag){
                blinkFlag = false;
                knightPos += dir;
                if(knightPos == NUM_LEDS-1 || knightPos==0) dir = -dir;
                updateLEDs(knightPos);
            } else if((currentMode==MODE2 || currentMode==MODE3) && countFlag){
                countFlag = false;
                if(currentMode==MODE2){
                    counter++;
                    if(counter>63) counter=0;
                } else {
                    if(counter>0) counter--;
                    else counter=63;
                }
                updateLEDs(counter);
            }
        }
    }
}
