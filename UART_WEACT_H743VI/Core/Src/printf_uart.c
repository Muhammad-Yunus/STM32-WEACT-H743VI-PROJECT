/*
 * printf_uart.c
 *
 *  Created on: Jun 16, 2026
 *      Author: Asus
 */

#include "main.h"
#include <stdint.h>

extern UART_HandleTypeDef huart5;


int __io_putchar(int ch)
{
    HAL_UART_Transmit(&huart5, (uint8_t *)&ch, 1, HAL_MAX_DELAY);
    return ch;
}
