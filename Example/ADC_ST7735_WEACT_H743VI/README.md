# ADC Reader - STM32H743VIT6 + ST7735 LCD

Read analog voltage via ADC3 on a WEACT Mini STM32H743VIT6 board and display the raw ADC value and calculated voltage on an integrated 0.96-inch ST7735 SPI LCD (onboard, not external).

## Hardware

| Component | Part |
|-----------|------|
| MCU | WEACT Mini STM32H743VIT6 (ARM Cortex-M7, 480 MHz) |
| LCD | 0.96" TFT ST7735 (80x160, onboard, HannStar panel, SPI interface) |
| LED | PE3 (board LED) |
| Button | PC13 (KEY) |

## Peripherals Used

- **ADC3** — 16-bit resolution, single-ended, channel 0
- **SPI4** — 8-bit, master, baud prescaler /4, polarity low, phase 1 edge
- **TIM1** — PWM generation (channel 2, for LCD backlight brightness control)
- **RTC** — configured (24-hour, HSE/LSE enabled)
- **GPIO** — PE3 (LED output), PC13 (KEY input), PE11 (LCD CS), PE13 (LCD WR/RS)

## How It Works

1. **System Clock** — HSE drives PLL1 at 480 MHz (`RCC_OscInitStruct.PLL.PLLN=192`, `PLLP=2`)
2. **ADC3 Calibration** — hardware offset calibration at startup
3. **LCD Init** — `LCD_Test()` initializes the ST7735 driver, sets orientation to landscape-rotated-180, displays the WeAct Studio boot logo (160x80 BMP), waits for KEY press to dim, then clears and shows device ID
4. **Main Loop** — continuously:
   - Start ADC3 conversion
   - Poll for completion (100 ms timeout)
   - Read 16-bit raw value (0–65535)
   - Calculate voltage: `voltage = (raw * 3.3) / 65535.0`
   - Display text on LCD at position (0, 46): `ADC:xxxxx x.xxxV`
   - Blink LED PE3 with configurable delay

## Wiring

The ST7735 LCD is **onboard** on the WEACT Mini STM32H743VIT6 module — no external wiring needed.

### ADC Input Wiring (PC2 — ADC3 Channel 0)

Connect a potentiometer as a voltage divider to **PC2**:

```
Potentiometer:
     ┌─────┐
3.3V-│     │-GND
     └──┬──┘
        └───→ PC2 (ADC3 IN0, wiper)
```

- Rotate the pot fully clockwise → ~3.3V (ADC ≈ 65535)
- Rotate fully counter-clockwise → ~0V (ADC ≈ 0)
- Middle → any voltage between 0–3.3V
- **Do not exceed 3.3V on PC2** — the STM32H7 GPIOs are 5V-tolerant for input but the ADC reference is 3.3V

## Build & Flash

1. Import the project into **STM32CubeIDE**
2. Verify the `.ioc` file configuration matches your hardware (display type: `TFT96` / 0.96" 80x160)
3. Build the project (`Ctrl+B`)
4. Connect your ST-Link debugger to the WEACT board
5. Flash and run (`F11` or the debug button)

The LCD will show a boot logo, then continuously update with the ADC reading and voltage.

## Project Structure

```
ADC_ST7735_WEACT_H743VI/
├── Core/
│   ├── Inc/
│   │   ├── main.h              # Pin definitions, peripheral declarations
│   │   ├── stm32h7xx_hal_conf.h
│   │   └── stm32h7xx_it.h
│   ├── Src/
│   │   ├── main.c              # Application code (ADC read + LCD display loop)
│   │   ├── stm32h7xx_hal_msp.c
│   │   ├── stm32h7xx_it.c
│   │   ├── syscalls.c
│   │   ├── sysmem.c
│   │   └── system_stm32h7xx.c
│   └── Startup/
│       └── startup_stm32h743vitx.s
├── Drivers/
│   ├── BSP/
│   │   └── ST7735/              # ST7735 LCD driver (BSP layer)
│   │       ├── st7735.h/.c
│   │       ├── st7735_reg.h/.c
│   │       ├── lcd.h/.c         # High-level LCD API (colors, text, bitmap)
│   │       ├── font.h
│   │       └── logo_*.c
│   ├── CMSIS/
│   └── STM32H7xx_HAL_Driver/
├── ADC_ST7735_WEACT_H743VI.ioc  # STM32CubeMX project configuration
└── *.ld                         # Linker scripts (Flash/RAM)
```

## Key Code Locations

- **Main loop (ADC + LCD)** — `Core/Src/main.c:115-137`
- **ADC3 configuration** — `Core/Src/main.c:210-262` (16-bit, single-ended, channel 0)
- **SPI4 configuration** — `Core/Src/main.c:305-346` (master, /4 prescaler)
- **LCD driver** — `Drivers/BSP/ST7735/`
- **Display type selection** — `Drivers/BSP/ST7735/lcd.h:TFT96` (0.96" ST7735, 80x160, HannStar panel, landscape-rotated-180)

## Notes

- ADC3 is 16-bit resolution (max value 65535). VREF+ is connected to the MCU 3.3V VDD supply, so the full-scale input range is 0–3.3V.
- Use a voltage divider if measuring signals above 3.3V.
- The display type is configured as `TFT96` (0.96" ST7735, 80x160, HannStar panel) in `lcd.h`. Change this macro if using a different panel.
- The ST7735 is mounted onboard the WEACT Mini module — it is not externally accessible.
