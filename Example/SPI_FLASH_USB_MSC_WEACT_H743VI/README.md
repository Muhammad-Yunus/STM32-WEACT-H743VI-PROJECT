# SPI Flash USB MSC – STM32H743 + W25Q64

**Project:** STM32CubeIDE — WEACT Mini STM32H743VIT6 → W25Q64 (8 MB) → USB Mass Storage Class (MSC)

---

## Hardware

| Item | Part |
|------|------|
| **MCU Board** | WEACT Mini STM32H743VIT6 (STM32H743VIT6, ARM Cortex‑M7 @ 480 MHz) |
| **External Flash** | Winbond W25Q64JV (8 Mbit / 1 MB, SPI NOR) |
| **USB** | USB OTG FS (PA11/DM, PA12/DP) — device mode |
| **Debug / Console** | UART5 (PB13 = TX, PB12 = RX) @ 115200 baud |
| **LED** | PE3 (board on‑board LED) |

### SPI Pin Mapping (PB13 / PB4 / PD7)

| Signal | Pin | MCU | Function |
|--------|-----|-----|----------|
| SPI1_SCK   | PB13 | AF5  | SPI1 Clock |
| SPI1_MISO  | PB4  | AF5  | SPI1 MISO (also NJTRST — locked by JTAG) |
| SPI1_MOSI  | PD7  | AF5  | SPI1 MOSI |
| F_CS       | PD6  | GPIO Output | Flash Chip Select (active low) |

> **Pin Notes:** PB4 is a JTAG pin (NJTRST). STM32H7 **does not support IO\_SWAP** for SPI1, so the default pin mapping is used. PB3 carries SPI1\_SCK in the STM32 default layout — on the WEACT board it is routed as `JTDO/TRACESWO`, which is why it was **locked** in CubeMX before assigning SPI1\_SCK.

---

## System Clock

| Parameter | Value |
|-----------|-------|
| HSE crystal | 8 MHz (on board) |
| PLL VCO | 480 MHz (`R` divider) |
| SYSCLK | 480 MHz |
| AHB (HCLK) | 240 MHz |
| APB2 (SPI1, USB) | 120 MHz → SPI1 clock 48 MHz |
| APB1/APB3/APB4 | 120 MHz |
| Flash Latency | 4 WS |
| USB clock | 48 MHz (from `Q` divider) |
| SPI1 Baud Rate Prescaler | `/2` → **24 Mbps** |

---

## Memory Map (Linker)

| Region | Address | Size | Use |
|--------|---------|------|-----|
| **FLASH** | `0x08000000` | 2 MB (2048 KB) | Program code + constants |
| **DTCMRAM** | `0x20000000` | 128 KB | DMA buffers, scratch |
| **RAM\_D1** (AXI SRAM) | `0x24000000` | 512 KB | `.data`, heap (512 B), stack (1 KB) |
| **RAM\_D2** (FLITF SRAM) | `0x30000000` | 288 KB | BSS |
| **RAM\_D3** (SRAM4) | `0x38000000` | 64 KB | — |
| **ITCMRAM** | `0x00000000` | 64 KB | — |

MPU: All memory disabled (`4 GB`, `NO_ACCESS`) — the board uses non‑MMU mode with MPU set to deny access by default.

---

## Project Structure

```
SPI_FLASH_USB_MSC_WEACT_H743VI/
├── Core/
│   ├── Inc/
│   │   ├── main.h                   # HAL, RCC, clock config prototypes
│   │   ├── stm32h7xx_hal_conf.h     # HAL feature toggles
│   │   └── stm32h7xx_it.h           # Exception handlers (NMI, HardFault, etc.)
│   └── Src/
│       ├── main.c                   # Entry point — init order, infinite loop
│       ├── w25qxx_demo.c            # Manual SPI read/write/erase demo (UART output)
│       ├── w25qxx_disk_test.c       # Block-level read/write/erase benchmark
│       ├── printf_uart.c            # fputc() redirect to UART5 for printf
│       ├── stm32h7xx_hal_msp.c      # HAL MSP callbacks (DMA, GPIO init hooks)
│       ├── stm32h7xx_it.c           # Interrupt service routines
│       ├── syscalls.c               # CMSIS syscalls (sbrk, _exit)
│       ├── sysmem.c                 # Heap allocation target
│       └── system_stm32h7xx.c       # SystemInit (core setup before main)
├── Drivers/
│   ├── BSP/
│   │   └── W25QXX/
│   │       ├── w25qxx.h             # W25Q flash API + JEDEC ID lookup table
│   │       ├── w25qxx.c             # Byte-level SPI driver (Read/Write/Erase/Status)
│   │       ├── w25qxx_disk.h        # Block device abstraction (512-byte sectors)
│   │       └── w25qxx_disk.c        # RMW read-modify-write, sector mapping
│   ��── STM32H7xx_HAL_Driver/        # ST official HAL library (auto-generated)
├── USB_DEVICE/
│   ├── App/
│   │   ├── usb_device.c             # USB device init + class registration
│   │   ├── usb_device.h
│   │   ├── usbd_desc.c              # USB descriptors (VID/PID/product/serial)
│   │   ├── usbd_desc.h
│   │   ├── usbd_storage_if.c        # MSC storage callback (read/write/inq)
│   │   └── usbd_storage_if.h
│   └── Target/
│       └── usbd_conf.c / usbd_usr.c # USB OTG底层驱动 & 回调
├── Middlewares/
│   └── ST/
│       └── USB_Device/              # ST USB Device Library (CDC/MSC/HID)
├── SPI_FLASH_USB_MSC_WEACT_H743VI.ioc       # CubeMX project (pin config, clocks)
├── STM32H743VITX_FLASH.ld                   # Flash linker script (2 MB FLASH + 4× SRAM)
├── STM32H743VITX_RAM.ld                     # RAM-only linker script (debug/run-from-RAM)
├── .mxproject                               # CubeMX configuration data
└── .cproject / .project                     # STM32CubeIDE project metadata
```

---

## Code Flow

### 1. Boot & Initialization (`main.c`)

```
MPU_Config()                    // Disable MPU (4 GB, NO_ACCESS)
HAL_Init()                     // SysTick + HAL core
SystemClock_Config()           // HSE → PLL → 480 MHz SYSCLK
MX_GPIO_Init()                 // LED (PE3), F_CS (PD6) — push-pull
MX_SPI1_Init()                 // SPI1 master, 24 Mbps, 8-bit, MSB first
MX_UART5_Init()                // 115200 8N1 (printf redirect)
MX_USB_DEVICE_Init()           // USB FS device → MSC class
```

### 2. W25Q Flash Driver (`Drivers/BSP/W25QXX/`)

Layer 1 — **Byte-level SPI driver** (`w25qxx.c / h`):

| Function | Description |
|----------|-------------|
| `W25Qx_Init()` | Reset flash → read parameter (ID, capacity) |
| `W25Qx_Read()` | 0x03 read (up to page-aligned boundary) |
| `W25Qx_Write()` | Writes with page-cross protection (max 256 B/page) |
| `W25Qx_Erase_Block()` | 0x20 4 KB sector erase |
| `W25Qx_Erase_Chip()` | 0xC7 bulk erase (~4 min for W25Q64) |
| `W25Qx_GetStatus()` | Poll status register (busy bit) |
| `W25Qx_WriteEnable()` | Send 0x06 WREN command |

Auto-detect JEDEC ID from table:

| JEDEC ID | Part | Capacity |
|----------|------|----------|
| `0xEF13` | W25Q80 | 1 MB |
| `0xEF14` | W25Q16 | 2 MB |
| `0xEF15` | W25Q32 | 4 MB |
| `0xEF16` | **W25Q64** | **8 MB** ← used by this project |
| `0xEF17` | W25Q128 | 16 MB |
| `0xEF18` | W25Q256 | 32 MB |

> **Note:** The W25Qxx driver is currently hardcoded for 4 MB (`W25Q32`) in `w25qxx.h`. For the W25Q64 (8 MB / `0xEF16`), update `W25QXXXX_FLASH_SIZE` to `0x800000` and adjust `SECTOR_COUNT` accordingly.

Layer 2 — **Block device abstraction** (`w25qxx_disk.c / h`):

Converts the byte-addressable flash into a **512-byte sector block device** compatible with USB MSC:

```
Logical Sector (LBA)  →  Flash Address  =  LBA × 512
Flash Address         →  Logical Sector  =  Address / 512
```

Key behavior:
- **ReadSectors():** Direct SPI read from flash address
- **WriteSectors():** Full Read-Modify-Write (RMW) pipeline:
  1. Read entire 4 KB erase block into RAM scratch buffer
  2. `memcmp()` — skip write if data is identical (optimization)
  3. Overwrite modified 512-byte sector(s) in scratch buffer
  4. Erase 4 KB flash block
  5. Write back entire 4 KB block
  6. Optionally verify write-back (via `W25QXX_DISK_VERIFY_WRITE`)
- **EraseBlock():** Direct 4 KB sector erase (convenience for benchmark)

The 8 MB flash maps to **16 384 logical sectors** (512 B each = 8 MB):
```
Total Sectors:  16 384  (0x4000)
Erase Blocks:    2 048  (4 KB each)
Sectors/Block:      8  (512 × 8 = 4096)
```

### 3. USB MSC Storage Interface (`USB_DEVICE/App/usbd_storage_if.c`)

Implements the 5 mandatory MSC callbacks:

| Callback | Purpose |
|----------|---------|
| `storage_initialize()` | Called when device connects — no-op placeholder |
| `storage_retrieve_block()` | Read from flash → `W25QXX_Disk_ReadSectors()` |
| `storage_write_memory()` | Write from PC → flash via `W25QXX_Disk_WriteSectors()` |
| `storage_manage_write()` | SCSI COMMAND COMPLETE / ERROR status |
| `storage_manage_read()` | SCSI TERMINATED / IN PROGRESS status |

USB descriptor strings:
- Product: `"STM32 Mass Storage"`
- Vendor ID: `0x05AC` (Apple — STCube default)
- Product ID: `0x5702`
- Serial: Auto-generated from STM32 unique 96-bit UID

### 4. Demo Functions (`Core/Src/w25qxx_demo.c`)

Manual SPI commands sent over UART console (via `printf`):
- `printf("Read ID: 0x%04X\r\n", id);` — shows JEDEC ID
- `printf("Chip Erase\r\n");` → `W25Qx_Erase_Chip()`
- `printf("Sector Erase 0\r\n");` → `W25Qx_Erase_Block(0)`
- `printf("Write Page0\r\n");` → writes 256 bytes of pattern to page 0
- `printf("Read Page0\r\n");` → reads back and hex-prints 64 bytes

### 5. Disk Benchmark (`Core/Src/w25qxx_disk_test.c`)

Tests the block-level API:
- `W25QXX_Disk_Init()` — init flash and verify ID
- `W25QXX_Disk_GetCapacity()` — print total sectors and size
- `W25QXX_Disk_ReadSectors()` — reads LBA 0–7 (4 KB)
- `W25QXX_Disk_EraseBlock()` — erases block 0
- `W25QXX_Disk_WriteSectors()` — writes 1 sector at LBA 0
- Then reads it back and compares with `memcmp()`

### 6. Main Loop

```c
while (1) {
    LED_Blink(5);   // PE3 blink at ~1 Hz (500 ms ON, 500 ms OFF)
}
```

The demo functions (`W25Qxx_Demo()`, `W25QXX_Disk_Test()`) are **commented out** by default and can be uncommented in `main.c` `USER CODE BEGIN 2` to run a one-shot test before the USB/MSC loop starts.

---

## How It All Fits Together

```
 ┌─────────────┐    SPI1 @ 24 Mbps    ┌───────────────┐
 │ STM32H743   │ ◄──────────────────► │ W25Q64        │
 │ Cortex-M7   │                      │ 8 MB NOR Flash│
 │ 480 MHz     │                      │ PD6 = CS      │
 └──────┬──────┘                      └───────────────┘
        │
        │  
        │  
        │
 ┌──────▼──────┐
 │ USB OTG FS  │ ← PA11 (DM), PA12 (DP)
 │ Device Mode │ → MSC class → PC sees "Removable Disk"
 └─────────────┘
```

**PC side:** Plug in the USB cable → Windows/Mac/Linux enumerates a removable mass storage drive. Format it with FAT32 and the W25Q64 acts as the physical disk backing the volume.

---

## CubeMX Configuration

| Peripheral | Settings |
|------------|----------|
| **SPI1** | Master, Full-Duplex, 24 MHz baud (prescaler `/2` from 48 MHz APB2), 8-bit, CPOL=0 CPHA=0, NSS=Software |
| **UART5** | Async, 115200 8N1, No hardware flow |
| **USB_OTG_FS** | Device Only, MSC class, DMA enabled |

---

## Building

1. Open the project in **STM32CubeIDE**
2. The project should import cleanly (CubeMX version 6.16.1, FW_H7 v1.12.1)
3. Build with **Project → Build All** (or Ctrl+B)
4. Flash via ST‑LINK/V2 (on-board USB debugger) or connect an external probe
