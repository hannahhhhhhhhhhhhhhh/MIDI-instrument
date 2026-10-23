# MIDI-instrument!
Inspired by a standard gamepad, anyone interested in music production can pick this MIDI instrument up without already knowing how to play an instrument. If someone 
can already play piano, they probably have access to a piano...
Soo this is for everyone else.
<img width="1451" height="948" alt="image" src="https://github.com/user-attachments/assets/d19204cd-abad-40f3-89f5-189096fbbe4b" />

## Why I Made This
The design of this instrument is inspired by standard controller controls rather than shaped to mirror a certain instrument because it is for people who do not have a musical instrument, and more generally people who are interested in music production. Me and my friend often produce music together and sometimes what we find annoying is that we need a whole keyboard or a digital drum set MIDI inputs, specific MIDI guitars, etc, which is why I want my controls to be more universal rather than targeting one single instrument. It is also annoying switching from MIDI instruments to mouse then to keyboard when producing music, so I made the arrow controls easier to navigate between UIs.

## How It Works
The controller is built around the XIAO ESP32-S3 microcontroller, which reads all inputs and sends MIDI signals to a DAW over USB. The board has 7 main key switches arranged in a controller layout (t1-t7), 3 additional push buttons along the top (SW2, SW3, SW4), and 8 SK6812MINI-E addressable RGB LEDs chained in series for visual feedback. The LEDs are driven from the ESP32-S3 GPIO8 data line and powered at 3.3V. The key switches connect through a 7-pin connector (J1) back to the GPIO pins on the MCU.

### Control Layout
The up and down arrows would be for octave switching, while left and right arrows are for semitones, and the middle button is selecting/confirm the notes(t1-5). The two keys on the left of the arrow controls are for more detailed settings. The top left(t6) is for switching the mode to selecting note length(which will correspond to X key on the software), where then the up/right arrow would be lengthen the note and down/left would be shorten. 
The bottom left(t7) is mapped to the 'g' key on my computer which is the shortcut for me on vital studio to change the effect curve shape. The 3 switches on the top could be mapped to any 3 keyboard shortcut as well, for now I have it on 'e', 'esc', and 'r'.
The two rows of LEDs on the right side of the board serve as a stats display, showing feedback for the current mode or selection.

## Schematic
<img width="1619" height="668" alt="image" src="https://github.com/user-attachments/assets/01e3f035-8754-44ae-865d-3c2512bd40c9" />


## PCB Layout
<img width="1365" height="849" alt="image" src="https://github.com/user-attachments/assets/109adba1-ec50-4f8c-a238-b67eb708ad68" />

## Firmware Flashing Guide

### What You Need
- Arduino IDE 
- ESP32 board package installed
- USB-C cable

### Installing the ESP32 Board Package
1. Open Arduino IDE
4. Go to Tools > Board > Boards Manager
5. Search "esp32" and install the package by Espressif Systems

### Connecting the Board
1. Plug the XIAO ESP32-S3 into your computer via USB-C
2. Wait for drivers to install
3. If the port does not appear, download the CH343 driver from 
   seeedstudio.com

### Selecting the Board
1. Go to Tools > Board > esp32 > XIAO_ESP32S3
2. Go to Tools > Port and select the port that appeared when 
   you plugged in the board

### Uploading
1. Open `keyboard.ino` in Arduino IDE
2. Click the Upload button (right arrow, top left)
3. Wait for "Done uploading" in the console
4. The controller is ready to use

## Bill of Materials

| Item | Qty | Supplier | Cost/Item (CAD) | Total Cost (CAD) |
|------|-----|----------|-----------------|------------------|
| XIAO ESP32-S3 | 1 | Seeed Studio | $7.49 | $7.49 |
| SW_Push Button | 1 | YUNZII | $12.99 | $12.99 |
| Conn_01x07 7-pin Header | 1 | Seeed Studio | $0.64 | $0.64 |
| PCB Fabrication (JLCPCB) | 1 | JLCPCB | $9.40 | $9.40 |
| Estimated Shipping | 1 | — | — | $2.00 |
| | | | **Total** | **$32.52** |

