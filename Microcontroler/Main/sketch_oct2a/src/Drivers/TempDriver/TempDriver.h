#ifndef TEMP_DRIVER_H
#define TEMP_DRIVER_H

// Temperature driver interface.
// The rest of the system only talks to the temperature sensor through these functions.
void Temp_Init(void);

float Temp_Read(void);

#endif
