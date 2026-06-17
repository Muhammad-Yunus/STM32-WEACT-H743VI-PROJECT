# STM32 UART5 Echo Test

Simple UART echo test application for STM32 using STM32CubeIDE and HAL drivers.

The firmware receives text from a serial terminal and echoes the complete line back when the user presses Enter.

## Features

- UART5 @ 115200 baud
- `printf()` redirected to UART5 using `__io_putchar()`
- Line-based UART command input
- Echo received text after Enter (`CR` or `LF`)
- Useful for validating:
  - UART TX
  - UART RX
  - Serial terminal configuration
  - Board pin mapping

## Hardware

- STM32 MCU
- UART5 configured in STM32CubeMX
- USB-to-UART adapter or onboard ST-LINK Virtual COM Port

## UART Configuration

| Parameter | Value |
|-----------|-------|
| Baud Rate | 115200 |
| Data Bits | 8 |
| Parity | None |
| Stop Bits | 1 |
| Flow Control | None |

## Redirecting printf()

The project redirects standard output to UART5:

```c
int __io_putchar(int ch)
{
    HAL_UART_Transmit(&huart5, (uint8_t *)&ch, 1, HAL_MAX_DELAY);
    return ch;
}
```

This allows normal C `printf()` calls:

```c
printf("Hello World\r\n");
printf("Counter = %lu\r\n", counter);
```

## Example Application

The application receives characters one-by-one and stores them in a buffer.

When Enter is pressed, the buffer is terminated with a NULL character and echoed back.

Example:

```text
UART5 Line Echo Test
> hello world

Echo: hello world

> stm32

Echo: stm32
```

## Main Loop

```c
while (1)
{
    if (HAL_UART_Receive(&huart5, &rx, 1, 10) == HAL_OK)
    {
        if (rx == '\r' || rx == '\n')
        {
            linebuf[idx] = '\0';

            printf("\r\nEcho: %s\r\n", linebuf);
            printf("> ");

            idx = 0;
        }
        else
        {
            if (idx < sizeof(linebuf) - 1)
            {
                linebuf[idx++] = rx;

                printf("%c", rx);
            }
        }
    }
}
```

## Testing

1. Build and flash the firmware.
2. Open a serial terminal:
   - TeraTerm
   - PuTTY
   - RealTerm
   - MobaXterm
3. Configure:
   - 115200 baud
   - 8 data bits
   - No parity
   - 1 stop bit
4. Type text and press Enter.
5. Verify the echoed response.

## Project Structure

```text
Core/
├── Inc/
│   └── main.h
├── Src/
│   ├── main.c
│   ├── stm32h7xx_hal_msp.c
│   ├── stm32h7xx_it.c
│   └── uart_printf.c
```