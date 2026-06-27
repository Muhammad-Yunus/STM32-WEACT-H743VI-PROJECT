# UART_WEACT_H743VI

Simple UART echo demo for the **WeAct Studio MiniSTM32H743VIT6** board. The
firmware receives characters over **UART5**, locally echoes each typed character
back to the terminal, and on every newline prints the accumulated line prefixed
with `Echo:`.

The project was generated with **STM32CubeIDE 1.18.1** and **STM32CubeMX
6.16.1** (firmware package `STM32Cube FW_H7 V1.12.1`).

## Hardware

- Board: WeAct Studio MiniSTM32H743VIT6 (LQFP100, Cortex-M7 @ up to 480 MHz)
- MCU: STM32H743VITx
- External crystal: HSE on PH0 (OSC_IN) / PH1 (OSC_OUT)
- USB-UART bridge on the WeAct board is wired to **UART5**, not USART1.

| Signal | Pin  | UART peripheral |
| ------ | ---- | --------------- |
| TX     | PB13 | UART5_TX        |
| RX     | PB12 | UART5_RX        |

> The project name says "UART1" because the WeAct H743VIT6's on-board
> USB-to-serial converter is labeled `UART1` on the silkscreen, but the MCU
> peripheral it is actually wired to is `UART5`. The firmware uses `UART5`.

## UART settings

| Parameter      | Value     |
| -------------- | --------- |
| Baud rate      | 115200    |
| Word length    | 8 bits    |
| Parity         | None      |
| Stop bits      | 1         |
| Flow control   | None      |
| Mode           | TX + RX   |
| Oversampling   | 16        |

## Clock configuration

HSE (external crystal) feeds PLL1 to produce the system clock:

- PLL source: HSE
- PLLM = 5, PLLN = 192, PLLP = 2, PLLQ = 2, PLLR = 2
- AHB / APB prescalers all divide by 2 except SYSCLK (`DIV1`)
- Voltage scaling: `PWR_REGULATOR_VOLTAGE_SCALE0`, supply: `PWR_LDO_SUPPLY`
- Flash latency: 4

## What the firmware does

`Core/Src/main.c` runs a polling loop on UART5:

1. Calls `HAL_UART_Receive(&huart5, &rx, 1, 10)` to wait for a single byte
   with a 10 ms timeout.
2. If the byte is `\r` or `\n`, terminates the current line buffer, prints
   `Echo: <line>` followed by a new prompt `> `, and resets the buffer index.
3. Otherwise, appends the byte to `linebuf` (up to 127 chars) and prints the
   character back so the terminal shows it as it is typed (local echo).

`Core/Src/printf_uart.c` implements `__io_putchar`, redirecting all `printf`
output to UART5 via `HAL_UART_Transmit`.

A startup banner is printed once after `MX_UART5_Init`:

```
================================
UART5 Echo Test Ready
================================
```

## Building and flashing

1. Open the project in **STM32CubeIDE 1.18.1** (`File → Open Projects from
   File System…` and select this folder).
2. Build with `Project → Build Project` (Ctrl+B).
3. Flash the produced `.elf` / `.bin` from
   `Debug/UART_WEACT_H743VI.elf` using your usual tool (STM32CubeProgrammer,
   OpenOCD, ST-Link, etc.).

If you regenerate code from `UART_WEACT_H743VI.ioc`, keep the `UART5` IP and
the `PB12` / `PB13` asynchronous pin assignments; this project does not use
DMA or interrupts for UART5, so no NVIC configuration is needed.

## Trying it from the PC

1. Connect the WeAct board's on-board USB-C (USB-UART) to your PC.
2. Identify the serial port (Windows: `Device Manager → Ports (COM & LPT)`).
3. Open it with any serial client: PuTTY, Tera Term, minicom, `screen`,
   Arduino IDE Serial Monitor, etc.
4. Configure **115200 baud, 8N1, no flow control**.
5. Type a line and press Enter. You should see the characters appear as you
   type (local echo), and after Enter the line is repeated back as:

   ```
   Echo: hello world
   >
   ```

### Quick test from a terminal (Linux/macOS)

```bash
# find the device, e.g. /dev/ttyACM0 or /dev/ttyUSB0
ls /dev/ttyACM* /dev/ttyUSB* 2>/dev/null

# with minicom
minicom -D /dev/ttyACM0 -b 115200

# or with screen
screen /dev/ttyACM0 115200
```

### Quick test from PowerShell on Windows

```powershell
# Using PowerShell + a serial client (PuTTY example)
putty.exe -serial COM7 -sercfg 115200,8,n,1,N
```

## Project layout

```
.
├── Core/
│   ├── Inc/                # main.h, stm32h7xx_it.h, stm32h7xx_hal_conf.h
│   ├── Src/
│   │   ├── main.c          # UART5 echo loop
│   │   ├── printf_uart.c   # __io_putchar → UART5
│   │   ├── stm32h7xx_hal_msp.c
│   │   ├── stm32h7xx_it.c
│   │   └── system_stm32h7xx.c
│   └── Startup/            # startup_stm32h743vitx.s
├── Drivers/
│   ├── CMSIS/              # CMSIS core + device headers
│   └── STM32H7xx_HAL_Driver/
├── STM32H743VITX_FLASH.ld  # Flash linker script
├── STM32H743VITX_RAM.ld    # RAM linker script
└── UART_WEACT_H743VI.ioc   # STM32CubeMX configuration
```

## Notes

- `HeapSize` is set to `0x200` and `StackSize` to `0x400` in the `.ioc`. Plenty
  for this demo, but if you grow the project you may want to raise both.
- `__io_putchar` blocks with `HAL_MAX_DELAY` on every byte, so `printf` calls
  are blocking. That is fine for an interactive terminal but will stall the
  echo loop if the terminal is not open.
- The 10 ms timeout in `HAL_UART_Receive` is just there so the loop can be
  re-entered cheaply; it does not affect throughput once bytes start arriving.