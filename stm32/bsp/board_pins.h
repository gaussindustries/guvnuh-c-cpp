#pragma once
// ===== Choose pins from the NUCLEO-H753ZI schematics / RM =====
// LED (status) — pick one user LED on your board:
#define LED_GPIO_PORT_BASE   (0x48000000UL)  // EXAMPLE: GPIOA base (AHB4) — REPLACE
#define LED_PIN              (5u)            // EXAMPLE: PA5 — REPLACE

// UART (debug) — simple TX/RX for loopback (same USART instance in board_init.c):
#define UART_INST_BASE       (0x40011000UL)  // EXAMPLE: USART1 base — REPLACE
// TX pin (AF), RX pin (AF) belong to some GPIO ports:
#define UART_TX_GPIO_BASE    (0x48000000UL)  // EXAMPLE: GPIOA base — REPLACE
#define UART_TX_PIN          (9u)            // EXAMPLE: PA9  (AF7 USART1_TX) — REPLACE
#define UART_RX_GPIO_BASE    (0x48000000UL)  // EXAMPLE: GPIOA base — REPLACE
#define UART_RX_PIN          (10u)           // EXAMPLE: PA10 (AF7 USART1_RX) — REPLACE

// RCC registers (AHB4 for GPIO, APB for USART) — H7 family offsets
#define RCC_BASE             (0x58024400UL)
