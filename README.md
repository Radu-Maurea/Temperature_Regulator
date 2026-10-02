# PSN

- `Microcontroler/` – Arduino firmware (ILI9341 TFT display)
- `Web APP/` – web interface (`index.html`)

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

Open `Microcontroler/Main/sketch_oct2a/sketch_oct2a.ino` in the Arduino IDE. Then select your board and port, and upload.

### Project structure

```
Microcontroler/Main/sketch_oct2a/
├── sketch_oct2a.ino            main program – only calls driver functions
└── src/Drivers/
    └── DispDriver/
        ├── DispDriver.h        display interface (Disp_Init, Disp_TestScreen, ...)
        └── DispDriver.cpp      implementation (pins, Adafruit library calls)
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
