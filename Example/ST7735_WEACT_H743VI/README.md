# ST7735 SPI TFT LCD + RTC Calendar on WeAct Mini STM32H743VIT6

This project runs on the **WeAct Mini STM32H743VIT6** board and drives a **0.96-inch SPI TFT LCD** using the **ST7735** display controller. It initializes the RTC hardware calendar and continuously renders the current date and time on the LCD screen. LCD brightness is controlled via PWM on TIM1.

**Tested hardware**: WeAct Mini STM32H743VIT6 + ST7735 0.96-inch SPI TFT LCD (HannStar panel).

---

## What It Does

In the main loop the firmware:

1. Reads the current time and date from the hardware RTC every iteration via `HAL_RTC_GetTime()` / `HAL_RTC_GetDate()`.
2. Formats the output as `HH:mm DD MMM YY` (e.g. `20:40 27 Jun 26`) with a blinking colon separator that toggles every second.
3. Draws the string on the LCD at the bottom row via `LCD_ShowString()`.
4. Blinks the on-board E3 LED to indicate the system is running.

### Boot-up LCD Test Sequence (`LCD_Test`)

On startup the firmware runs a brief diagnostic sequence:

1. **Brightness ramp** (0-100% over 1 second) with the WeAct Studio logo displayed.
2. **Brightness counter** (1000-3000ms) showing current brightness percentage.
3. **User press** the KEY button to proceed, or wait 3 seconds for auto-advance.
4. **Fading to full brightness** over 200ms, then displays "WeAct Studio", device ID (`0x450`), and LCD ID.
5. **Fades out** before clearing the screen and entering the main RTC clock loop.

Hold KEY button during boot to skip the demo sequence.

---

## Pin Map

All pins are configured via STM32CubeMX. The board uses an LQFP100 package (STM32H743VIT6).

| Signal | Pin | Port | Alternate Function | Direction / Mode | Notes |
| --- | --- | --- | --- | --- | --- |
| **LCD CS** | PE11 | GPIOE | GPIO_Output | Push-Pull, Very High Speed | SPI chip select, active low, idles HIGH |
| **LCD RS (WR/D&C)** | PE13 | GPIOE | GPIO_Output | Push-Pull, Very High Speed | Register select: low=command, high=data |
| **SPI4 SCK** | PE12 | GPIOE | AF5 (SPI4) | Push-Pull | SPI clock |
| **SPI4 MOSI** | PE14 | GPIOE | AF5 (SPI4) | Push-Pull | SPI master-out-slave-in (bidirectional simplex) |
| **TIM1 CH2N (PWM)** | PE10 | GPIOE | AF1 (TIM1_CH2N) | Push-Pull | LCD backlight PWM, complementary output |
| **E3 LED** | PE3 | GPIOE | GPIO_Output | Push-Pull, Low | On-board LED, active high |
| **KEY (User button)** | PC13 | GPIOC | GPIO_Input | Input, Pull-down | Press = HIGH, used to skip boot demo |

---

## Clock Tree

| Setting | Value |
| --- | --- |
| HSE | Crystal/Ceramic Resonator (25 MHz) |
| LSE | Crystal/Ceramic Resonator (32.768 kHz) |
| LSE Drive | High (`RCC_LSEDRIVE_HIGH`) |
| PLL (HSE) | M=5, N=192, P=2, Q=5, R=2 -> 480 MHz (VCO 960 MHz) |
| SYSCLK | 480 MHz (from PLL) |
| HCLK (AHB) | /2 -> 240 MHz |
| APB3 (D3PCLK1) | /2 -> 120 MHz |
| APB1 (D1PCLK1) | /2 -> 120 MHz |
| APB2 | /2 -> 120 MHz |
| APB4 | /2 -> 120 MHz |
| Flash Latency | 4 WS |
| Power Scale | Scale 0, LDO supply |
| FPU | Enabled (CP10/CP11 full access) |

### Peripheral Clock Sources

| Peripheral | Clock Source | Frequency |
| --- | --- | --- |
| SPI4 | D2PCLK1 | 120 MHz (prescaler /4 -> 30 Mbps) |
| TIM1 | APB2 (D1PCLK1) x2 (no prescaler) | 240 MHz |
| RTC | LSE | 32.768 kHz |

---

## STM32CubeMX Configuration

The project was generated with **STM32CubeMX 6.16.1** using **STM32Cube FW_H7 V1.12.1**.

| Setting | Value |
| --- | --- |
| MCU | STM32H743VITx |
| Package | LQFP100 |
| Compiler | GCC |
| Optimization | O6 (-O3) |
| Heap Size | 0x200 bytes |
| Stack Size | 0x400 bytes |
| Priority Group | NVIC_PRIORITYGROUP_4 |
| DMA Vectors | Force enabled |
| MCO1 | 64 MHz (HSI) |
| MCO2 | 480 MHz (SYSCLK) |

---

## Hardware Resources

### STM32H743VIT6 (Cortex-M7 @ 480 MHz)

| Resource | Usage | Status |
| --- | --- | --- |
| Cortex-M7 | Main application core | Active |
| FPU | Floating-point unit | Enabled |
| MPU | Memory Protection Unit | Active (Region 0: entire 4GB no-access, subregions 7-0 excluded) |
| Flash (IBANK1) | Code storage | Active |
| D1 AXI SRAM | Data storage | Available |
| D2 SRAM | Data storage | Available |
| D3 SRAM | Data storage | Available |
| HSE (25 MHz) | Main system clock | Active |
| LSE (32.768 kHz) | RTC clock | Active |
| CSI (4 MHz) | Internal oscillator | Available |
| HSI (64 MHz) | Internal oscillator | Available |

### Interrupts

| Interrupt | Priority | Enabled | Purpose |
| --- | --- | --- | --- |
| SysTick | 15 (highest) | Yes | HAL timebase (1 ms) |
| NMI | - | Yes | Default handler (infinite loop) |
| HardFault | - | Yes | Default handler (infinite loop) |
| MemManage | - | Yes | MPU violation handler |
| BusFault | - | Yes | Bus error handler |
| UsageFault | - | Yes | Undefined instruction handler |
| SVCall | 0 | Yes | OS syscall (freeRTOS ready) |
| PendSV | 0 | Yes | Context switch (freeRTOS ready) |
| DebugMonitor | 0 | Yes | Debug event |

No custom peripheral interrupts are used. All peripheral polling is handled in the main loop via HAL blocking calls.

---

## Display

- **Panel**: 0.96 inch ST7735, 80x160 pixels (HannStar panel)
- **Interface**: SPI4, 8-bit simplex master, 30 Mbps (prescaler /4)
- **Pixel format**: RGB565 (16 bpp)
- **Orientation**: Landscape Rotated 180 degrees
- **Font**: Fixed-width bitmap fonts (`font.h`): 12x6 (`asc2_1206`) and 16x8 (`asc2_1608`)
- **Backlight**: PWM controlled via TIM1 Channel 2N (PE10), range 0-65535
- **Supported panels**: 0.96" (TFT96, HannStar) and 1.8" (TFT18, BOE) - configurable via `#define`

### LCD Driver Architecture

The display driver follows the ST Microelectronics BSP component pattern:

- `lcd.c/h` - Board-level abstraction (pin macros, SPI wrapping, brightness control, character/string rendering)
- `st7735.c/h` - Component-level ST7735 driver (display commands, drawing primitives, orientation, function table `ST7735_LCD_Drv_t`)
- `st7735_reg.c/h` - Register-level I/O (write_reg, read_reg, send_data, recv_data) and ST7735 register addresses

Key features:
- Bus IO registration via `ST7735_RegisterBusIO()` with callback-based SPI abstraction
- Panel-specific cursor calibration offsets (HannStar: X+26/Y+1, BOE: X+24/Y+0)
- Gamma curve configuration (16-byte positive and negative lookup tables)
- Frame rate control: `Rate = fosc / (1x2+40) * (LINE+2C+2D)` = 0x01, 0x2C, 0x2D

### Supported Colors (RGB565)

WHITE, BLACK, BLUE, RED, MAGENTA, GREEN, CYAN, YELLOW, BROWN, BRRED, GRAY, DARKBLUE, LIGHTBLUE, GRAYBLUE, BRED, GRED, GBLUE

---

## RTC (Real-Time Clock)

- **Clock source**: LSE 32.768 kHz crystal (onboard PC14/PC15)
- **Predivisors**: Asynchronous /128, Synchronous /255 -> 1 Hz tick
- **Format**: Binary
- **Mode**: 24-hour
- **Default set time**: 20:40:00, Monday 27 June 2026
- **Output**: Disabled (no RTC_OUT pin remap)

---

## PWM (LCD Backlight Control)

- **Timer**: TIM1 (advanced-control 16-bit timer)
- **Channel**: Channel 2N (complementary output)
- **Pin**: PE10 (AF1 - TIM1_CH2N)
- **Prescaler**: 0 (direct clock input)
- **Period**: 65535 (16-bit)
- **PWM mode**: PWM1
- **Polarity**: High
- **Clock**: APB2 at 120 MHz (input to timer = 240 MHz due to APB prescaler <= 2)
- **Actual PWM frequency**: 240 MHz / 65536 ~= 3.66 kHz
- **Duty cycle range**: 0-65535 (mapped to 0-100% brightness in LCD API)

### PWM API

| Function | Description |
| --- | --- |
| `LCD_SetBrightness(uint32_t)` | Set duty cycle directly (0-65535 range) |
| `LCD_GetBrightness(void)` | Get current duty cycle value |
| `LCD_Light(uint32_t target, uint32_t duration)` | Smoothly transition brightness over specified milliseconds |

---

## Known Issues & Troubleshooting

### LSE Startup Failure

On the WeAct Mini STM32H743VIT6, enabling LSE in STM32CubeMX may cause `HAL_RCC_OscConfig()` to return `HAL_TIMEOUT`. This is resolved by:

1. Resetting the Backup Domain before LSE initialization
2. Setting LSE drive capability to HIGH via `__HAL_RCC_LSEDRIVE_CONFIG(RCC_LSEDRIVE_HIGH)`

Both steps are already included in `SystemClock_Config()` (`main.c` lines 165-169):

```c
HAL_PWR_EnableBkUpAccess();
__HAL_RCC_BACKUPRESET_FORCE();
HAL_Delay(10);
__HAL_RCC_BACKUPRESET_RELEASE();
__HAL_RCC_LSEDRIVE_CONFIG(RCC_LSEDRIVE_HIGH);
```

### Backup Domain Reset is Destructive

The backup reset (`__HAL_RCC_BACKUPRESET_FORCE()`) clears RTC calendar, time, alarm, and all backup registers on every boot. The RTC time is re-set to the compile-time default each time the firmware starts. For production use where RTC time should persist across resets, guard the backup reset with a first-boot detection (e.g., read a backup register flag).

---

## Project Structure

```
ST7735_WEACT_H743VI/
+-- Core/
|   +-- Inc/
|   |   +-- main.h              # Pin definitions, function prototypes
|   |   +-- stm32h7xx_hal_conf.h # HAL configuration
|   |   +-- stm32h7xx_it.h       # Interrupt handler declarations
|   +-- Src/
|   |   +-- main.c               # Application entry: clock, LCD, RTC, PWM, main loop
|   |   +-- stm32h7xx_hal_msp.c  # HAL MSP callbacks (GPIO, SPI, RTC, TIM1)
|   |   +-- stm32h7xx_it.c       # Interrupt handlers (SysTick, NMI, HardFault, etc.)
|   |   +-- system_stm32h7xx.c   # CMSIS system init, FPU setup, power supply
|   |   +-- syscalls.c           # Standard library syscalls
|   |   +-- sysmem.c             # System memory allocation
|   +-- Startup/
|       +-- startup_stm32h743vitx.s # CMSIS startup file
+-- Drivers/
|   +-- BSP/ST7735/              # Custom LCD board-support package
|   |   +-- lcd.h / lcd.c        # High-level LCD helpers (colors, draw, brightness)
|   |   +-- st7735.h / st7735.c  # STMicroelectronics ST7735 HAL driver
|   |   +-- st7735_reg.h / .c    # Panel init register sequences (0.9" HannStar)
|   |   +-- font.h               # Bitmap font tables (12x6, 16x8, 32x32 Chinese chars)
|   |   +-- logo_128_160.c       # 128x160 bitmap logo (1.8" variant)
|   |   +-- logo_160_80.c        # 160x80 bitmap logo (0.96" variant)
|   +-- CMSIS/                   # ARM CMSIS core + device headers
|   +-- STM32H7xx_HAL_Driver/    # ST HAL library
+-- ST7735_WEACT_H743VI.ioc      # STM32CubeMX project configuration
```

---

## Build & Flash

1. Open `ST7735_WEACT_H743VI.ioc` in **STM32CubeMX** to verify peripheral configuration.
2. Generate code: `Project -> Generate Code` (or `Ctrl+B` in STM32CubeIDE).
3. Open the project in **STM32CubeIDE**.
4. Build with `Ctrl+B` (optimization level: O6).
5. Flash via ST-Link/V2 or onboard debugger.
