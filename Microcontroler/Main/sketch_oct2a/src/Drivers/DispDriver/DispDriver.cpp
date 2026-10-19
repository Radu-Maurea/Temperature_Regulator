#include "DispDriver.h"

#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>

#define TFT_CS   10
#define TFT_DC   9
#define TFT_RST  8
#define TOP_H       24
#define BOTTOM_H    24   // same height as the top bar
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

#define MAIN_LINES      2

static const char EXPECTED_LABEL[] = "Expected temperature:";
static const char CURRENT_LABEL[]  = "Current temperature:";

// Top of line n (0..MAIN_LINES-1), with the same space above, between and below the lines
// in the area between the top and bottom bars.
static int mainLineY(int n)
{
  const int areaH = tft.height() - TOP_H - BOTTOM_H;
  const int gap   = (areaH - MAIN_LINES * MAIN_LINE_H) / (MAIN_LINES + 1);
  return TOP_H + gap + n * (gap + MAIN_LINE_H);
}

void Disp_Draw_Top(const char *time)
{
  tft.fillRect(0, 0, tft.width(), TOP_H, ILI9341_NAVY);

  tft.setTextSize(2);
  tft.setTextColor(ILI9341_WHITE);
  tft.setCursor(10, 6);
  tft.print("Time: ");   // print() continues on the same line,
  tft.print(time);       // so two calls show up as "Time: 12:34:56"

}

void Disp_Draw_Middle(void)
{
  // Only clear the area between the top and bottom bars, so they stay visible.
  tft.fillRect(0, TOP_H, tft.width(), tft.height() - TOP_H - BOTTOM_H, ILI9341_BLACK);
  tft.setTextColor(ILI9341_WHITE);
  tft.setTextSize(MAIN_TEXT_SIZE);

  tft.setCursor(MAIN_LEFT, mainLineY(0));
  tft.print(EXPECTED_LABEL);

  tft.setCursor(MAIN_LEFT, mainLineY(1));
  tft.print(CURRENT_LABEL);
}

// Clears the bottom bar and leaves the cursor at the start of its text line.
static void startBottomLine(void)
{
  const int barY  = tft.height() - BOTTOM_H;
  const int textSize = 1;
  const int textH = 8 * textSize;

  tft.fillRect(0, barY, tft.width(), BOTTOM_H, ILI9341_NAVY);
  tft.setTextColor(ILI9341_WHITE);
  tft.setTextSize(textSize);
  tft.setCursor(MAIN_LEFT, barY + (BOTTOM_H - textH) / 2);
}

void Disp_ShowConnecting(void)
{
  startBottomLine();
  tft.print("Connecting...");
}

void Disp_Draw_Bottom(const char *ip)
{
  startBottomLine();
  tft.print("Web: http://");
  tft.print(ip);
}

// Writes a temperature as tenths ("22.5") right after a label of labelLen chars on row y.
// NAN (sensor missing / failed) is shown as "--.-".
// Only the value area is cleared, so the label does not flicker.
static void showTemp(size_t labelLen, int y, float temp)
{
  const int x = MAIN_LEFT + labelLen * MAIN_CHAR_W + MAIN_CHAR_W / 2;

  // Printed as tenths ("22.5") so we don't depend on float support in snprintf.
  char text[8];
  if (isnan(temp)) {
    snprintf(text, sizeof(text), "--.-");
  } else {
    int t10 = (int)roundf(temp * 10.0f);
    snprintf(text, sizeof(text), "%s%d.%d", t10 < 0 ? "-" : "", abs(t10) / 10, abs(t10) % 10);
  }

  tft.fillRect(x, y, tft.width() - x, MAIN_LINE_H, ILI9341_BLACK);   // clear the old value
  tft.setTextColor(ILI9341_WHITE);
  tft.setTextSize(MAIN_TEXT_SIZE);
  tft.setCursor(x, y);
  tft.print(text);
}

void Disp_ShowExpected(float temp)
{
  showTemp(sizeof(EXPECTED_LABEL) - 1, mainLineY(0), temp);
}

void Disp_ShowCurrent(float temp)
{
  showTemp(sizeof(CURRENT_LABEL) - 1, mainLineY(1), temp);
}

