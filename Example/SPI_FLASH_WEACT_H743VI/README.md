# SPI Flash W25Qxx Driver Demo on STM32H743VIT6

STM32CubeIDE project demonstrating W25Qxx SPI NOR Flash driver on **WeAct Mini STM32H743VIT6** board.

## Hardware

| Component | Details |
|---|---|
| **MCU Board** | WeAct Mini STM32H743VIT6 (Cortex-M7, 2.0-2.8V) |
| **Flash Chip** | W25Q64 (8MB, 0xEF16) — header constants adjustable for W25Q80/16/32/64/128 |
| **Interface** | SPI1 (master, 8-bit, Mode 0, prescaler ÷16) |
| **SPI1 Pins** | PB3 = SCK, PB4 = MISO, PD7 = MOSI |
| **CS Pin** | PD6 (GPIO Output) |
| **UART** | UART5, PB13 = TX, PB12 = RX, 115200-8-N-1 |
| **LED** | PE3 |

## Clock Configuration

| Parameter | Value |
|---|---|
| Oscillator | HSE |
| PLL N | 192 → SYSCLK 480 MHz |
| PLL P | 2 → USB/DFSDM clock |
| PLL Q | 20 → OTG FS clock |
| PLL R | 2 |
| AHB Prescaler | ÷2 (HCLK 240 MHz) |
| Flash Latency | 4 WS |

## Project Structure

```
SPI_FLASH_WEACT_H743VI/
├── Core/
│   ├── Inc/
│   │   ├── main.h          # Pin defs: LED (PE3), CS (PD6)
│   │   └── stm32h7xx_it.h
│   └── Src/
│       ├── main.c           # HAL init, SPI1/UART5 config, entry point
│       ├── w25qxx_demo.c    # Demo/test application
│       └── printf_uart.c    # UART-backed printf redirect
├── Drivers/
│   ├── BSP/W25QXX/
│   │   ├── w25qxx.c         # W25Qxx driver (read/write/erase/id/reset)
│   │   └── w25qxx.h         # Driver API + flash parameter structs
│   └── CMSIS/               # ST STM32H7 device library
└── *.ld                     # Flash/RAM linker scripts
```

## Key APIs

| Function | Description |
|---|---|
| `W25Qx_Init()` | Reset flash, detect part, fill parameter struct |
| `W25Qx_Read_ID(uint16_t*)` | Read manufacturer/device ID |
| `W25Qx_Write()` | Page-program (≤256 bytes per call, auto page-boundary split) |
| `W25Qx_Read()` | Read bytes at any address (3-byte addressing) |
| `W25Qx_Erase_Block(uint32_t)` | Sector erase (64 KB) |
| `W25Qx_Erase_Chip()` | Full chip erase |
| `W25Qx_WriteEnable()` | Send WREN command, poll until ready |

## Running the Demo

Compile, flash via ST-LINK, and open a serial terminal on UART5 (115200 baud). The demo runs once in `main()` and prints:

```
=====================================
      W25Qxx Driver Demonstration
=====================================
Init OK
Flash ID          : 0xEF16

Flash Parameters
-----------------------------
Size             : 8388608 Bytes
Sector Count     : 128
Sector Size      : 65536
SubSector Count  : 2048
SubSector Size   : 4096
Page Size        : 256

Write Buffer : Hello SPI Flash from STM32H743!

Erasing Sector 0 ... OK
Writing Data ..... OK
Reading Data ..... OK

Read Buffer : Hello SPI Flash from STM32H743!

Verification : PASS

Page Write Demo
256-byte Page Test : PASS

Write Enable : OK

Demo Finished
=====================================
```

## Test Coverage

- **Init & ID detection** — hardware connection verified
- **Sector erase** — 64 KB sector at address 0
- **Byte-level write/read** — text payload with verification loop
- **Page-boundary write** — 256-byte page program at offset 4096, full round-trip compare
- **Write Enable** — standalone WREN command with busy polling

## Adapting to Other W25Qxx Parts

Edit `Drivers/BSP/W25QXX/w25qxx.h` constants to match your chip:

```
W25Q80  = 0x13  (1 MB)
W25Q16  = 0x14  (2 MB)
W25Q32  = 0x15  (4 MB)
W25Q64  = 0x16  (8 MB) — default
W25Q128 = 0x17  (16 MB)
```

The driver auto-detects the part from the device ID and computes size/sector counts dynamically.

## License

STMicroelectronics — as-is, per LICENSE file in `Core/Inc/`.
