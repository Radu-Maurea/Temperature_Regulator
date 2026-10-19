#ifndef DIGITAL_TEMP_DRIVER_H
#define DIGITAL_TEMP_DRIVER_H

// DS18B20 digital temperature sensor (OneWire), the main temperature sensor.
// The LM35 driver (TempDriver) is kept but not used for now.
// The rest of the system only talks to the sensor through these functions.
void DigitalTemp_Init(void);

// Returns the last finished reading (degrees C) and starts the next one.
// Never waits for the sensor, so it does not block WiFi or the display.
// Returns NAN if the sensor is missing or the reading failed.
float DigitalTemp_Read(void);

#endif
