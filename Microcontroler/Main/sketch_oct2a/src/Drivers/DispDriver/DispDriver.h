#ifndef DISP_DRIVER_H
#define DISP_DRIVER_H

// Display driver interface (ILI9341, 320 x 240).
// The rest of the system only talks to the display through these functions.

// Starts the display: SPI speed, rotation and panel orientation.
void Disp_Init(void);

// Draws a test pattern: colored corners, white border and the screen size.
void Disp_TestScreen(void);

#endif
