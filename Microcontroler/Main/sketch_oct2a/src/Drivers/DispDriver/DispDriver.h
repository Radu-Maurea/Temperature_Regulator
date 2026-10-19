#ifndef DISP_DRIVER_H
#define DISP_DRIVER_H

// Display driver interface (ILI9341, 320 x 240).
// The rest of the system only talks to the display through these functions.

// Starts the display: SPI speed, rotation and panel orientation.
void Disp_Init(void);

// Debug layout: colored corners, white border and a test text.
void Disp_ShowDebug(void);

// Main screen: black background with the expected and current temperature labels.
void Disp_Draw(void);

// Writes the expected temperature (degrees C, e.g. 22.5) next to its label.
void Disp_ShowExpected(float temp);

// Small line at the bottom of the screen saying the board is joining the hotspot.
void Disp_ShowConnecting(void);

// Small line at the bottom of the screen with the address to open on the phone.
// Replaces the "Connecting..." line.
void Disp_ShowIp(const char *ip);

#endif
