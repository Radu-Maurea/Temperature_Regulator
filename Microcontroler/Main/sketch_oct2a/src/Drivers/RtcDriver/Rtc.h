#ifndef RTC_H
#define RTC_H

#include <stdbool.h>
#include <stddef.h>

// Clock driver: gets the time from the internet (NTP) once at startup
// and keeps it in the board's built-in RTC.
// The rest of the system only talks to the clock through these functions.

// Starts the RTC and sets it from NTP. Call after Wi-Fi is connected.
// Returns false if NTP did not answer (the clock then starts from 00:00:00).
bool Rtc_Init(void);

// Call from loop(). When a new second has started, writes the time as
// "HH:MM:SS" into text (needs room for 9 chars) and returns true.
bool Rtc_Update(char *text, size_t len);

#endif
