# HyperSPI
SPI bridge for AWA protocol to control a LED strip from HyperHDR.  
Diagnostic and performance data available at the serial port output [read more](#performancedebug-output).  
Raspberry Pi acts as a master, ESP8266/ESP32/ESP32-S2/rp2040(Raspberry Pi Pico) is in slave mode. 
  
| LED strip / Device             | rp2040 / Pico | ESP8266<br>(limited performance) |    ESP32 / ESP32-S2 mini    
|--------------------------------|:-------:|:-----------:|:-------:|
| SK6812 cold white              |   yes   |   yes   |     yes     |
| SK6812 neutral white           |   yes   |   yes   |     yes     |
| WS281x                         |   yes   |   yes   |     yes     |
  
  
# Why this project was created?

- SPI is very faster. HyperSPI works best at speed over 20Mb
- SPI doesn't have any data integration check. But AWA protocol does have one
- you don't need to have 2Mb capable serial port on your ESP board
- SPI transmission is much lighter than serial communication
- There is a hardware limitation for the Rpi current design...even if you connect your grabber using USB2.0 mode, working serial port driver (used by Adalight) results in quite a large drop in overall USB transfer. So we can replace Adalight with a pure SPI data transfer as an alternative
- I needed it and I was able to implemented it 😉

# Hardware connection  

If you are using an ESP board compatible with the Wemos board (ESP8266 Wemos D1/pro, ESP32 MH-ET Live, ESP32-S2 lolin mini), the SPI connection uses the same pinout location on the ESP board! The pin positions of the LED output may vary. Cables (including ground) should not exceed 15-20cm or it may be necessary to lower the SPI speed. 

The photos below use the same home-made adapter throughout, so you can see a repeating pattern and the cable colors should help you locate the correct pins. However, always consult the GPIO diagram for your boards to confirm that you have connected the cables correctly, because if you make a mistake and connect to the 5V GPIO line, it may damage both devices.  

As you can also notice, the pinout of the SPI0 interface is identical for the entire Raspberry Pi SBC family: 3, 4, 5, Zero 2W, etc.  

<table>  
  <tr>
    <td colspan="2"><p align="center">See how easy it is to connect Raspberry Pi Pico (rp2040) to Raspberry Pi 5 using SPI</p></td>    
  </tr>
  <tr>
    <td colspan="2"><img src="https://github.com/awawa-dev/HyperSPI/assets/69086569/7f24f87e-f7e0-43f3-a568-39d0f6beced1"/></td>
  </tr>
  <tr>
    <td><img src="https://github.com/awawa-dev/HyperSPI/assets/69086569/170f5718-df88-4ef2-9ed8-7d92d913aeec"/></td>
    <td><img src="https://github.com/awawa-dev/HyperSPI/assets/69086569/9235c689-062e-4c62-b632-8c506b0e2e97"/></td>
  </tr>
  <tr>
    <td colspan="2"><p align="center">or if you prefer ESP32/ESP32-S2/Esp8266</p></td>
  </tr>
  <tr>
    <td><img src="https://github.com/awawa-dev/HyperSPI/assets/69086569/1e500ca3-e93d-4082-af59-b701e6274a29"/></td>
    <td><img src="https://github.com/awawa-dev/HyperSPI/assets/69086569/6e9ea441-312e-46d8-99b0-d33fc0b9f923"/></td>
  </tr>
  <tr>
    <td><img src="https://user-images.githubusercontent.com/69086569/216763154-ca4aa8fa-5855-43c1-86c2-d401010de675.png"/></td>
    <td><img src="https://user-images.githubusercontent.com/69086569/231207350-a670bfea-a96d-4d21-9e8f-f2ca027105da.png"/></td>
  </tr> 
  <tr>
    <td colspan="2"><p align="center">multi-architecture support via FTDI (ex. FT232H)</p></td>
  </tr>
  <tr>
    <td colspan="2"><img src="https://github.com/user-attachments/assets/153cd27b-9fa8-4223-8770-fcd54da59a37"/></td>    
  </tr>  
</table>

# Example of supported boards

<p align="center">
<b>Adafruit RP2040 Scorpio and ItsyBitsy with built-in level shifter (recommended!)</b><br/>  
<img src="https://github.com/awawa-dev/HyperSPI/assets/69086569/c7ef2768-357e-47ca-aa1c-543769eeb360" width="250" /><img src="https://github.com/awawa-dev/HyperSPI/assets/69086569/3e55948d-fd3d-44cb-908c-9aad8ce2715d" width="250" />
</p>

<p align="center">
<b>Esp8266 Wemos D1 mini (CH340) and Wemos D1 mini pro (CP2104)</b><br/>
<img src="https://user-images.githubusercontent.com/69086569/207572306-2b0bd3dd-fcb2-4f0c-8426-64341cbbadbf.png" width="250" /><img src="https://user-images.githubusercontent.com/69086569/207572335-9caf2567-2e23-4ee4-85a4-0f2f82676c16.png" width="250" />
</p>

<p align="center">
<b>ESP32 MH-ET Live and ESP32-S2 Lolin mini (CDC)</b><br/>
<img src="https://user-images.githubusercontent.com/69086569/207587620-1c4c53c8-426c-486e-a6d9-d429fd1b050d.png" width="250"/><img src="https://user-images.githubusercontent.com/69086569/207587635-b7816329-0e29-47ee-a75a-bc6c41cdc51f.png" width="250"/>
</p>

## Default pinout (can be changed for esp32, esp32-s2 and rp2040 Pico)
  
|    PINOUT   |  ESP8266  |   ESP32   | ESP32-S2 | Pico (rp2040) | Adafruit rp2040<br/>Scorpio / ItsyBitsy
|-------------|-----------|-----------|-----------|-----------|-----------|
| Clock (SCK) | GPIO 14   | GPIO 18   | GPIO 7    | GPIO 2    | GPIO 26 (A0) |
| Data (MOSI) | GPIO 13   | GPIO 23   | GPIO 11   | GPIO 4    | GPIO 28 (A2) |
| SPI Chip Select(e.g. CE0) | not used    | GPIO 5    | GPIO 12   | GPIO 5    | GPIO 29 (A3) |
| GROUND      | mandatory | mandatory | mandatory | mandatory | mandatory |
| LED output  | GPIO 2    | GPIO 2    | GPIO 2    | GPIO 14 | GPIO16 / GPIO14 |

> [!CAUTION]
> The ground connection between both GPIOs is as important as the other SPI data connections. The ground cable should be of a similar length as them and run directly next to them.

# Flashing the firmware

There are two versions of the firmware for ESP32 and ESP32-S2. The 'factory' (in the `recovery_firmware.zip` archive) and the 'base' one. Factory firmware should be flashed to offset 0x0, base firmware to offset 0x10000.

## Flashing ESP32-S2 Lolin mini

Requires using `esptool.py` to flash the firmware e.g.  
 - `esptool.py write_flash 0x10000 hyperspi_esp32_s2_mini_SK6812_RGBW_COLD.bin` or
 - `esptool.py write_flash 0x0 hyperspi_esp32_s2_mini_SK6812_RGBW_COLD.factory.bin`

Troubleshooting: ESP32-S2 Lolin mini recovery procedure if the board is not detected or is malfunctioning.  
1. Put the board into dfu mode using board buttons: press `Rst` + `0` buttons, then release `Rst`, next release `0`  
Do not reset or disconnect the board until the end of the recovery procedure.
2. Execute `esptool.py erase_flash`  
3. Flash 'factory' version of the firmware e.g.  
`esptool.py write_flash 0x0 hyperspi_esp32_s2_mini_SK6812_RGBW_COLD.factory.bin`  
4. **`esptool.py` is not able to automatically reset esp32-s2 when in dfu mode. Reconnect or hard reset it manually.** The board should be detected as a COM port in the system.

## Flashing generic Esp8266/ESP32

Recommend to use [esphome-flasher](https://github.com/esphome/esphome-flasher/releases)  
Or use `esptool.py` e.g.  
 - `esptool.py write_flash 0x10000 hyperspi_esp32_SK6812_RGBW_COLD.bin` or
 - `esptool.py write_flash 0x0 hyperspi_esp32_SK6812_RGBW_COLD.factory.bin`

For **RGBW LED strip** like RGBW SK6812 NEUTRAL white choose: *hyperspi_..._SK6812_RGBW_NEUTRAL.bin*  
For **RGBW LED strip** like RGBW SK6812 COLD white choose: *hyperspi_..._SK6812_RGBW_COLD.bin*  
For **RGB LED strip** like WS8212b or RGB SK6812 variant choose: *hyperspi_..._WS281x_RGB.bin*  

## Flashing Pico boards

It's very easy and you don't need any special flasher.  

Use firmware from the `hyperspi_pico_rp2040.zip` archive. Adafruit boards have their own custom firmware package inside the archive: `Adafruit_ItsyBitsy_RP2040.zip` and `Adafruit_Feather_RP2040_Scorpio.zip`

Put your Pico board into DFU mode:  
* If your Pico board has only one button (`boot`) then press & hold it and connect the board to the USB port. Then you can release the button.
* If your Pico board has two buttons, connect it to the USB port. Then press & hold `boot` and `reset` buttons, then release `reset` and next release `boot` button.  

In the system file explorer you should find new drive (e.g. called `RPI-RP2` drive) exposed by the Pico board. Drag & drop (or copy) the selected firmware to this drive. 
The Pico will reset automaticly after the upload and after few seconds it will be ready to use.
  
# Software configuration (HyperHDR v17 and above)

**In HyperHDR `Image Processing→Smoothing→Update frequency` you should do not exceed the maximum capacity of the device. Read more here: [testing performance](https://github.com/awawa-dev/HyperSPI#performance-output)**

Select `esp8266` protocol for ESP proprietary SPI protocol, `esp32` for ESP32 boards, `rp2040 (Pico)` for Pico boards or `standard` for other devices.    
Make sure you set "Refresh time" to zero, "Baudrate" should be set to high but realistic value like ```25 000 000```.  
Enabling "White channel calibration" is optional, if you want to fine tune the white channel balance of your sk6812 RGBW LED strip.
  
<img src="https://user-images.githubusercontent.com/69086569/193319124-0054f367-3d30-4e50-8c52-3683c7bbc50e.png" width="800"/>

# Benchmark results

## ESP32 & ESP32-S2 parallel multi-segment mode

| sk6812 LED strip / Device                                                                      | ESP32 MH-ET LIVE mini<br/>HyperSPI v9 |  ESP32-S2 Lolin mini<br/>HyperSPI v9 |
|-----------------------------------------------------------------------------------------|-----------------------|----------------------|
| 300 RGBW LEDs<br>Refresh rate/continues output=100Hz<br>SECOND_SEGMENT_START_INDEX=150 |          100          |          100         |
| 600 RGBW LEDs<br>Refresh rate/continues output=83Hz<br>SECOND_SEGMENT_START_INDEX=300  |           83          |           83         |
| 900 RGBW LEDs<br>Refresh rate/continues output=55Hz <br>SECOND_SEGMENT_START_INDEX=450 |         54-55         |           55         |

## SP8266 / ESP32

| sk6812 LED strip / Device                             | ESP32 MH ET Live<br/>HyperSPI v9 | ESP32-S2 Lolin mini<br/>HyperSPI v9 | ESP8266 Wemos D1 Pro<br/>HyperSPI v9 |
|------------------------------------------------|-----------------|--------------------|---------------------|
| 300 RGBW LEDs<br>continues output=83/70Hz  |        83       |          83        |          70         |
| 600 RGBW LEDs<br>continues output=43/33Hz  |      42-43      |          42        |          33         |
| 900 RGBW LEDs<br>continues output=28/22Hz  |       28        |          28        |          22         |

# Compiling

## ESP8266 / ESP32

Currently we use PlatformIO to compile the project. Install [Visual Studio Code](https://code.visualstudio.com/) and add [PlatformIO plugin](https://platformio.org/).
This environment will take care of everything and compile the firmware for you. Low-level LED strip support is provided by my highly optimizated (pre-fill I2S DMA modes, turbo I2S parallel mode for up to 2 segments etc) version of Neopixelbus library: [link](https://github.com/awawa-dev/NeoPixelBus).

## Pico rp2040

Use Pico SDK and Visual Code to open ```rp2040``` folder. Edit ```rp2040\CMakeLists.txt``` configuration file if you need to apply changes.

## Github Action

But there is also an alternative and an easier way. Just fork the project and enable its Github Action. Use the online editor to make changes:
- esp8266/ESP32 boards: to the ```platformio.ini``` file
- rp2040 Pico boards: to the ```rp2040\CMakeLists.txt``` file

for example change default pin-outs or enable multi-segments support, and save it. Github Action will compile new firmware automatically in the Artifacts archive. It has never been so easy! **Just remember to follow the steps in the correct order otherwise the Github Action may not be triggered the first time after saving the changes.**

Tutorial: https://github.com/awawa-dev/HyperSPI/wiki

# Multi-Segment Wiring (ESP32, ESP32-S2 and Pico rp2040 boards only)

## ESP32

Using parallel multi-segment allows you to double your Neopixel (e.g. sk6812 RGBW) LED strip refresh rate by dividing it into two smaller equal parts. Both smaller segments are perfectly in sync so you don't need to worry about it. Proposed example of building a multisegment:
- Divide a long or dense strip of LEDs into 2 smaller equal parts. So `SECOND_SEGMENT_START_INDEX` in the HyperSPI firmware is the total number of LEDs divided by 2.
- Build your first segment traditional way e.g. clockwise, so it starts somewhere in middle of the bottom of frame/TV and ends in the middle of the top of frame/TV
- Start the second segment in the opposite direction to the first one e.g. counterclockwise (`SECOND_SEGMENT_REVERSED` option in the HyperSPI firmware configuration must be enabled). So it starts somewhere in the middle of the bottom of the frame/TV and ends in the middle of the top of the TV/frame. Both segments could be connected if possible at the top but only 5v and ground ( NOT the data line).
- The data line starts for both segments somewhere in the middle of the bottom of the TV/frame (where each of the LED strips starts)
- Configuration in HyperHDR does not change! It's should be configured as one, single continues segment. All is done in HyperSPI firmware transparently and does not affect LED strip configuration in HyperHDR.

You also must configure data pin in the `platformio.ini`. Review the comments at the top of the file:
* `SECOND_SEGMENT_DATA_PIN` - These is data pin for your second strip

You add these to your board's config. Be sure to put `-D` in front of each setting. 

Examples of final build_flags for 288 LEDs divided into 2 equal segments in the `platformio.ini`:
```
[env:SK6812_RGBW_COLD]
build_flags = -DNEOPIXEL_RGBW -DCOLD_WHITE -DDATA_PIN=2 ${env.build_flags} -DSECOND_SEGMENT_START_INDEX=144 -DSECOND_SEGMENT_DATA_PIN=4 -DSECOND_SEGMENT_REVERSED
...
[env:WS281x_RGB]
build_flags = -DNEOPIXEL_RGB -DDATA_PIN=2 ${env.build_flags} -DSECOND_SEGMENT_START_INDEX=144 -DSECOND_SEGMENT_DATA_PIN=4 -DSECOND_SEGMENT_REVERSED
...
```
Implementation example:
- The diagram of the board for WS2812b/SK6812 including ESP32 and the SN74AHCT125N 74AHCT125 [level shifter](https://github.com/awawa-dev/HyperHDR/wiki/Level-Shifter).

## Pico rp2040

Edit ```rp2040\CMakeLists.txt``` file and recompile the project.

![HyperSPI](https://user-images.githubusercontent.com/85223482/222923979-f344349a-1f8b-4195-94ca-51721923359e.png)

# Automatic LED Power Control (ESP32)

HyperSPI can optionally drive a relay (or a MOSFET power switch) that disconnects the **positive 5V supply of the LED strip** when there is nothing to display, similar to the relay feature of WLED and HyperSerialESP32. The feature is compiled in only when `LED_POWER_PIN` is defined. Without it the firmware behaves exactly as before.

Tested target: ESP32-WROOM-32 (`esp32dev`). The code only uses `pinMode`/`digitalWrite`/`millis`, but other boards have not been verified.

## How to enable it

Use one of the ready-made environments. Both use data on GPIO2 and an active-high power switch on GPIO16, matching the P-MOSFET board in [`hardware/hyperspi-led-controller`](hardware/hyperspi-led-controller):

| LED strip | Environment | Firmware |
|---|---|---|
| SK6812 RGBW cold white | `SK6812_RGBW_COLD_LED_POWER` | `hyperspi_esp32_SK6812_RGBW_COLD_LED_POWER.bin` |
| WS281x RGB (WS2812B etc.) | `WS281x_RGB_LED_POWER` | `hyperspi_esp32_WS281x_RGB_LED_POWER.bin` |

```
pio run -e SK6812_RGBW_COLD_LED_POWER
```
You can also append the flags to the `build_flags` of any ESP32 environment in `platformio.ini`:
```ini
; active-low relay module (IN = LOW switches the LED power on)
build_flags = -DNEOPIXEL_RGB -DDATA_PIN=2 ${env.build_flags} -DLED_POWER_PIN=16 -DLED_POWER_INVERT

; active-high relay module or MOSFET board (IN = HIGH switches the LED power on)
build_flags = -DNEOPIXEL_RGB -DDATA_PIN=2 ${env.build_flags} -DLED_POWER_PIN=16
```

| Flag | Default | Description |
|---|---|---|
| `LED_POWER_PIN` | not defined (feature disabled) | GPIO connected to the relay control input |
| `LED_POWER_INVERT` | not defined (active-high) | Define it for active-low relay modules |
| `LED_POWER_OFF_DELAY` | `5000` | How long (ms) the LEDs must stay completely black before the power is switched off |
| `LED_POWER_ON_DELAY` | `200` | Time (ms) the LED supply gets to stabilize before the first frame is sent to the strip |
| `LED_POWER_IDLE_TIMEOUT` | `5000` | Time (ms) without any valid frame before the power is switched off. `0` disables it |
| `LED_POWER_DEBUG` | not defined | Prints `[LED POWER] ...` messages to the serial port (115200) on state changes only |

## Recommended GPIO

- **GPIO16** (default in the example). It is not a strapping pin and is not used by HyperSPI. Note: on WROVER modules GPIO16/17 are used by the PSRAM, so pick another pin there.
- Alternatives: GPIO17, 25, 26, 27, 32, 33.
- Avoid the strapping pins GPIO0, 2, 5, 12 and 15. Avoid the flash pins GPIO6–11, and the input-only pins GPIO34–39.
- HyperSPI already uses: SCK GPIO18, MISO GPIO19, MOSI GPIO23, CS GPIO5, LED data GPIO2 (and the second segment data pin if configured).

## Wiring

```
 5V PSU (+) ──── relay COM ──── relay NO ──── LED strip +5V
 5V PSU (−) ──────────────────────────────── LED strip GND ──── ESP32 GND ──── Raspberry Pi GND
 ESP32 GPIO16 ──────────── relay IN
 ESP32 GPIO2 ── level shifter (e.g. 74AHCT125) ── 330 Ω ── LED strip DIN
 ESP32 powered independently (USB or its own 5V supply that is NOT switched by the relay)
```

- The relay switches **only the positive 5V supply of the LED strip**. Never switch the ground: an LED strip with a floating ground can find a ground path through the data line and the ESP32.
- The ESP32 must stay powered while the LED power is off, otherwise it cannot switch it on again.
- The ESP32, the LED strip, the power supply and the Raspberry Pi must share a common ground.
- The relay or power switch must be rated for the **maximum DC current of the LED strip and its inrush current** when it is switched on, which can be a lot higher than the steady current. Mechanical relays are often rated for AC. Check the DC rating, or use a suitably rated logic-level MOSFET/high-side switch for higher currents.
- **Back-powering:** an unpowered addressable LED can be partly powered through its data input. HyperSPI does not render anything while the LED power is off, so the data line stays idle (low). A 3.3V→5V level shifter such as the 74AHCT125 / SN74AHCT1G125 powered from the unswitched 5V (the ESP32 side, not the relay output), plus a 220–470 Ω series resistor close to the strip, isolates the ESP32 GPIO and limits any current. A level shifter is recommended anyway for reliable 5V LED data.
- **Active-low modules:** many common opto-isolated relay boards switch on when IN is pulled LOW. If the module is powered from 5V, a 3.3V HIGH level from the ESP32 may not fully release the relay. Prefer a module that is specified for 3.3V logic or as a "high level trigger" type, power its input side from 3.3V if the board allows it (separate JD-VCC jumper), or use a transistor/MOSFET stage.

### Boot state

The firmware sets the relay pin to its inactive level as the first step of `setup()`, before the serial port, the LED driver and SPI are started. The level is written before the pin becomes an output, so there is no glitch. However, **software cannot control the pin during the ESP32 hardware reset and bootloader period** (a few hundred ms after power-up, reset or flashing). During that time GPIO16 is a high-impedance input. To keep the relay off during that time, add an external resistor:
- active-high module: 10 kΩ pull-down from the GPIO to GND
- active-low module: 10 kΩ pull-up from the GPIO to the logic level of the relay input

## Expected behavior

| Situation | Relay |
|---|---|
| ESP32 boots | OFF |
| Invalid/damaged frame (CRC or Fletcher checksum fails) | No change |
| Valid frame that is completely black, relay OFF | Stays OFF (the frame is not sent to the strip) |
| First valid non-black frame | ON. The frame is held for `LED_POWER_ON_DELAY` and then rendered, even if HyperHDR sends nothing more |
| Normal frames | Stays ON. The GPIO is written only when the state changes |
| Black scene shorter than `LED_POWER_OFF_DELAY` | Stays ON (black frames are rendered normally) |
| Black for `LED_POWER_OFF_DELAY` | OFF (measured from the first black frame, so it also works when HyperHDR sends black and then goes silent) |
| No valid frame for `LED_POWER_IDLE_TIMEOUT` | OFF (only if the timeout is not `0`) |
| Non-black frame while OFF | ON again, after the stabilization delay |

With multiple segments, the relay is treated as one shared power supply. The frame is black only if all LEDs of all segments are black.

All timers are non-blocking (`millis()` based and safe across its rollover), so SPI reception and LED rendering are not delayed.

## HyperHDR behavior and settings

- When you disable the LED device in HyperHDR (or stop it), HyperHDR sends 2 black frames and closes the SPI device. The relay switches off `LED_POWER_OFF_DELAY` later.
- The Awa protocol has no power on/off command. The relay is controlled only by the frame content and by timing.
- With `Image Processing→Smoothing` enabled (the HyperHDR default), a static image is still re-sent about once per second, which keeps the default 5 s `LED_POWER_IDLE_TIMEOUT` from triggering.
- **Limitation:** a static image that is not re-sent and a stopped or crashed host look identical over SPI. If you disable Smoothing or see the LEDs switch off while showing a static color, either enable `Smoothing→Continuous output` (consider a lower update frequency), or build with `-DLED_POWER_IDLE_TIMEOUT=0`. With `0`, the relay switches off only after black frames, so a crashed host leaves the LEDs on with the last image.
- Keep "Refresh time" at 0 as recommended above. It is not needed for this feature.

## Troubleshooting

- **Relay never switches on**: check the polarity (`LED_POWER_INVERT`), and check that the module accepts a 3.3V control signal. Enable `LED_POWER_DEBUG` and look for `[LED POWER] Relay ON` in the serial output. Black content never switches the relay on.
- **Relay clicks on at power-up or reset**: add the pull-up/pull-down resistor described above.
- **First frame is missing or flickers after power-on**: increase `LED_POWER_ON_DELAY` (e.g. 500) for power supplies that are slow to stabilize.
- **LEDs switch off during a static color**: see the idle timeout limitation above.
- **LEDs switch off too quickly in dark movie scenes**: increase `LED_POWER_OFF_DELAY` (e.g. 30000).
- **ESP32 resets when the relay switches**: the inrush current or relay coil is pulling the supply down. Power the ESP32 separately, add bulk capacitance (e.g. 1000 µF) at the strip input, and keep the relay coil supply away from the ESP32 3.3V regulator.

# Performance/debug output

**The output is only available when HyperHDR is not using the device at the moment, so it should be disabled in the app for a while.** Stores the last result when HyperHDR was running in the current session. You can read it from the serial port at a speed of 115200.

On Linux you can `screen` command.  
First install it: `sudo apt install screen`. Adjust USB port if necessary and connect to the serial port:  
`screen /dev/ttyACM0 115200`  
If you want to exit screen press `Ctrl-a` then `k` and confirm exit.

You can also use `Putty` on Windows
![obraz](https://user-images.githubusercontent.com/69086569/216762783-0ce47e57-98a7-474d-aa84-7e5afb42d294.png)  

For testing maximum performance in HyperHDR enable `Image Processing→Smoothing→Continuous output`, high `Update frequency` in the same tab and set any color in the `Remote control` tab as an active effect. After testing you need to disable `Continuous output`and set `Update frequency` according to your results.

  
 









