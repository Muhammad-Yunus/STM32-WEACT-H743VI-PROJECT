# SDMMC USB MSC on WEACT Mini STM32H743VIT6

Turn the **WEACT Mini STM32H743VIT6** dev board into a USB flash drive by exposing an SD card slot as a USB Mass Storage Class (MSC) device via the USB OTG Full-Speed (USB_FS) peripheral.

## Hardware

- **Board**: WEACT Mini STM32H743VIT6 (STM32H743VIT6, LQFP100)
- **MCU**: STM32H743VIT6 - ARM Cortex-M7 @ 480 MHz
- **SD Card Interface**: SDMMC1, 4-bit wide bus
- **USB Interface**: USB_OTG_FS, device-only mode

### Pin Mapping

| Signal | Pin | Port |
|--------|-----|------|
| SD MMC CK | PC12 | SDMMC1_CK |
| SD MMC CMD | PD2 | SDMMC1_CMD |
| SD MMC D0 | PC8 | SDMMC1_D0 |
| SD MMC D1 | PC9 | SDMMC1_D1 |
| SD MMC D2 | PC10 | SDMMC1_D2 |
| SD MMC D3 | PC11 | SDMMC1_D3 |
| USB DP | PA12 | USB_OTG_FS_DP |
| USB DM | PA11 | USB_OTG_FS_DM |
| LED (status) | PE3 | GPIO_Output |
| HSE | PH0/PH1 | 8 MHz external crystal |

## Project Overview

This STM32CubeIDE project configures the STM32H743 as a USB device that presents an microSD card as a removable USB mass storage drive. When connected to a PC via USB, the board appears as a thumb drive backed by the SD card.

### Key Features

- **USB MSC device** using STM32 USB Device Library (class MSC)
- **SD card as storage medium** via HAL SD driver (4-bit wide bus)
- **Dynamic capacity reporting** - reads block count and block size from the SD card at runtime via `HAL_SD_GetCardInfo()`
- **Ready and write-protected** checks using `HAL_SD_GetCardState()`
- **Blowfish-style encryption** support in storage interface (placeholder)
- **LED heartbeat** on PE3 to indicate the firmware is running

## Architecture

```
PC (USB Host)
    |
    v
STM32H743 USB_OTG_FS (Device)
    |
    v
STM32 USB Device Library (MSC Class)
    |
    v
usbd_storage_if.c (Storage Middleware)
    |
    v
HAL SD Driver (SDMMC1, 4-bit)
    |
    v
microSD Card
```

### Startup Sequence

1. `HAL_Init()` - Initialize HAL library
2. `SystemClock_Config()` - HSE 8MHz -> PLL -> SYSCLK 480 MHz, SDMMC 48 MHz, USB 48 MHz
3. `MX_GPIO_Init()` - Configure PE3 as output (LED)
4. `MX_SDMMC1_SD_Init()` - Initialize SDMMC1 with 4-bit wide bus, clock divider = 5
5. `MX_USB_DEVICE_Init()` - Initialize USB FS device, register MSC class and storage callbacks
6. Main loop - Blink LED on PE3 to indicate system is alive

### Storage Interface

The core bridging logic is in `USB_DEVICE/App/usbd_storage_if.c`:

- **STORAGE_Init_FS()** - Returns OK immediately (SD card already initialized by HAL)
- **STORAGE_GetCapacity_FS()** - Queries `HAL_SD_GetCardInfo()` for real-time block count and size
- **STORAGE_IsReady_FS()** - Checks `HAL_SD_GetCardState()` == `HAL_SD_CARD_TRANSFER`
- **STORAGE_IsWriteProtected_FS()** - Always returns OK (no write protection)
- **STORAGE_Read_FS()** - Calls `HAL_SD_ReadBlocks()` with timeout scaled by block count
- **STORAGE_Write_FS()** - Calls `HAL_SD_WriteBlocks()` with timeout scaled by block count
- **STORAGE_GetMaxLun_FS()** - Returns 0 (single logical unit)

## Build Requirements

- **IDE**: STM32CubeIDE 1.15+
- **Toolchain**: GCC ARM (ST Microelectronics Firmware Package v1.12.1)
- **Compiler Optimization**: O6 (Level 6)

## Project Structure

```
SDMMC_USB_MSC_WEACT_H743VI/
+-- Core/
|   +-- Src/
|   |   +-- main.c              # Application entry, clock, GPIO, SDMMC init, LED blink
|   |   +-- stm32h7xx_it.c      # Interrupt handlers
|   |   +-- stm32h7xx_hal_msp.c # HAL MSP callbacks
|   +-- Inc/
|       +-- main.h              # PE3 pin definitions
+-- USB_DEVICE/
|   +-- App/
|   |   +-- usb_device.c        # USB device stack init, MSC class registration
|   |   +-- usbd_desc.c         # USB device descriptors (VID:0x0483, PID:0x5740)
|   |   +-- usbd_storage_if.c   # Storage middleware: bridges USB MSC <-> SD card
|   +-- Target/
|       +-- usbd_conf.c         # USB device configuration callbacks
+-- Middlewares/
|   +-- ST/STM32_USB_Device_Library/  # USB device library (core + MSC class)
+-- Drivers/
    +-- STM32H7xx_HAL_Driver/       # HAL drivers
    +-- CMSIS/                      # CMSIS core + device headers
```

## Hardware Setup

1. Insert a formatted microSD card into the card slot on the WEACT Mini board
2. Connect the board to your PC via the USB ST-LINK port (or user USB port if wired)
3. Program the firmware using the built-in ST-LINK/V2-1 debugger
4. The LED on PE3 should begin blinking, indicating the firmware is running
5. The PC should detect a new USB mass storage device

## USB Device Information

- **Vendor ID**: 0x0483 (STMicroelectronics)
- **Product ID**: 0x5740
- **Product String**: "STM32 Mass Storage"
- **Manufacturer String**: "STMicroelectronics"
- **Serial Number**: Derived from STM32 unique device ID

## Clock Configuration

| Clock | Frequency | Source |
|-------|-----------|--------|
| SYSCLK | 480 MHz | PLL (HSE/5 * 192) |
| HCLK (AHB) | 240 MHz | SYSCLK / 2 |
| SDMMC CLK | 48 MHz | HCLK / 5 (divider) |
| USB CLK | 48 MHz | PLL Q divider (480/10) |
| APB1/APB2 | 120 MHz | HCLK / 2 |

## Troubleshooting

- **"USB device not recognized"** - Verify SD card is properly inserted and formatted (FAT32 recommended)
- **No LED blink** - Check power supply; the board needs 5V via USB
- **SD card not detected** - Verify SD card wiring and that the card is seated properly
- **Write failures** - Ensure the SD card write-protect switch is disabled
