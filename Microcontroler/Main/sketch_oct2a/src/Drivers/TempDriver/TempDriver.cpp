#include "TempDriver.h"
#include <Arduino.h>

#define LM35_PIN A1
#define USE_INTERNAL_REF 0   // 1 = internal 1.5V reference, 0 = default 5V reference
#if USE_INTERNAL_REF
#define VREF 1.5f
#else
#define VREF 4.69f   // measured: USB "5V" is lower than 5V (3.3V pin read as 3.52V)
#endif
#define ADC_MAX 16383.0f

#define SAMPLES 32        // readings per Temp_Read()
#define SMOOTH 0.2f       // weight of a new reading in the running average (0..1)

static float smoothed = NAN;

void Temp_Init(void)
{
    analogReadResolution(14);
#if USE_INTERNAL_REF
    analogReference(AR_INTERNAL);   // UNO R4: internal 1.5V reference, must match VREF
#endif
}

float Temp_Read(void)
{
    int s[SAMPLES];

    analogRead(LM35_PIN);   // throw away the first reading after switching channel
    for(int i=0;i<SAMPLES;i++)
    {
        s[i] = analogRead(LM35_PIN);
        delayMicroseconds(200);   // spread samples out so they don't all catch the same noise
    }

    // Sort, then average only the middle values so single spikes are ignored
    for(int i=1;i<SAMPLES;i++)
    {
        int v = s[i];
        int j = i-1;
        while(j>=0 && s[j]>v) { s[j+1] = s[j]; j--; }
        s[j+1] = v;
    }
    // A reading of 0 means the signal dropped out (0V = 0°C, impossible indoors), so skip those
    int first = 0;
    while(first<SAMPLES && s[first]==0) first++;
    int valid = SAMPLES-first;
    if(valid==0) return smoothed;   // whole burst dropped out, keep the last value

    // Average the middle half of the valid readings so remaining spikes are ignored
    int trim = valid/4;
    long sum = 0;
    for(int i=first+trim;i<SAMPLES-trim;i++)
    {
        sum += s[i];
    }
    float raw = sum/(float)(valid-2*trim);
    float temp = raw*VREF/ADC_MAX*100.0f;

    // Running average across calls so the display doesn't jump around
    if(isnan(smoothed)) smoothed = temp;
    else smoothed += SMOOTH*(temp - smoothed);

    // DEBUG (remove once the sensor reads correctly)
    Serial.print("LM35 zeros=");
    Serial.print(first);
    Serial.print(" min=");
    Serial.print(s[first]);
    Serial.print(" max=");
    Serial.print(s[SAMPLES-1]);
    Serial.print(" temp=");
    Serial.print(temp, 1);
    Serial.print(" smoothed=");
    Serial.println(smoothed, 1);

    return smoothed;
}
