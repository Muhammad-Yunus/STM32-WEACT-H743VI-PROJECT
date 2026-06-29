# QSPI Flash USB MSC — WeAct Mini STM32H743VIT6

Expose an external **W25Q64 (64 Mbit / 8 MB)** QuadSPI flash as a **USB Mass Storage Class** drive on a WeAct Studio STM32H743VIT6 minimal system board.

When the device is plugged into a USB host, Windows / Linux sees an unformatted 8 MB removable disk backed by the QSPI flash chip.

## Hardware

| Item | Detail |
|------|--------|
| MCU board | WeAct Studio MiniSTM32H743VIT6 (H743VI, Cortex-M7 @ 480 MHz) |
| External flash | Winbond W25Q64 — 64 Mbit, 8 MB, QuadSPI |
| USB | USB OTG Full-Speed, internal PHY (PA11/PA12) |

## Pinout

### QuadSPI (W25Q64, Bank 1, single chip)

| STM32H743 pin | Signal       | W25Q64 pin |
|---------------|--------------|------------|
| `PB2`         | QUADSPI_CLK  | CLK  (6)   |
| `PB6`         | QUADSPI_BK1_NCS | CS#  (1) |
| `PD11`        | QUADSPI_BK1_IO0 | IO0 / SI  (5) |
| `PD12`        | QUADSPI_BK1_IO1 | IO1 / SO  (2) |
| `PE2`         | QUADSPI_BK1_IO2 | IO2 / WP# (3) |
| `PD13`        | QUADSPI_BK1_IO3 | IO3 / HOLD# (7) |

All QSPI lines must be wired straight-through; no series resistors on the data lines (W25Q64 is happy with `GPIO_SPEED_FREQ_VERY_HIGH`). Keep CS# pulled-up on board or rely on the QSPI peripheral driving it actively.

### UART5 (debug printf)

| STM32H743 pin | Signal     | Notes |
|---------------|------------|-------|
| `PB12`        | UART5_RX   | Connect to TX of USB-UART adapter |
| `PB13`        | UART5_TX   | Connect to RX of USB-UART adapter |

Default baud: 115200, 8N1. Used for `printf` debug output from `W25QXX_Demo` and the QSPI driver.

### USB OTG FS

| STM32H743 pin | Signal          |
|---------------|-----------------|
| `PA11`        | USB_OTG_FS_DM   |
| `PA12`        | USB_OTG_FS_DP   |

Uses the on-chip Full-Speed PHY (no external PHY chip). VBUS sensing is supplied by the USB voltage detector; no 5V tolerance conversion required on the data lines.

### LED

| STM32H743 pin | Label |
|---------------|-------|
| `PE3`         | LED   |

Toggled by `LED_Blink()` in USER CODE 0 of `main.c`.

## Software layout

```
Drivers/
├── BSP/W25QXX/
│   ├── w25qxx_qspi.c          Low-level W25Q64 driver (SPI / QPI mode switch)
│   ├── w25qxx_qspi.h
│   ├── w25qxx_qspi_disk.c     Block-device layer (4 KB ReadModifyWrite cache)
│   └── w25qxx_qspi_disk.h
└── STM32H7xx_HAL_Driver/      Standard STM32Cube HAL

Core/Src/
├── main.c                     Entry point (USB MSC + W25Q64 demo)
├── w25qxx_qspi_demo.c         Standalone QSPI read/write/erase demo
└── ...

USB_DEVICE/
└── App/usbd_storage_if.c       STORAGE_*_FS hooks → W25QXX_QSPI_Disk_*
```

## How the layers connect

```
USB host
   │  (Bulk-Only Transport)
   ▼
USB OTG FS → usbd_storage_if.c (Cube-generated, USER CODE blocks only)
   │
   ▼
W25QXX_QSPI_Disk_*   (block-device: 512-byte sectors, capacity = 8 MB / 512)
   │
   ▼
W25qxx_*              (sector / page / block erase / read / write)
   │
   ▼
HAL_QSPI               (QSPI peripheral driver)
   │
   ▼
W25Q64 over PB2/PB6/PD11/PD12/PD13/PE2
```

## Block-device layer

`w25qxx_qspi_disk.c` exposes a 4 KB erase-aligned block-device view of the flash:

- `W25QXX_QSPI_Disk_Init()` — initialises the chip and switches to QPI mode
- `W25QXX_QSPI_Disk_ReadSectors(addr, buf, count)` — 512-byte sectors
- `W25QXX_QSPI_Disk_WriteSectors(addr, buf, count)` — Read-Modify-Write using a 4 KB scratch buffer
- `W25QXX_QSPI_Disk_GetSectorCount()` — returns `16384` (= 8 MB / 512 B)

The Write path reads the surrounding 4 KB erase block into `gEraseBuffer`, splices in the new data, erases the block, then writes it back. This is required because the W25Q64 cannot write a byte that previously held a different bit value.

## Required CubeMX settings

These are set in `QSPI_FLASH_USB_MSC_WEACT_H743VI.ioc` and must be applied via CubeMX (Project → Settings → Project Manager → General / USB_DEVICE → MSC_FS):

| Setting | Value | Reason |
|---------|-------|--------|
| `USB_DEVICE.MSC_MEDIA_PACKET-MSC_FS` | `512` | Match logical sector size so MSC transfers don't span Read-Modify-Write block boundaries |
| `ProjectManager.StackSize` | `0x2000` (8 KB) | `gEraseBuffer[4096]` + USB + QSPI stack frames exceed the 1 KB default |
| `W25QXX_Demo()` call in `main()` | comment out | Demo runs once at startup and would otherwise race with USB enumeration |

Other Cube defaults (RCC @ 480 MHz, MPU region at `0x90000000` size 8 MB for memory-mapped mode, USB_OTG_FS Full-Speed, UART5 @ 115200) are kept as generated.

## Build & flash

1. Open `QSPI_FLASH_USB_MSC_WEACT_H743VI.ioc` in STM32CubeMX, click **Generate Code** if `.ioc` settings were changed.
2. Build in STM32CubeIDE (Ctrl+B). Output: `Debug/QSPI_FLASH_USB_MSC_WEACT_H743VI.elf`.
3. Flash via ST-Link (the WeAct board exposes SWD on the onboard ST-Link).
4. Plug USB-C into a host. After a few seconds the host should mount an 8 MB removable disk.

If the host does **not** enumerate:
- Check that `MSC_MEDIA_PACKET = 512` (not 4096) and stack is 8 KB.
- Open a serial terminal on UART5 (115200) and watch for the QSPI driver initialisation banner.
- Probe with `USBlyzer` / Wireshark USBPcap to see if the device descriptor is even requested.

## Capacity & geometry

| Property | Value |
|----------|-------|
| Flash size | 8 388 608 bytes (8 MiB) |
| Logical sector size | 512 bytes |
| Logical sector count | 16384 |
| Erase block size | 4096 bytes |
| Sectors per erase block | 8 |

Reports as **8 MB** (decimal megabytes, the way USB MSC capacity is conventionally quoted) rather than 7.81 MiB.