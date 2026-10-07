#include "WifiDriver.h"

#include <Arduino.h>
#include <WiFiS3.h>

#define WIFI_SSID        "Radu's Galaxy A51"
#define WIFI_PASS        "12345678"
#define WIFI_TIMEOUT_MS  20000UL   // give up on one connect attempt after this
#define WIFI_RETRY_MS    10000UL   // wait this long between reconnect attempts

static unsigned long lastTry = 0;

// The modem can report "connected" a moment before DHCP gives us an address,
// so we only count it as connected once the IP is not 0.0.0.0.
static bool hasIp(void)
{
  return WiFi.localIP() != IPAddress(0, 0, 0, 0);
}

bool Wifi_Init(void)
{
  if (WiFi.status() == WL_NO_MODULE) {
    Serial.println("WiFi: no WiFi module found");
    return false;
  }

  Serial.print("WiFi: connecting to ");
  Serial.println(WIFI_SSID);

  lastTry = millis();
  WiFi.begin(WIFI_SSID, WIFI_PASS);

  unsigned long start = millis();
  while (!Wifi_IsConnected() && millis() - start < WIFI_TIMEOUT_MS) {
    delay(250);
  }

  if (!Wifi_IsConnected()) {
    Serial.println("WiFi: connection failed");
    return false;
  }

  char ip[16];
  Wifi_GetIp(ip, sizeof(ip));
  Serial.print("WiFi: connected, IP ");
  Serial.println(ip);
  return true;
}

bool Wifi_IsConnected(void)
{
  return WiFi.status() == WL_CONNECTED && hasIp();
}

void Wifi_Task(void)
{
  if (Wifi_IsConnected() || millis() - lastTry < WIFI_RETRY_MS) {
    return;
  }
  Serial.println("WiFi: connection lost, retrying");
  WiFi.disconnect();
  Wifi_Init();
}

void Wifi_GetIp(char *out, int len)
{
  if (!Wifi_IsConnected()) {
    snprintf(out, len, "No WiFi");
    return;
  }
  IPAddress ip = WiFi.localIP();
  snprintf(out, len, "%u.%u.%u.%u", ip[0], ip[1], ip[2], ip[3]);
}
