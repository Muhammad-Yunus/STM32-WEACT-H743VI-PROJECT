# STM32H7 GPIO EXTI Button & LED Blink Demo

## Overview

This project demonstrates basic GPIO output control and external interrupt (EXTI) handling on an STM32H7 microcontroller using STM32 HAL.

The application blinks an LED with a configurable period. Each button press generates an EXTI interrupt that changes the blink rate.

## Features

- GPIO output (LED)
- GPIO external interrupt (EXTI)
- Interrupt-driven button handling
- Adjustable LED blink period
- STM32 HAL based implementation

## Hardware Configuration

### LED

| Signal | Pin |
|---------|-----|
| LED     | PE3 |

### Button

| Signal | Pin |
|---------|-----|
| BTN     | PC13 |

Button configuration:

- Interrupt Mode: Rising Edge Trigger
- Internal Pull-down Enabled

Expected hardware connection:

```text
VDD ---- Button ---- PC13
                  |
             Internal
             Pull-down
                  |
                 GND
```

## Behavior

The LED blink period starts at:

```text
1000 ms
```

Each button press decreases the blink period by:

```text
100 ms
```

Sequence:

```text
1000 ms
900 ms
800 ms
700 ms
600 ms
500 ms
400 ms
300 ms
200 ms
100 ms
```

After reaching 100 ms, the next button press resets the blink period to:

```text
1000 ms
```

## Application Flow

```text
Startup
   |
   v
HAL_Init()
   |
SystemClock_Config()
   |
MX_GPIO_Init()
   |
Enable EXTI Interrupt
   |
   v
Main Loop
   |
   +--> Toggle LED every led_period_ms
   |
   +--> Wait for Button Interrupt
```

### Button Interrupt Flow

```text
Button Press
      |
      v
EXTI15_10_IRQHandler()
      |
      v
HAL_GPIO_EXTI_IRQHandler()
      |
      v
HAL_GPIO_EXTI_Callback()
      |
      v
Update led_period_ms
```

## EXTI Configuration

### GPIO Initialization

```c
GPIO_InitStruct.Pin = BTN_Pin;
GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
GPIO_InitStruct.Pull = GPIO_PULLDOWN;
HAL_GPIO_Init(BTN_GPIO_Port, &GPIO_InitStruct);
```

### NVIC Configuration

```c
HAL_NVIC_SetPriority(BTN_EXTI_IRQn, 0, 0);
HAL_NVIC_EnableIRQ(BTN_EXTI_IRQn);
```

### Interrupt Handler

```c
void EXTI15_10_IRQHandler(void)
{
    HAL_GPIO_EXTI_IRQHandler(BTN_Pin);
}
```

### Callback Function

```c
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    if (GPIO_Pin == GPIO_PIN_13)
    {
        if (led_period_ms > 100)
        {
            led_period_ms -= 100;
        }
        else
        {
            led_period_ms = 1000;
        }
    }
}
```

## Main Loop

```c
while (1)
{
    if ((HAL_GetTick() - last_tick) >= led_period_ms)
    {
        last_tick = HAL_GetTick();
        HAL_GPIO_TogglePin(GPIOE, GPIO_PIN_3);
    }
}
```

## Development Environment

- STM32CubeMX
- STM32CubeIDE
- STM32 HAL Driver
- STM32H7 Series MCU
- ST-Link Debugger
