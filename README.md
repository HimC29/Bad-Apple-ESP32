<div align="center">

# 🍎 Bad Apple!! on ESP32

### A tiny device that plays the classic Bad Apple!! music video — screen and audio included — with a knob to control the volume.

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg?style=for-the-badge)](https://opensource.org/licenses/MIT)
[![Platform: ESP32](https://img.shields.io/badge/Platform-ESP32-E7352C?style=for-the-badge&logo=espressif&logoColor=white)](https://www.espressif.com/en/products/socs/esp32)
[![Arduino](https://img.shields.io/badge/Arduino-Framework-00979D?style=for-the-badge&logo=arduino&logoColor=white)](https://www.arduino.cc/)

**Plays the full Bad Apple!! music video on a 128×64 OLED display with synchronized audio, powered by an ESP32 and an SD card.**

[How It Works](#-how-it-works) • [Hardware](#-hardware) • [Wiring](#-wiring) • [Setup](#-setup) • [File Structure](#-file-structure)

</div>

---

## ✨ Features

<table>
<tr>
<td>

🎬 **Full Music Video**  
Plays the entire Bad Apple!! MV frame by frame

🔊 **Synchronized Audio**  
MP3 audio plays in sync with the video via I2S DAC

🖥️ **OLED Display**  
128×64 SSD1306 display at 20fps

</td>
<td>

🎛️ **Volume Control**  
Rotary encoder to adjust volume on the fly

💾 **SD Card Storage**  
Video frames and audio stored on a FAT32 SD card

⚡ **Dual-Core FreeRTOS**  
Audio and video run on separate cores for smooth playback

</td>
</tr>
</table>

---

## 🎬 Original Video

The music video and audio used in this project are from the iconic Bad Apple!! Touhou fan video:

> **Bad Apple!! feat.nomico** — Alstroemeria Records  
> MV uploaded by [Anira☆](https://youtu.be/FtutLA63Cp8?si=1KIgajlmQthSrCgQ)

All rights to the original music and video belong to their respective owners. This project is for personal/educational use only.

---

## 🔧 How It Works

### Video frames

The SSD1306 is a 1-bit display — every pixel is either on or off. Each frame of the video is stored as raw binary: 128×64 pixels at 1 bit per pixel = exactly **1024 bytes per frame**. All frames are packed back-to-back into a single `bad_apple.bin` file with no headers or delimiters.

To read frame N, the ESP32 seeks to byte `N × 1024` in the file and reads 1024 bytes straight into a buffer, then pushes it to the display. No parsing needed.

### Audio

The MP3 file is stored separately on the SD card and decoded in real time using the ESP8266Audio library, outputting to the MAX98357A I2S DAC.

### Sync

Both audio and video start from the same `millis()` reference point. Instead of incrementing frames one by one, the ESP32 calculates the *expected* frame based on elapsed time — so if rendering falls behind, frames are skipped to catch up, keeping video locked to wall time and in sync with audio.

### Dual-core split

Audio decoding runs on **Core 0** as a FreeRTOS task, while video rendering runs on **Core 1** (the default Arduino loop core). This ensures the audio buffer stays fed even when the display is busy pushing a frame over I2C.

---

## 🛒 Hardware

| Component | Details |
|-----------|---------|
| ESP32 | DevKitC V4 (or any ESP32 with enough GPIOs) |
| OLED Display | SSD1306 128×64 I2C |
| DAC | MAX98357A I2S amplifier module |
| SD card module | SPI SD card reader |
| Rotary encoder | Any standard KY-040 type encoder |
| SD card | FAT32, 8GB or under recommended |
| Speaker | Any small 4Ω or 8Ω speaker |

---

## 🔌 Wiring

### SSD1306 OLED (I2C)

| OLED | ESP32 |
|------|-------|
| SDA | GPIO 21 |
| SCL | GPIO 22 |
| VCC | 3.3V |
| GND | GND |

### SD Card Module (SPI)

| SD Module | ESP32 |
|-----------|-------|
| MOSI | GPIO 23 |
| MISO | GPIO 19 |
| SCK | GPIO 18 |
| CS | GPIO 5 |
| VCC | 3.3V |
| GND | GND |

### MAX98357A I2S DAC

| MAX98357A | ESP32 |
|-----------|-------|
| BCLK | GPIO 26 |
| LRC | GPIO 25 |
| DIN | GPIO 27 |
| VIN | 5V |
| GND | GND |

> ⚠️ The SD_MODE pin on the MAX98357A should be pulled to 3.3V through a 100kΩ resistor for normal operation. Most breakout boards (e.g. Adafruit's) already do this onboard.

### Rotary Encoder

| Encoder | ESP32 |
|---------|-------|
| CLK | GPIO 32 |
| DT | GPIO 33 |
| SW | GPIO 34 |
| VCC | 3.3V |
| GND | GND |

> ⚠️ GPIO 34 is input-only and has no internal pullup. Connect a 10kΩ resistor between SW and 3.3V externally.

---

## 🚀 Setup
 
### 1. Install dependencies
 
In PlatformIO or Arduino IDE, install:
 
- `Adafruit SSD1306`
- `Adafruit GFX Library`
- `earlephilhower/ESP8266Audio`
 
### 2. Get the video frames
 
`bad_apple.bin` is already provided in this repo — no conversion needed. Just use it as-is.
 
> If you want to regenerate it yourself for any reason, you'll need Python, Pillow, and ffmpeg. Extract frames with `ffmpeg -i bad_apple.mp4 -vf "fps=20,scale=128:64" frames/frame_%04d.png` then run `python pack_frames.py`.
 
### 3. Get the audio
 
Download the MP3 from the original video using a YouTube to MP3 converter like [ytmp3.sc](https://ytmp3.sc):
 
> **[【東方】Bad Apple!! ＰＶ【影絵】 — https://youtu.be/FtutLA63Cp8](https://youtu.be/FtutLA63Cp8?si=1KIgajlmQthSrCgQ)**
 
Rename the downloaded file to exactly `bad_apple.mp3`.
 
### 4. Copy to SD card
 
Format your SD card as **FAT32**, then copy both files to the root:
 
```
bad_apple.bin
bad_apple.mp3
```
 
### 5. Flash the firmware
 
Open `bad-apple-esp32.ino` in Arduino IDE or PlatformIO and flash to your ESP32. Open the serial monitor at 115200 baud — it will wait for the SD card to be inserted, then start playback automatically.

---

## 📁 File Structure

```
bad-apple-esp32/
├── bad-apple-esp32.ino   — Main firmware: display, audio, sync, volume control
├── pack_frames.py        — Python script to convert video frames into bad_apple.bin
├── bad_apple.bin         — Packed binary of all video frames (1024 bytes each)
├── .gitignore
├── LICENSE
└── README.md
```

**`pack_frames.py`** — Takes the PNG frames extracted by ffmpeg, converts each one to 1-bit (black and white), and writes them back-to-back into a single binary file. Run this once on your PC before copying to the SD card.

**`bad-apple-esp32.ino`** — The ESP32 firmware. Handles SD card init, OLED display, I2S audio decoding, frame-skip sync logic, FreeRTOS task setup, and rotary encoder volume control.

**`bad_apple.bin`** — The output of `pack_frames.py`. Not stored in the repo (too large) — you generate it yourself. Lives on the SD card, not in the project folder.

---

## 📄 License

Distributed under the MIT License. See `LICENSE` for more information.

---

<div align="center">

**Made with ❤️ by [HimC29](https://github.com/HimC29)**

[Report Bug](https://github.com/HimC29/bad-apple-esp32/issues) • [Request Feature](https://github.com/HimC29/bad-apple-esp32/issues)

</div>