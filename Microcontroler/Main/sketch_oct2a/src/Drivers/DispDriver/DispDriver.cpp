#include "DispDriver.h"

#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>

#define TFT_CS   10
#define TFT_DC   9
#define TFT_RST  8

// static: only this driver can touch the display object
static Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC, TFT_RST);

void Disp_Init(void)
{
  tft.begin(10000000);
  tft.setRotation(1);                            // tells the library: 320 x 240
  uint8_t madctl = 0x40;                         // panel orientation + RGB colors
  tft.sendCommand(ILI9341_MADCTL, &madctl, 1);
}

void Disp_TestScreen(void)
{
  int w = tft.width();
  int h = tft.height();

  tft.fillScreen(ILI9341_PINK);
  tft.drawRect(0, 0, w, h, ILI9341_WHITE);

  tft.fillRect(0,      0,      30, 30, ILI9341_RED);     // top-left
  tft.fillRect(w - 30, 0,      30, 30, ILI9341_GREEN);   // top-right
  tft.fillRect(0,      h - 30, 30, 30, ILI9341_BLUE);    // bottom-left
  tft.fillRect(w - 30, h - 30, 30, 30, ILI9341_YELLOW);  // bottom-right

  tft.setTextColor(ILI9341_WHITE);
  tft.setTextSize(2);
  tft.setCursor(100, 110);
  //tft.print(w);
  tft.print(" Oli<3 ");
  //tft.print(h);
}
