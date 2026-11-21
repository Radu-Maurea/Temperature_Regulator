#ifndef PID_DRIVER_H
#define PID_DRIVER_H

#include <stdint.h>

// PID regulator that follows the heat -> hold -> cool profile. Output: bulb power 0 - 100 %.

void Pid_Init(void);

// Can change during a cycle without an output jump.
void Pid_SetTunings(float kp, float ki, float kd);

// TSET in degrees C, times in seconds. Ignored while a cycle runs.
void Pid_SetProfile(float tset, uint16_t tinc, uint16_t tmen, uint16_t trac);

// A NAN temperature does not start the cycle (phase "err").
void Pid_Start(float temp);

void Pid_Stop(void);

// Call once a second. A NAN reading stops the cycle (phase "err").
void Pid_Update(float temp);

bool Pid_IsRunning(void);

// "idle", "inc", "men", "rac", "done" or "err".
const char *Pid_GetPhase(void);

// NAN when no cycle runs.
float Pid_GetSetpoint(void);

uint8_t Pid_GetOutput(void);

// Seconds since Start, 0 when no cycle runs.
uint32_t Pid_GetElapsed(void);

#endif
