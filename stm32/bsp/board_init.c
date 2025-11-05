// stm32/bsp/board_init.c
#include <stdint.h>
#include "board_pins.h"

// --- Minimal register views (enough for Phase-0) ---
typedef struct { volatile uint32_t MODER, OTYPER, OSPEEDR, PUPDR, IDR, ODR, BSRR;
                 volatile uint32_t LCKR, AFRL, AFRH, BRR; } gpio_t;
typedef struct { volatile uint32_t CR, CFGR, D1CFGR, D2CFGR, D3CFGR, CKPR, CSR, RESERVED0[1];
                 volatile uint32_t AHB3ENR, AHB1ENR, AHB2ENR, AHB4ENR, APB3ENR, APB1LENR, APB1HENR, APB2ENR, APB4ENR;
                 /* lots omitted */ } rcc_t;
typedef struct { volatile uint32_t CR1, CR2, CR3, BRR, GTPR, RTOR, RQR, ISR, ICR, RDR, TDR;
                 /* omitted */ } usart_t;

#define GPIO(port_base)   ((gpio_t *)(uintptr_t)(port_base))
#define RCC               ((rcc_t *)(uintptr_t)RCC_BASE)
#define USART(inst_base)  ((usart_t *)(uintptr_t)(inst_base))

static void gpio_enable_clock(uint32_t gpio_base) {
    // AHB4ENR bit index: A=0, B=1, C=2, ... map from base address (quick helper)
    // On H7, GPIOA base = 0x48000000, GPIOB = 0x48000400, ... step 0x400 per port.
    uint32_t idx = (gpio_base - 0x48000000UL) / 0x400UL;  // verify against RM
    RCC->AHB4ENR |= (1u << idx);
    (void)RCC->AHB4ENR; // read-back to avoid write buffering issues
}

static void gpio_config_output(uint32_t gpio_base, uint32_t pin) {
    gpio_t *g = GPIO(gpio_base);
    // MODER: 00=input, 01=output, 10=AF, 11=analog
    g->MODER &= ~(3u << (pin * 2));
    g->MODER |=  (1u << (pin * 2));            // output
    // push-pull, low speed, no pull
    g->OTYPER  &= ~(1u << pin);
    g->OSPEEDR &= ~(3u << (pin * 2));
    g->PUPDR   &= ~(3u << (pin * 2));
}

static void gpio_config_af(uint32_t gpio_base, uint32_t pin, uint32_t af_num) {
    gpio_t *g = GPIO(gpio_base);
    // Alternate function mode
    g->MODER &= ~(3u << (pin * 2));
    g->MODER |=  (2u << (pin * 2));
    if (pin < 8) {
        g->AFRL &= ~(0xFu << (pin * 4));
        g->AFRL |=  ((af_num & 0xFu) << (pin * 4));
    } else {
        uint32_t p = pin - 8;
        g->AFRH &= ~(0xFu << (p * 4));
        g->AFRH |=  ((af_num & 0xFu) << (p * 4));
    }
}

static void usart_enable_clock(usart_t *u) {
    // Minimal mapping: decide which APB enables this USART and set its bit.
    // TODO: set the correct APB ENR bit for your chosen USARTx.
    // Example for USART1 on APB2:
    RCC->APB2ENR |= (1u << 4); // USART1EN is bit 4 on many H7 revisions — VERIFY in RM!
    (void)RCC->APB2ENR;
}

static void usart_basic_init(usart_t *u, uint32_t baud, uint32_t pclk_hz) {
    // Disable before config
    u->CR1 = 0;
    // 8N1, oversampling by 16 default. BRR = pclk / baud.
    u->BRR = (pclk_hz + (baud/2u)) / baud;
    u->CR1 = (1u<<3) | (1u<<2) | (1u<<0); // TE | RE | UE
}

void board_init(void) {
    // 1) Enable LED port clock + configure output
    gpio_enable_clock(LED_GPIO_PORT_BASE);
    gpio_config_output(LED_GPIO_PORT_BASE, LED_PIN);

    // 2) Enable UART port clocks + set alternate functions (AF number depends on pin mux)
    gpio_enable_clock(UART_TX_GPIO_BASE);
    gpio_enable_clock(UART_RX_GPIO_BASE);

    // TODO: replace '7' with actual AF for your chosen pins (see datasheet AF table).
    gpio_config_af(UART_TX_GPIO_BASE, UART_TX_PIN, 7u);
    gpio_config_af(UART_RX_GPIO_BASE, UART_RX_PIN, 7u);

    // 3) Enable USART clock + basic config
    usart_t *U = USART(UART_INST_BASE);
    usart_enable_clock(U);

    // TODO: set your APB clock here if using default reset clocks; 64 MHz placeholder.
    usart_basic_init(U, /*baud*/115200u, /*pclk_hz*/64000000u);
}
