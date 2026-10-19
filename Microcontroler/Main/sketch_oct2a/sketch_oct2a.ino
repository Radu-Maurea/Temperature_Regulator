#include "src/Drivers/WifiDriver/WifiDriver.h"
#include "src/Drivers/DispDriver/DispDriver.h"
#include "src/Drivers/WebDriver/WebDriver.h"

void setup() {
  Serial.begin(115200);

  // Display first, so the screen is not dark while WiFi connects (up to 20 s).
  Disp_Init();
  Disp_Draw();
  Disp_ShowExpected(Web_GetTarget());
  Disp_ShowConnecting();

  Wifi_Init();

  char ip[16];
  Wifi_GetIp(ip, sizeof(ip));
  Disp_ShowIp(ip);

  Web_Init();
}

void loop() {
  static float shownTarget = Web_GetTarget();

  Wifi_Task();
  Web_Task();

  // Redraw only when the phone set a new target; redrawing every loop would flicker.
  if (Web_GetTarget() != shownTarget) {
    shownTarget = Web_GetTarget();
    Disp_ShowExpected(shownTarget);
  }
}
