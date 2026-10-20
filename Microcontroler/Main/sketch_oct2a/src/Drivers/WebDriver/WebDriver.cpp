#include "WebDriver.h"
#include "WebPage.h"

#include <Arduino.h>
#include <WiFiS3.h>
#include <string.h>

#define WEB_PORT        80
#define REQ_TIMEOUT_MS  2000UL   // drop a client that does not finish its request
#define CHUNK           512      // bytes per write; big writes can stall the WiFi modem
#define TARGET_MIN      20.0f
#define TARGET_MAX      50.0f

static WiFiServer server(WEB_PORT);
static float target = 22.0f;
static float temp = NAN;   // NAN until the first reading arrives

// Reads one line ("GET /status HTTP/1.1") into line, without the \r\n.
// Returns false if the client went quiet or disconnected first.
static bool readLine(WiFiClient &client, char *line, int len, unsigned long start)
{
  int n = 0;
  while (client.connected() && millis() - start < REQ_TIMEOUT_MS) {
    if (!client.available()) {
      continue;
    }
    char c = client.read();
    if (c == '\n') {
      line[n] = '\0';
      return true;
    }
    if (c != '\r' && n < len - 1) {
      line[n++] = c;
    }
  }
  return false;
}

// Every write is a separate command to the WiFi modem, so build the header first
// and send it in one go.
static void sendHeader(WiFiClient &client, const char *status, const char *type)
{
  char head[160];
  int n = snprintf(head, sizeof(head),
                   "HTTP/1.1 %s\r\n"
                   "Content-Type: %s\r\n"
                   "Cache-Control: no-store\r\n"
                   "Connection: close\r\n"
                   "\r\n",
                   status, type);
  client.write((const uint8_t *)head, n);
}

static void sendPage(WiFiClient &client)
{
  sendHeader(client, "200 OK", "text/html; charset=utf-8");
  const char *p = WEB_PAGE;
  size_t left = strlen(WEB_PAGE);
  while (left > 0 && client.connected()) {
    size_t n = left < CHUNK ? left : CHUNK;
    client.write((const uint8_t *)p, n);
    p += n;
    left -= n;
  }
}

// Writes v with one decimal ("-3.5"), or "null" for NAN.
// Printed as tenths so we don't depend on float support in snprintf.
static void printTenths(char *buf, int len, float v)
{
  if (isnan(v)) {
    snprintf(buf, len, "null");
    return;
  }
  int t10 = (int)roundf(fabsf(v) * 10.0f);
  snprintf(buf, len, "%s%d.%d", (v < 0 && t10 != 0) ? "-" : "", t10 / 10, t10 % 10);
}

static void sendStatus(WiFiClient &client)
{
  // No heater yet, so power is 0.
  char t[12], tg[12];
  printTenths(t, sizeof(t), temp);
  printTenths(tg, sizeof(tg), target);
  char body[80];
  snprintf(body, sizeof(body),
           "{\"temp\":%s,\"target\":%s,\"power\":0,\"fault\":null}", t, tg);
  sendHeader(client, "200 OK", "application/json");
  client.write((const uint8_t *)body, strlen(body));
}

// path looks like "/set?target=22.5". Ignores values that are not a number.
static void setTarget(const char *path)
{
  const char *arg = strstr(path, "target=");
  if (arg == NULL) {
    return;
  }
  char *end;
  float v = strtof(arg + 7, &end);
  if (end == arg + 7 || isnan(v)) {
    return;
  }
  v = constrain(v, TARGET_MIN, TARGET_MAX);
  target = roundf(v * 2.0f) / 2.0f;   // the page works in 0.5 degree steps
}

void Web_Init(void)
{
  server.begin();
  Serial.println("Web: server started on port 80");
}

void Web_Task(void)
{
  WiFiClient client = server.available();
  if (!client) {
    return;
  }

  unsigned long start = millis();
  char line[128];
  if (!readLine(client, line, sizeof(line), start)) {
    client.stop();
    return;
  }

  // Skip the rest of the headers, up to the empty line.
  char header[128];
  while (readLine(client, header, sizeof(header), start) && header[0] != '\0') {
  }

  // line = "GET /path HTTP/1.1" -> cut out "/path"
  char *path = strchr(line, ' ');
  char *end  = path ? strchr(path + 1, ' ') : NULL;
  if (strncmp(line, "GET ", 4) != 0 || end == NULL) {
    sendHeader(client, "400 Bad Request", "text/plain");
    client.stop();
    return;
  }
  path++;
  *end = '\0';

  if (strcmp(path, "/") == 0 || strcmp(path, "/index.html") == 0) {
    sendPage(client);
  } else if (strcmp(path, "/status") == 0) {
    sendStatus(client);
  } else if (strncmp(path, "/set?", 5) == 0) {
    setTarget(path);
    sendStatus(client);
  } else {
    sendHeader(client, "404 Not Found", "text/plain");
  }

  client.flush();
  client.stop();
}

float Web_GetTarget(void)
{
  return target;
}

void Web_SetTemp(float t)
{
  temp = t;
}
