#ifndef DISP_DRIVER_H
#define DISP_DRIVER_H

// Display driver interface (ILI9341, 320 x 240).
// The rest of the system only talks to the display through these functions.

// Starts the display: SPI speed, rotation and panel orientation.
void Disp_Init(void);

// Debug layout: colored corners, white border and a test text.
void Disp_ShowDebug(void);

// Main screen: black background with the expected and current temperature labels.


// Blue bar at the top of the screen with the current time ("HH:MM:SS").
void Disp_Draw_Top(const char *time);
// Writes the expected temperature (degrees C, e.g. 22.5) next to its label.


// Small line at the bottom of the screen saying the board is joining the hotspot.
void Disp_ShowConnecting(void);

// Small line at the bottom of the screen with the address to open on the phone.
// Replaces the "Connecting..." line.
void Disp_Draw_Bottom(const char *ip);
void Disp_Draw_Middle(void);
void Disp_ShowExpected(float temp);
// Writes the current (measured) temperature (degrees C, e.g. 23.1) next to its label.
// NAN (sensor missing / failed) shows "--.-".
void Disp_ShowCurrent(float temp);
#endif
