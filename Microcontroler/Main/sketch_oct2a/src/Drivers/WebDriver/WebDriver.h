#ifndef WEB_DRIVER_H
#define WEB_DRIVER_H

// Web server interface. Serves the Web APP page on port 80 and its two endpoints:
//   GET /                   the page itself
//   GET /status             {"temp":..,"target":..,"power":..,"fault":..}
//   GET /set?target=22.5    changes the target, answers like /status
// Needs a WiFi connection, so call it after Wifi_Init().

// Starts listening on port 80.
void Web_Init(void);

// Answers one waiting request, if there is one. Call it from loop().
void Web_Task(void);

// Target temperature set from the page, in degrees C (20 - 50).
float Web_GetTarget(void);

#endif
