#include "HeaterDriver.h"
#include <Arduino.h>

#define HEATER_PIN      3      // SIG pin of the MOSFET module
#define RAMP_STEP       5      // PWM counts (of 255) per step
#define RAMP_PERIOD_MS  10UL   // 0 -> 255 takes ~0.5 s

static uint8_t wanted = 0;     // requested PWM value, 0 - 255
static uint8_t output = 0;     // value currently on the pin

void Heater_Init(void)
{
    pinMode(HEATER_PIN, OUTPUT);
    analogWrite(HEATER_PIN, 0);
}

void Heater_SetDuty(uint8_t percent)
{
    if(percent > 100) percent = 100;
    wanted = (uint8_t)((percent * 255U + 50U) / 100U);
}

void Heater_Task(void)
{
    static unsigned long lastStep = 0;
    if(output == wanted || millis() - lastStep < RAMP_PERIOD_MS) return;
    lastStep = millis();

    // Only ramp up; turning down can go straight to the new value
    if(wanted > output) output = (wanted - output > RAMP_STEP) ? output + RAMP_STEP : wanted;
    else                output = wanted;

    analogWrite(HEATER_PIN, output);
}
