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
#define TIME_MAX        5999.0f // seconds
#define KP_MAX          99.9f
#define KI_MAX          9.99f
#define KD_MAX          99.9f

static WiFiServer server(WEB_PORT);
static float target = 22.0f;
static float temp = NAN;     // NAN until the first reading arrives
static uint8_t duty = 0;     // bulb power in percent

static WebParams params = {50.0f, 60, 20, 50, 8.0f, 0.05f, 20.0f};
static bool paramsNew = true;
static uint8_t command = WEB_CMD_NONE;

static const char *runPhase = "idle";
static bool running = false;
static float runSetpoint = NAN;
static uint8_t runPower = 0;
static uint32_t runElapsed = 0;

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

// Writes v with dec decimals ("-3.5"), or "null" for NAN.
// Printed as integers so we don't depend on float support in snprintf.
static void printFixed(char *buf, int len, float v, int dec)
{
  if (isnan(v)) {
    snprintf(buf, len, "null");
    return;
  }
  int scale = (dec == 2) ? 100 : 10;
  int n = (int)roundf(fabsf(v) * scale);
  snprintf(buf, len, "%s%d.%0*d", (v < 0 && n != 0) ? "-" : "", n / scale, dec, n % scale);
}

static void sendStatus(WiFiClient &client)
{
  char t[12], tg[12];
  printFixed(t, sizeof(t), temp, 1);
  printFixed(tg, sizeof(tg), target, 1);
  char body[80];
  snprintf(body, sizeof(body),
           "{\"temp\":%s,\"target\":%s,\"power\":%u,\"fault\":null}", t, tg, duty);
  sendHeader(client, "200 OK", "application/json");
  client.write((const uint8_t *)body, strlen(body));
}

static void sendRun(WiFiClient &client)
{
  char t[12], sp[12], ts[12], kp[12], ki[12], kd[12];
  printFixed(t, sizeof(t), temp, 1);
  printFixed(sp, sizeof(sp), runSetpoint, 1);
  printFixed(ts, sizeof(ts), params.tset, 1);
  printFixed(kp, sizeof(kp), params.kp, 1);
  printFixed(ki, sizeof(ki), params.ki, 2);
  printFixed(kd, sizeof(kd), params.kd, 1);
  char body[256];
  snprintf(body, sizeof(body),
           "{\"phase\":\"%s\",\"sp\":%s,\"temp\":%s,\"power\":%u,\"elapsed\":%lu,"
           "\"tset\":%s,\"tinc\":%u,\"tmen\":%u,\"trac\":%u,\"kp\":%s,\"ki\":%s,\"kd\":%s}",
           runPhase, sp, t, runPower, (unsigned long)runElapsed,
           ts, params.tinc, params.tmen, params.trac, kp, ki, kd);
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

// path looks like "/pwm?duty=50". Only the levels the page offers are accepted.
static void setDuty(const char *path)
{
  const char *arg = strstr(path, "duty=");
  if (arg == NULL) {
    return;
  }
  int v = atoi(arg + 5);
  if (v == 0 || v == 25 || v == 50 || v == 100) {
    duty = (uint8_t)v;
  }
}

// Number after "name=" in the query; false if missing or not a number.
static bool readArg(const char *path, const char *name, float *v)
{
  int n = strlen(name);
  for (const char *q = strchr(path, '?'); q != NULL; q = strchr(q + 1, '&')) {
    if (strncmp(q + 1, name, n) != 0 || q[1 + n] != '=') {
      continue;
    }
    char *end;
    float x = strtof(q + 2 + n, &end);
    if (end == q + 2 + n || isnan(x)) {
      return false;
    }
    *v = x;
    return true;
  }
  return false;
}

// Limits v to lo..hi and rounds it to the nearest step.
static float toStep(float v, float lo, float hi, float step)
{
  return roundf(constrain(v, lo, hi) / step) * step;
}

// path looks like "/params?tset=50&kp=8". TSET and the times are locked while a cycle runs.
static void setParams(const char *path)
{
  float v;
  if (!running) {
    if (readArg(path, "tset", &v)) params.tset = toStep(v, TARGET_MIN, TARGET_MAX, 0.5f);
    if (readArg(path, "tinc", &v)) params.tinc = (uint16_t)toStep(v, 0.0f, TIME_MAX, 1.0f);
    if (readArg(path, "tmen", &v)) params.tmen = (uint16_t)toStep(v, 0.0f, TIME_MAX, 1.0f);
    if (readArg(path, "trac", &v)) params.trac = (uint16_t)toStep(v, 0.0f, TIME_MAX, 1.0f);
  }
  if (readArg(path, "kp", &v)) params.kp = toStep(v, 0.0f, KP_MAX, 0.1f);
  if (readArg(path, "ki", &v)) params.ki = toStep(v, 0.0f, KI_MAX, 0.01f);
  if (readArg(path, "kd", &v)) params.kd = toStep(v, 0.0f, KD_MAX, 0.1f);
  paramsNew = true;
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
  } else if (strncmp(path, "/pwm?", 5) == 0) {
    setDuty(path);
    sendStatus(client);
  } else if (strcmp(path, "/run") == 0) {
    sendRun(client);
  } else if (strncmp(path, "/params?", 8) == 0) {
    setParams(path);
    sendRun(client);
  } else if (strcmp(path, "/start") == 0) {
    command = WEB_CMD_START;
    duty = 0;       // bulb stays off after the cycle
    sendRun(client);
  } else if (strcmp(path, "/stop") == 0) {
    command = WEB_CMD_STOP;
    sendRun(client);
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

uint8_t Web_GetDuty(void)
{
  return duty;
}

bool Web_TakeParams(WebParams *p)
{
  *p = params;
  bool changed = paramsNew;
  paramsNew = false;
  return changed;
}

uint8_t Web_TakeCommand(void)
{
  uint8_t c = command;
  command = WEB_CMD_NONE;
  return c;
}

void Web_SetRun(const char *phase, bool isRunning, float setpoint, uint8_t power, uint32_t elapsed)
{
  runPhase = phase;
  running = isRunning;
  runSetpoint = setpoint;
  runPower = power;
  runElapsed = elapsed;
}
