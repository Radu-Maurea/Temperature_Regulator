#ifndef WIFI_DRIVER_H
#define WIFI_DRIVER_H

#include <stdbool.h>

// WiFi driver interface (Arduino UNO R4 WiFi, WiFiS3 library).
// Connects the board to the phone hotspot as a normal WiFi client.

// Connects to the hotspot. Blocks until connected or until the timeout runs out.
// Returns true if the board is connected and has an IP address.
bool Wifi_Init(void);

// True while the board is connected to the hotspot.
bool Wifi_IsConnected(void);

// Tries to connect again if the connection was lost. Call it from loop().
void Wifi_Task(void);

// Writes the board's IP address as text ("192.168.43.17") into out.
// Writes "No WiFi" if the board is not connected.
void Wifi_GetIp(char *out, int len);

#endif
