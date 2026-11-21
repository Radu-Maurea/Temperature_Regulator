#include "PidDriver.h"
#include <Arduino.h>

#define DT       1.0f     // seconds, one sensor reading per second
#define OUT_MIN  0.0f
#define OUT_MAX  100.0f

enum { IDLE, INC, MEN, RAC, DONE, ERR };
static const char *const PHASE_NAMES[] = {"idle", "inc", "men", "rac", "done", "err"};

static float kp, ki, kd;
static float tset;
static uint16_t tinc, tmen, trac;

static uint8_t phase = IDLE;
static unsigned long startMs;
static float startTemp;
static float setpoint = NAN;
static float iTerm;          // already multiplied by ki
static float lastTemp;
static float output = 0;

static float limit(float v)
{
    return constrain(v, OUT_MIN, OUT_MAX);
}

static void off(uint8_t p)
{
    phase = p;
    setpoint = NAN;
    output = 0;
}

// Phase and setpoint t seconds after Start.
static void followProfile(float t)
{
    if(t < tinc) {
        phase = INC;
        setpoint = startTemp + (tset - startTemp) * t / tinc;
    } else if(t < tinc + tmen) {
        phase = MEN;
        setpoint = tset;
    } else if(t < tinc + tmen + trac) {
        phase = RAC;
        setpoint = tset - (tset - startTemp) * (t - tinc - tmen) / trac;
    } else {
        off(DONE);
    }
}

void Pid_Init(void)
{
    off(IDLE);
}

void Pid_SetTunings(float p, float i, float d)
{
    if(p < 0 || i < 0 || d < 0) return;
    kp = p;
    ki = i;
    kd = d;
}

void Pid_SetProfile(float t, uint16_t inc, uint16_t men, uint16_t rac)
{
    if(Pid_IsRunning()) return;
    tset = t;
    tinc = inc;
    tmen = men;
    trac = rac;
}

void Pid_Start(float temp)
{
    if(Pid_IsRunning()) return;
    if(isnan(temp)) {
        off(ERR);
        return;
    }
    startMs = millis();
    startTemp = temp;
    lastTemp = temp;
    iTerm = 0;
    output = 0;
    followProfile(0);
}

void Pid_Stop(void)
{
    off(IDLE);
}

void Pid_Update(float temp)
{
    if(!Pid_IsRunning()) return;
    if(isnan(temp)) {
        off(ERR);
        return;
    }

    followProfile((millis() - startMs) / 1000.0f);
    if(!Pid_IsRunning()) return;

    float error = setpoint - temp;
    iTerm = limit(iTerm + ki * error * DT);       // anti-windup
    float dInput = (temp - lastTemp) / DT;        // on the measurement: no derivative kick
    lastTemp = temp;

    output = limit(kp * error + iTerm - kd * dInput);
}

bool Pid_IsRunning(void)
{
    return phase == INC || phase == MEN || phase == RAC;
}

const char *Pid_GetPhase(void)
{
    return PHASE_NAMES[phase];
}

float Pid_GetSetpoint(void)
{
    return setpoint;
}

uint8_t Pid_GetOutput(void)
{
    return (uint8_t)roundf(output);
}

uint32_t Pid_GetElapsed(void)
{
    return Pid_IsRunning() ? (millis() - startMs) / 1000UL : 0;
}
