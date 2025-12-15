#include "BSP_UART.h"
#include "stm32f4xx_hal.h" /* so UART_HandleTypeDef is defined */

extern UART_HandleTypeDef huart2;

static BSP_UART_RXCB _RxCB;
static BSP_UART_TXCB _TxCB;

void BSP_UART_Init(unsigned port, uint32_t baud, unsigned databits, unsigned parity,
                   unsigned stopbits)
{
    (void)port;
    (void)databits;
    (void)parity;
    (void)stopbits;

    huart2.Init.BaudRate = baud;
    HAL_UART_Init(&huart2);

    // start IRQ-driven RX
    uint8_t d;
    HAL_UART_Receive_IT(&huart2, &d, 1);
}

void BSP_UART_Write1(unsigned port, uint8_t byte)
{
    (void)port;
    HAL_UART_Transmit_IT(&huart2, &byte, 1);
}

/* HAL callbacks ----------------------------------------------------*/
/*
void HAL_UART_RxCpltCallback(UART_HandleTypeDef* huart)
{
    if ((huart == &huart2) && _RxCB)
    {
        _RxCB(0, *(huart->pRxBuffPtr - 1)); // port = 0
    }
    // restart reception
    HAL_UART_Receive_IT(&huart2, huart->pRxBuffPtr, 1);
}
*/

void HAL_UART_TxCpltCallback(UART_HandleTypeDef* huart)
{
    if ((huart == &huart2) && _TxCB)
    {
        if (_TxCB(0) == 0)
        {           // more to send?
            return; // BSP will write next byte
        }
    }
}

/* Register user callbacks -----------------------------------------*/
void BSP_UART_SetReadCallback(unsigned port, BSP_UART_RXCB cb)
{
    (void)port;
    _RxCB = cb;
}

void BSP_UART_SetWriteCallback(unsigned port, BSP_UART_TXCB cb)
{
    (void)port;
    _TxCB = cb;
}
