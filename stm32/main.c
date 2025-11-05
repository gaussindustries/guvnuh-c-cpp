// stm32/main.c
// Phase-0 bootstrap: register-level bring-up (no HAL/LL).
// Calls board_init() and then runs a tiny poll loop.
// If you don't have CMSIS headers wired yet, you can still read the flow,
// then wire <stm32h7xx.h> include paths in CMake.

#include <stdint.h>
#include "bsp/board_init.h"
#include "app/app_main.h"

static void SystemClock_Config(void);
static inline void dwt_init(void) {
    // Enable cycle counter for ad-hoc delays/timing
    *(volatile uint32_t *)0xE000EDFC |= (1u << 24);   // DEMCR.TRCENA
    *(volatile uint32_t *)0xE0001000 = 0;             // DWT.CYCCNT
    *(volatile uint32_t *)0xE0001004 |= 1u;           // DWT.CTRL.CYCCNTENA
}

int main(void) {
    // On STM32, SystemInit() runs before main() (startup file).
    SystemClock_Config();  // TODO: keep stubbed for GPIO-only bring-up
    dwt_init();

    board_init();          // RCC: GPIOxEN, pin modes, UART pins

    for (;;) {
        app_main_poll();   // Phase-0: blink + optional UART echo
    }
}

// Minimal placeholder—keep clocks default while you prove GPIO.
// Later: HSE/HSI → PLL → prescalers → switch SYSCLK, update SystemCoreClock.
static void SystemClock_Config(void) { /* TODO: fill in Phase-1/2 */ }
