# STM32H7 ST7735 0.96" LCD + RTC Project

Project STM32CubeIDE untuk STM32H7 yang mengintegrasikan:
- TFT LCD ST7735 0.96 inch (SPI)
- RTC (Real Time Clock)
- HAL driver STM32Cube
- Custom UI digital clock dengan blinking separator

---

## 📌 Features

- Display time (HH:MM:SS)
- Display date (DD MMM YYYY)
- Blinking colon animation on LCD
- Real-time update using RTC HAL
- Tick counter (HAL_GetTick)
- ST7735 SPI driver integration
- Optimized LCD rendering

---

## 🧰 Hardware Used

- STM32H7 Series MCU (e.g. STM32H743 / H750 / H723)
- ST7735 0.96" SPI TFT LCD
- 3.3V power supply
- Optional: User button (for UI interaction)

---

## 🔌 Connections (Typical SPI)

| ST7735 Pin | STM32 Pin |
|------------|----------|
| VCC        | 3.3V     |
| GND        | GND      |
| SCK        | SPI_SCK  |
| SDA (MOSI) | SPI_MOSI |
| RES        | GPIO     |
| DC         | GPIO     |
| CS         | GPIO     |
| BL         | 3.3V / PWM |

---