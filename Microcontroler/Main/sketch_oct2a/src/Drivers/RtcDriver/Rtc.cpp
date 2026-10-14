#include "Rtc.h"
#include <RTC.h>        // RTC, RTCTime
#include <WiFiS3.h>     // WiFiUDP
#include <NTPClient.h>

// static: only this driver can touch the NTP client
static WiFiUDP udp;
static NTPClient timeClient(udp);

#define TZ_OFFSET    (3 * 3600)   // Romania summer time (UTC+3); winter is 2 * 3600
#define NTP_RETRIES  5

bool Rtc_Init(void)
{
    RTC.begin();
    timeClient.begin();

    // The first NTP request often gets no answer, so try a few times.
    bool ok = false;
    for (int i = 0; i < NTP_RETRIES && !ok; i++) {
        ok = timeClient.forceUpdate();
    }

    RTCTime t(ok ? timeClient.getEpochTime() + TZ_OFFSET : 0);
    RTC.setTime(t);
    return ok;
}

bool Rtc_Update(char *text, size_t len)
{
    static int lastSec = -1;
    RTCTime now;
    RTC.getTime(now);

    if (now.getSeconds() == lastSec) {
        return false;
    }
    lastSec = now.getSeconds();
    snprintf(text, len, "%02d:%02d:%02d", now.getHour(), now.getMinutes(), now.getSeconds());
    return true;
}
