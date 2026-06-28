# STM32H7 - WEACT Mini H7VIT6 + W25Qxx QSPI Flash Demo

STM32CubeIDE project demonstrating Quad-SPI (QSPI) driver for W25Qxx series SPI NOR Flash on the **WEACT Mini STM32H7VIT6** board.

## Hardware

| Item | Specification |
|------|---------------|
| **MCU** | STM32H743VIT6 (LQFP100, 512KB Flash, 320KB RAM, 480MHz) |
| **Board** | WEACT Mini STM32H7VIT6 |
| **Flash** | W25Qxx series SPI NOR Flash (e.g. W25Q64 / W25Q128) |
| **QSPI Memory Mapped Base** | `0x90000000` |
| **Flash Size** | 8 MB (`1 << 23` bytes) |

### QSPI Pin Mapping

| Signal | Pin | GPIO |
|--------|-----|------|
| QSPI_BK1_IO3 | PD13 | GPIO D |
| QSPI_BK1_IO2 | PE2 | GPIO E |
| QSPI_BK1_IO1 | PD12 | GPIO D |
| QSPI_BK1_IO0 | PD11 | GPIO D |
| QSPI_CLK | PB2 | GPIO B |
| QSPI_BK1_NCS | PB6 | GPIO B |

### UART Console

| Signal | Pin | Peripheral |
|--------|-----|------------|
| TX | PC12 | UART5 |
| RX | PD2 | UART5 |

- **Baudrate**: 115200
- **Format**: 8N1

### LED

| Signal | Pin | GPIO |
|--------|-----|------|
| LED | PE3 | GPIO E |

LED blinks continuously after the demo completes.

## Project Structure

```
QSPI_FLASH_WEACT_H743VI/
├── Core/
│   ├── Inc/
│   │   ├── main.h                  # LED pin definitions
│   │   └── stm32h7xx_hal_conf.h    # HAL configuration
│   ├── Src/
│   │   ├── main.c                  # Entry point, MPU, clock, peripherals
│   │   ├── w25qxx_qspi_demo.c      # Demo test sequence
│   │   ├── printf_uart.c           # stdout redirected to UART5
│   │   ├── stm32h7xx_it.c          # Interrupt handlers
│   │   └── system_stm32h7xx.c      # System init
│   └── Startup/
│       └── startup_stm32h743vitx.s # CMSIS startup
├── Drivers/
│   ├── BSP/W25QXX/
│   │   ├── w25qxx_qspi.c           # W25Qxx BSP driver (QSPI low-level)
│   │   └── w25qxx_qspi.h           # W25Qxx driver header
│   ├── CMSIS/                      # CMSIS core + device pack
│   └── STM32H7xx_HAL_Driver/       # ST HAL library
├── STM32H743VITX_FLASH.ld          # Flash linker script (512KB)
└── STM32H743VITX_RAM.ld            # RAM linker script (320KB)
```

## Key Implementation Details

### MPU Configuration (CRITICAL)

STM32H7 enables the Memory Protection Unit (MPU) by default. The default MPU configuration **blocks** access to the QSPI memory-mapped region (`0x90000000`), causing every memory access to trigger a **MemManage Fault**.

An MPU region must be explicitly configured before QSPI memory-mapped mode can work. This is handled in `main.c::MPU_Config()`:

**Region 0 -- Default deny (4GB):**
- Base: `0x00000000`, Size: 4GB
- Access: No Access, Instruction: Disable
- Sub-region disable mask: `0x87` (allows QSPI region 0x90000000)

**Region 1 -- QSPI Flash:**
- Base: `0x90000000`, Size: 8MB
- Access: Full (Read/Write)
- Instruction Access: Enable
- Shareable: Disabled
- Cacheable: Disabled
- Bufferable: Disabled

### Clock Configuration

- **Source**: HSI (16MHz)
- **PLL**: Multiply by 60 -> SYSCLK = 300MHz
- **AHB**: /2 -> 150MHz
- **Flash Latency**: 4 WS
- **QSPI Clock**: SYSCLK / (Prescaler 1 + 1) = 150MHz (with half-cycle sample shifting)

## Demo Test Sequence

Run the project and open a serial console (115200, 8N1) to see the output:

### 1. Initialize & Read ID

Resets the flash chip, enables the Quad Enable (QE) bit, and reads the device ID.

```
Flash ID : 0xEF16
Status Register 1 : 00
Status Register 2 : 02
Status Register 3 : 60
```

### 2. Read Data

Reads the first 64 bytes from address `0x000000`.

### 3. Sector Erase

Erases a 4KB sector at address `0x000000` and verifies all bytes are `0xFF`.

### 4. Page Program (Write)

Writes a sequential pattern (`0x00` .. `0xFF` .. `0x00` .. `0x01`) of 512 bytes, then reads back and verifies with `memcmp`.

### 5. Page Boundary Cross Test

Writes 300 bytes starting at offset 200 (crossing the 256-byte page boundary) and verifies.

### 6. Block Erase

Erases a 64KB block.

### 7. QPI Mode

Enters Quick Protocol Interface (QPI) mode where all 4 I/O lines are used for instructions and addresses.

### 8. Memory-Mapped Mode

Enables memory-mapped mode so the flash is accessible via regular pointer dereference at `0x90000000`. Tests include:
- Direct pointer read
- `memcpy` from flash address
- Verify against known data pattern

### Expected Success Output

```
=====================================
      W25QXX QUADSPI DEMO
=====================================
Flash ID : 0xEF16
Status Register 1 : 00
Status Register 2 : 02
Status Register 3 : 60

Read first 64 bytes
00 01 02 03 04 05 06 07 08 09 0A 0B 0C 0D 0E 0F 
...

Erase Sector...
Erase OK
Verify Erase
FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF 
...

Write 512 bytes...
COMPARE PASS

First 64 bytes after write
00 01 02 03 04 05 06 07 08 09 0A 0B 0C 0D 0E 0F 
...

Page Boundary Test
Boundary PASS

Erase Block (64KB)...
Block Erase PASS

Enter QPI Mode...
QPI Enabled

Enable MemoryMapped Mode...
MemoryMapped Enabled

MemoryMapped Direct Read
00 01 02 03 04 05 06 07 08 09 0A 0B 0C 0D 0E 0F 
...

MemoryMapped memcpy Test
00 01 02 03 04 05 06 07 08 09 0A 0B 0C 0D 0E 0F 
...

MemoryMapped VERIFY PASS

=========== Demo Finished ===========
```

## Building

1. Open the project in **STM32CubeIDE** (workspace `workspace_1.18.1`).
2. The project was generated via **STM32CubeMX** (`QSPI_FLASH_WEACT_H743VI.ioc`).
3. Build configuration: **Debug**.
4. Linker script: `STM32H743VITX_FLASH.ld` (starts at `0x08000000`, 512KB).

## Debugging / Programming

- **Stlink**: Use the built-in Stlink debugger in STM32CubeIDE.
- **Serial Console**: Open UART5 (PC12 TX / PD2 RX) at **115200 baud** to view demo output.
- The project launch configuration is pre-saved in `QSPI_FLASH_WEACT_H743VI Debug.launch`.

## Supported Flash Chips

The W25Qxx BSP driver supports any chip in the W25Q series that follows the standard SPI/QPI command set:

- W25Q16 / W25Q32 / W25Q64 / W25Q128 / W25Q256

The QE bit is automatically enabled during initialization for quad-mode operations.

