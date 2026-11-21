#include "src/Drivers/WifiDriver/WifiDriver.h"
#include "src/Drivers/DispDriver/DispDriver.h"
#include "src/Drivers/WebDriver/WebDriver.h"
#include "src/Drivers/RtcDriver/Rtc.h"
#include "src/Drivers/DigitalTempDriver/DigitalTempDriver.h"
#include "src/Drivers/HeaterDriver/HeaterDriver.h"
#include "src/Drivers/PidDriver/PidDriver.h"

void setup() {
  Serial.begin(9600);
  Heater_Init();       // first, so the bulb is off while everything else starts
  Pid_Init();

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
  static float temp = NAN;

  Wifi_Task();
  Web_Task();

  WebParams p;
  if (Web_TakeParams(&p)) {
    Pid_SetTunings(p.kp, p.ki, p.kd);
    Pid_SetProfile(p.tset, p.tinc, p.tmen, p.trac);
  }
  uint8_t cmd = Web_TakeCommand();
  if (cmd == WEB_CMD_START) Pid_Start(temp);
  if (cmd == WEB_CMD_STOP)  Pid_Stop();

  // PID output during a cycle
  // Bulb power chosen on the phone (0/25/50/100 %)
  Heater_SetDuty(Pid_IsRunning() ? Pid_GetOutput() : Web_GetDuty());
  Heater_Task();

  // Redraw only once a second
  char time[9];
  if (Rtc_Update(time, sizeof(time))) {
    Disp_Draw_Top(time);
  }

  // Setpoint on the display during a cycle
  // Redraw only when the target changes; redrawing every loop would flicker.
  float expected = Pid_IsRunning() ? roundf(Pid_GetSetpoint() * 10.0f) / 10.0f : Web_GetTarget();
  if (expected != shownTarget) {
    shownTarget = expected;
    Disp_ShowExpected(shownTarget);
  }

  static unsigned long lastRead = 0;
  if (millis() - lastRead >= 1000) {
    lastRead = millis();
    temp = DigitalTemp_Read();
    Pid_Update(temp);
    Disp_ShowCurrent(temp);
    Web_SetTemp(temp);
  }

  Web_SetRun(Pid_GetPhase(), Pid_IsRunning(), Pid_GetSetpoint(), Pid_GetOutput(), Pid_GetElapsed());
}
