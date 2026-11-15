#ifndef HEATER_DRIVER_H
#define HEATER_DRIVER_H

#include <stdint.h>

// Heater driver: drives the bulb through the MOSFET module with PWM on pin 3.
// The rest of the system only talks to the heater through these functions.

// Sets up the PWM pin with the bulb off.
void Heater_Init(void);

// Requested power in percent (0 - 100). The output ramps there in Heater_Task().
void Heater_SetDuty(uint8_t percent);

// Call from loop(). Moves the output a small step toward the requested duty,
// so a cold bulb filament never sees a full-power jump (it would overload the 1 A supply).
void Heater_Task(void);

#endif
