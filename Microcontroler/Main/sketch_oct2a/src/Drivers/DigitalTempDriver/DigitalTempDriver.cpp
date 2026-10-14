#include "DigitalTempDriver.h"
#include <Arduino.h>
#include <OneWire.h>
#include <DallasTemperature.h>

#define DS18B20_PIN 2   // DQ pin of the module (it has its own 4.7k pull-up)

// static: only this driver can touch the sensor objects
static OneWire oneWire(DS18B20_PIN);
static DallasTemperature sensor(&oneWire);

void DigitalTemp_Init(void)
{
    sensor.begin();
    sensor.setResolution(12);              // 0.0625 °C steps, takes ~750 ms
    sensor.setWaitForConversion(false);    // don't block; the result is collected on the next call
    sensor.requestTemperatures();          // start the first conversion
}

float DigitalTemp_Read(void)
{
    // Called once a second, so the conversion started last time is done by now
    float temp = sensor.getTempCByIndex(0);
    sensor.requestTemperatures();          // start the next one

    // -127 means no sensor answered (wiring / pull-up); 85 is the power-on value before any conversion
    if(temp == DEVICE_DISCONNECTED_C || temp == 85.0f) return NAN;

    // DEBUG (remove once the readings look right)
    Serial.print("DS18B20 temp=");
    Serial.println(temp, 2);

    return temp;
}
