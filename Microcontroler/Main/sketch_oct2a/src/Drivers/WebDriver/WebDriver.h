#ifndef WEB_DRIVER_H
#define WEB_DRIVER_H

#include <stdint.h>

// Web server interface. Serves the Web APP page on port 80 and its endpoints:
//   GET /                   the page itself
//   GET /status             {"temp":..,"target":..,"power":..,"fault":..}
//   GET /set?target=22.5    changes the target, answers like /status
//   GET /pwm?duty=50        changes the bulb power (0, 25, 50 or 100 %), answers like /status
//   GET /run                cycle state and parameters (Normal page)
//   GET /params?kp=8&...    changes the cycle parameters, answers like /run
//   GET /start, GET /stop   starts / stops the cycle, answers like /run
// Needs a WiFi connection, so call it after Wifi_Init().

#define WEB_CMD_NONE   0
#define WEB_CMD_START  1
#define WEB_CMD_STOP   2

// Times in seconds.
typedef struct {
  float tset;
  uint16_t tinc, tmen, trac;
  float kp, ki, kd;
} WebParams;

// Starts listening on port 80.
void Web_Init(void);

// Answers one waiting request, if there is one. Call it from loop().
void Web_Task(void);

// Target temperature set from the page, in degrees C (20 - 50).
float Web_GetTarget(void);

// Current temperature for /status, in degrees C. NAN makes the page show "--".
void Web_SetTemp(float t);

// Bulb power chosen on the page, in percent (0, 25, 50 or 100).
uint8_t Web_GetDuty(void);

// True if the parameters changed since the last call (also on the first call).
bool Web_TakeParams(WebParams *p);

// WEB_CMD_*, cleared once read.
uint8_t Web_TakeCommand(void);

// Cycle state for /run.
void Web_SetRun(const char *phase, bool running, float setpoint, uint8_t power, uint32_t elapsed);

#endif
