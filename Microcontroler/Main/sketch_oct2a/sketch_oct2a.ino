#include "src/Drivers/WifiDriver/WifiDriver.h"
#include "src/Drivers/DispDriver/DispDriver.h"
#include "src/Drivers/WebDriver/WebDriver.h"
#include "src/Drivers/RtcDriver/Rtc.h"
#include "src/Drivers/DigitalTempDriver/DigitalTempDriver.h"

void setup() {
  Serial.begin(9600);

  // Display first, so the screen is not dark while WiFi connects (up to 20 s).
  Disp_Init();
  Disp_Draw_Top("--:--:--");   // until the RTC gives the first time
  Disp_Draw_Middle();
  Disp_ShowExpected(Web_GetTarget());
  Disp_ShowConnecting();

  Wifi_Init();
  Rtc_Init();          // needs WiFi for NTP
  DigitalTemp_Init();

  char ip[16];
  Wifi_GetIp(ip, sizeof(ip));
  Disp_Draw_Bottom(ip);

  Web_Init();
}

void loop() {
  static float shownTarget = Web_GetTarget();

  Wifi_Task();
  Web_Task();

  // Redraw only once a second
  char time[9];
  if (Rtc_Update(time, sizeof(time))) {
    Disp_Draw_Top(time);
  }

  // Redraw only when the phone set a new target; redrawing every loop would flicker.
  if (Web_GetTarget() != shownTarget) {
    shownTarget = Web_GetTarget();
    Disp_ShowExpected(shownTarget);
  }

  static unsigned long lastRead = 0;
  if (millis() - lastRead >= 1000) {
    lastRead = millis();
    float temp = DigitalTemp_Read();
    Disp_ShowCurrent(temp);
    Web_SetTemp(temp);
  }
}
