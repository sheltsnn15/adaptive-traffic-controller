#pragma once
#include <stdint.h>

#define BSP_UART_PARITY_NONE 0

void BSP_UART_Init(unsigned port, uint32_t baud, unsigned databits, unsigned parity,
                   unsigned stopbits);

void BSP_UART_Write1(unsigned port, uint8_t byte);

typedef void (*BSP_UART_RXCB)(unsigned port, unsigned char c);
typedef int (*BSP_UART_TXCB)(unsigned port);

void BSP_UART_SetReadCallback(unsigned port, BSP_UART_RXCB cb);
void BSP_UART_SetWriteCallback(unsigned port, BSP_UART_TXCB cb);
