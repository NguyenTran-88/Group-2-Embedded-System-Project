# 🎧 MP3/Bluetooth Playback Device  
### STM32-based Audio Player with OLED UI and Analog Audio Switching  

> Embedded Systems Course Project – Group 2  

---

##  Overview

This project implements a **dual-mode audio playback system** based on the **STM32F411CEU6 (Blackpill)** microcontroller.  
The system supports both:

- 🎵 **MP3 playback from SD card** (via DFPlayer Mini)  
- 📶 **Bluetooth audio streaming** (via MH-M28 module)

A compact **OLED interface** provides real-time feedback, while a set of tactile buttons allows intuitive user interaction.

The system integrates both digital control and analog audio switching to create a complete embedded audio device.

---

##  Key Features

- Dual audio source:
  - MP3 playback (DFPlayer Mini)
  - Bluetooth streaming (MH-M28)
- Analog audio source switching using **74HC4053**
- OLED UI (SSD1306) with low-latency updates
- Full playback control:
  - Play / Pause
  - Next / Previous track
  - Volume up / down
  - Mode switching (MP3 ↔ Bluetooth)
- STM32 firmware using HAL (STM32CubeIDE)
- Clean modular firmware architecture
- Real-time UI update with minimal redraw region

---

##  System Highlights

- **Hybrid digital + analog design**  
  Combines MCU control logic with analog audio routing

- **Efficient UI rendering**  
  Partial OLED updates reduce flicker and improve responsiveness

- **Robust multi-source audio handling**  
  Seamless switching between Bluetooth and local playback

- **Full embedded integration**  
  Power management, signal routing, user interface, and firmware tightly coupled

---

##  System Architecture

![System Block Diagram](docs/images/system_block.png)

### Main Components

| Component | Description |
|----------|------------|
| STM32F411CEU6 | Main controller (Blackpill) |
| DFPlayer Mini | MP3 decoder (SD card playback) |
| MH-M28 | Bluetooth audio module |
| LM386 | Audio amplifier |
| 74HC4053BE | Analog multiplexer (audio switching) |
| SSD1306 OLED | Display (I2C) |
| Buck Converter | 12V → 5V power conversion |

---

##  Hardware Interface

- **UART1** → DFPlayer communication  
- **I2C1** → OLED display  
- **GPIO** → Button inputs  
- **GPIO** → Audio mode switching / Bluetooth control  

---

##  Power Architecture

- Input: **12V DC** (adapter or 3x 18650 battery pack)
- Step-down via **buck converter → 5V**
- 5V rail powers:
  - DFPlayer
  - MH-M28
  - LM386
- STM32 uses onboard regulation to **3.3V**

---

## Firmware Structure

```text
firmware/
├── Core/
│   ├── Inc/
│   ├── Src/
│   └── Startup/
├── Drivers/
│   ├── CMSIS/
│   └── STM32F4xx_HAL_Driver/
├── Third_Parties/
│   ├── dfplayer/
│   └── ssd1306/
└── Debug/                 (build output - should be ignored)

##  Main Modules

| Module | Responsibility |
|--------|----------------|
| `main.c` | System initialization & entry point |
| `player_service.c` | Playback logic and control |
| `input.c` | Button handling |
| `ui.c` | OLED rendering logic |
| `ui_assets.c` | Bitmap / UI resources |
| `dfplayer` | MP3 control driver |
| `ssd1306` | OLED driver |

---

##  Build & Flash Instructions

### Requirements
- STM32CubeIDE (latest version)
- ST-LINK programmer
- Blackpill STM32F411CEU6 board

---

### Build

1. Open **STM32CubeIDE**
2. Import project:
3. Select the `firmware` folder
4. Build project

### Flash

1. Connect **ST-LINK** to the board  
2. Click **Run / Debug**  
3. Select correct target (**STM32F411CEU6**)  
4. Flash firmware  

---

##  Usage

### Startup Behavior
- System initializes
- OLED displays:
- Current track: `1 / N`
- Initial state: **Paused**

---

### Button Mapping

| Pin | Function |
|-----|----------|
| PA3 | Play / Pause |
| PA4 | Previous track |
| PA5 | Next track |
| PA8 | Volume Down |
| PA9 | Volume Up |
| PA10 | Mode Switch (MP3 ↔ Bluetooth) |

---

## Demo & Images

### System Overview
![System Overview](docs/images/system_overview.jpg)

### Hardware Prototype
![Hardware](docs/images/hardware.jpg)

### OLED Interface
![OLED UI](docs/images/oled.jpg)

---

## ✅ Validation & Performance

- ✅ MP3 playback (stable)
- ✅ Bluetooth audio streaming (stable)
- ✅ Audio switching between sources (working correctly)
- ✅ OLED update latency: negligible
- ✅ Full button control verified

### Audio Quality
- Overall: **~8/10**
- Minor noise:
- Slight background noise when idle
- Slightly higher during playback
- Does not affect user experience significantly

---

##  Known Limitations

- Minor analog noise (likely from power/audio coupling)
- No advanced audio filtering yet
- No track metadata display (only index-based)

---

##  Future Improvements

- Improve audio filtering (reduce noise)
- Add track metadata display
- Enhance UI (progress bar, icons)
- Add battery monitoring
- PCB optimization (grounding & analog layout)

---

##  Contributors

- Group 2 – Embedded Systems Project
