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

void Disp_ShowDebug(void)
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

// Main screen layout, shared by Disp_Draw and the functions that fill in values.
#define MAIN_TEXT_SIZE  2
#define MAIN_LINE_H     (8 * MAIN_TEXT_SIZE)   // default font: 8 px tall per size unit
#define MAIN_CHAR_W     (6 * MAIN_TEXT_SIZE)   // default font: 6 px wide per size unit
#define MAIN_LEFT       10

static const char EXPECTED_LABEL[] = "Expected temperature:";

// Same space above, between and below the two lines.
static int mainGap(void)
{
  return (tft.height() - 2 * MAIN_LINE_H) / 3;
}

void Disp_Draw(void)
{
  const int textSize = MAIN_TEXT_SIZE;
  const int lineH    = MAIN_LINE_H;
  const int gap      = mainGap();
  const int left     = MAIN_LEFT;

  tft.fillScreen(ILI9341_BLACK);
  tft.setTextColor(ILI9341_WHITE);
  tft.setTextSize(textSize);

  tft.setCursor(left, gap);
  tft.print(EXPECTED_LABEL);

  tft.setCursor(left, 2 * gap + lineH);
  tft.print("Current temperature:");
}

void Disp_ShowIp(const char *ip)
{
  const int textSize = 1;
  const int lineH    = 8 * textSize;
  const int y        = tft.height() - lineH - 4;

  tft.fillRect(0, y, tft.width(), lineH, ILI9341_BLACK);   // clear the old address
  tft.setTextColor(ILI9341_WHITE);
  tft.setTextSize(textSize);
  tft.setCursor(10, y);
  tft.print("Web: http://");
  tft.print(ip);
}

void Disp_ShowExpected(float temp)
{
  const int x = MAIN_LEFT + (sizeof(EXPECTED_LABEL) - 1) * MAIN_CHAR_W + MAIN_CHAR_W / 2;
  const int y = mainGap();

  // Printed as tenths ("22.5") so we don't depend on float support in snprintf.
  int t10 = (int)roundf(temp * 10.0f);
  char text[8];
  snprintf(text, sizeof(text), "%d.%d", t10 / 10, t10 % 10);

  tft.fillRect(x, y, tft.width() - x, MAIN_LINE_H, ILI9341_BLACK);   // clear the old value
  tft.setTextColor(ILI9341_WHITE);
  tft.setTextSize(MAIN_TEXT_SIZE);
  tft.setCursor(x, y);
  tft.print(text);
}
