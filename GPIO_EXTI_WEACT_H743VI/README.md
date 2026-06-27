# GPIO_EXTI_WEACT_H743VI

STM32CubeIDE project for WEACT Mini STM32H743VIT6 demonstrating GPIO output (LED blinking) controlled by an external interrupt on PC13 (button press).

## Hardware

| Component | Pin | Config |
|-----------|-----|--------|
| LED       | PE3 | Output (push-pull) |
| Button    | PC13 | Input, pull-down, rising-edge EXTI |

**Board**: WEACT Mini STM32H743VIT6 (LQFP100)
**MCU**: STM32H743VIT6
**Clock**: HSE 8 MHz -> PLL -> SYSCLK 480 MHz

## How It Works

1. **PE3 LED** blinks continuously in the main loop using a tick-based delay (`HAL_GetTick()`).
2. **PC13 Button** is configured as an EXTI line with rising-edge trigger and pull-down resistor.
3. On each button press, the `HAL_GPIO_EXTI_Callback` fires, decrementing the blink period by 100 ms.
4. When the period would drop below 100 ms, it wraps back to 1000 ms.

**Button press cycle** (period -> next):

| Press | Period (ms) | Frequency (Hz) |
|-------|-------------|----------------|
| 1     | 900         | ~1.11          |
| 2     | 800         | ~1.25          |
| 3     | 700         | ~1.43          |
| ...   | ...         | ...            |
| 9     | 100         | ~10.0          |
| 10    | 1000 (reset)| ~0.50          |

## Project Structure

```
Core/
  Inc/
    main.h          -- Pin definitions (PE3 LED, PC13 BTN)
  Src/
    main.c          -- Application: LED toggle + EXTI callback
    stm32h7xx_it.c  -- Interrupt handlers (EXTI15_10)
    system_stm32h7xx.c -- System clock init
```

## Key Implementation Details

- **Interrupt priority**: EXTI15_10 configured at priority 0 (highest) with `NVIC_PRIORITYGROUP_4` (all bits for preemptive priority, none for subpriority).
- **No debouncing**: The button input has no software or hardware debouncing. Short presses or bounces may cause multiple callbacks.
- **Non-blocking**: LED toggle uses a delta-tick comparison in the main loop, not `HAL_Delay()`, so the MCU is free to handle other tasks.

## Build

Open the project in STM32CubeIDE and build (Ctrl+B). The project uses GCC with `-O2` optimization.
