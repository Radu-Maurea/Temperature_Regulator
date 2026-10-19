# PSN

- `Microcontroler/` – Arduino UNO R4 WiFi firmware (ILI9341 TFT display, WiFi, web server)
- `Web APP/` – web interface (`index.html`), served by the Arduino
- `tools/embed_page.py` – copies `Web APP/index.html` into the firmware

## Setting up the Arduino project

### 1. Install the libraries

The sketch needs these libraries. They all come from the Arduino Library Manager, and these are the versions we tested with:

| Library             | Version |
|---------------------|---------|
| Adafruit ILI9341    | 1.6.4   |
| Adafruit GFX Library| 1.12.6  |
| Adafruit BusIO      | 1.17.4  |
| Adafruit STMPE610   | 1.1.6   |
| Adafruit TouchScreen| 1.1.6   |
| Adafruit TSC2007    | 1.1.2   |
| Adafruit SH110X     | 2.1.15  |

**Option A – Arduino IDE (easiest):**
Go to *Tools → Manage Libraries…*, search for **Adafruit ILI9341** and click *Install*.
When it asks about dependencies, click **Install All**. That pulls in all the other libraries above.

**Option B – arduino-cli (exact versions):**

```sh
arduino-cli lib install "Adafruit ILI9341@1.6.4" "Adafruit GFX Library@1.12.6" "Adafruit BusIO@1.17.4" "Adafruit STMPE610@1.1.6" "Adafruit TouchScreen@1.1.6" "Adafruit TSC2007@1.1.2" "Adafruit SH110X@2.1.15"
```

### 2. Open and upload

Open `Microcontroler/Main/sketch_oct2a/sketch_oct2a.ino` in the Arduino IDE. Then select **Arduino UNO R4 WiFi** and its port, and upload.
WiFi uses the `WiFiS3` library, which comes with the UNO R4 board package (no extra install).

### 3. Open the web page on the phone

1. Turn on the hotspot **Radu's Galaxy A51** (password `12345678`). It must use the **2.4 GHz** band and **WPA2** security – the UNO R4 cannot join 5 GHz or WPA3-only networks.
2. Reset the board. The display starts right away and shows `Connecting...` at the bottom while it joins the hotspot (up to 20 s), then `Web: http://<ip>`.
3. On the phone, open that address in the browser.

The Serial Monitor (115200 baud) also prints the connection steps and the IP.
The WiFi name and password are in `src/Drivers/WifiDriver/WifiDriver.cpp`.

### Changing the web page

Edit `Web APP/index.html`, then run

```sh
python tools/embed_page.py
```

and upload again. The script regenerates `src/Drivers/WebDriver/WebPage.h` (never edit that file by hand).
Opening `index.html` directly from disk still works and runs it in mock mode.

### Project structure

```
Microcontroler/Main/sketch_oct2a/
├── sketch_oct2a.ino            main program – only calls driver functions
└── src/Drivers/
    ├── WifiDriver/
    │   ├── WifiDriver.h        WiFi interface (Wifi_Init, Wifi_Task, Wifi_GetIp, ...)
    │   └── WifiDriver.cpp      hotspot name/password, WiFiS3 library calls
    ├── DispDriver/
    │   ├── DispDriver.h        display interface (Disp_Init, Disp_Draw, Disp_ShowConnecting, Disp_ShowIp, ...)
    │   └── DispDriver.cpp      implementation (pins, Adafruit library calls)
    └── WebDriver/
        ├── WebDriver.h         web server interface (Web_Init, Web_Task, Web_GetTarget)
        ├── WebDriver.cpp       HTTP server on port 80: /, /status, /set?target=
        └── WebPage.h           GENERATED from Web APP/index.html
```

Drivers live in `src/` because the Arduino IDE only compiles files inside the sketch folder, and it compiles everything under `src/` automatically.
To add a new driver, create `src/Drivers/<Name>/<Name>.h` + `.cpp` and include it in the `.ino` with
`#include "src/Drivers/<Name>/<Name>.h"`.

### Wiring (ILI9341)

| Display | Arduino pin |
|---------|-------------|
| CS      | 10          |
| DC      | 9           |
| RST     | 8           |
| MOSI/MISO/SCK | hardware SPI |
"# Temperature_Regulator" 
