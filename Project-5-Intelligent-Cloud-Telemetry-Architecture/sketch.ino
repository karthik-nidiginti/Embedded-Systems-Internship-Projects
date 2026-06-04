#define BLYNK_TEMPLATE_ID   "TMPL39ce99WWV"
#define BLYNK_TEMPLATE_NAME "Intelligent Cloud Telemetry Architecture"
#define BLYNK_AUTH_TOKEN    "oAS5giQwGYVcZtP1IIeK8xD1drL3BqjO"

#include <WiFi.h>
#include <BlynkSimpleEsp32.h>

char auth[] = BLYNK_AUTH_TOKEN;
char ssid[] = "Wokwi-GUEST";
char pass[] = "";

#define SENSOR_PIN  32
#define LED_GREEN   25
#define LED_RED     26

const float LOW_THRESHOLD   = 2.5;
const float HIGH_THRESHOLD  = 4.2;
const float ANOMALY_MAX     = 4.5;
const float DISCONNECT_MIN  = 0.1;
const float VOLTAGE_DELTA   = 0.15;

float voltage         = 0;
float previousVoltage = -1;

String currentState  = "NORMAL";
String previousState = "";

bool prevLedState           = false;
bool wifiWasDisconnected    = false;
bool blynkWasDisconnected   = false;
bool sensorWasDisconnected  = false;

unsigned long sensorTimer    = 0;
unsigned long wifiTimer      = 0;
unsigned long rssiTimer      = 0;
unsigned long dashboardTimer = 0;
unsigned long queueTimer     = 0;
unsigned long bootTime       = 0;

int           signalRSSI   = 0;
unsigned long eventCounter = 0;

const int MAX_QUEUE = 30;
String    eventQueue[MAX_QUEUE];
int       queueFront = 0;
int       queueRear  = 0;
int       queueSize  = 0;

float readVoltage()
{
  long sum = 0;
  for (int i = 0; i < 8; i++) sum += analogRead(SENSOR_PIN);
  float adcVolt = (sum / 8.0 / 4095.0) * 3.3;
  return adcVolt * 1.303;
}

void setLED(bool connected)
{
  if (connected == prevLedState) return;
  prevLedState = connected;
  digitalWrite(LED_GREEN, connected ? HIGH : LOW);
  digitalWrite(LED_RED,   connected ? LOW  : HIGH);
}

void enqueueEvent(String eventText)
{
  if (queueSize >= MAX_QUEUE)
  {
    Serial.println("[WARN] Queue full - dropped: " + eventText);
    return;
  }
  eventQueue[queueRear] = eventText;
  queueRear++;
  if (queueRear >= MAX_QUEUE) queueRear = 0;
  queueSize++;
  eventCounter++;

  Serial.print("[");
  Serial.print((millis() - bootTime) / 1000);
  Serial.print("s] EVENT: ");
  Serial.println(eventText);
}

void drainOneEvent()
{
  if (queueSize == 0) return;
  if (!Blynk.connected()) return;

  if (millis() - queueTimer < 200) return;
  queueTimer = millis();

  Blynk.virtualWrite(V4, eventQueue[queueFront]);
  queueFront++;
  if (queueFront >= MAX_QUEUE) queueFront = 0;
  queueSize--;
}

void postReconnectSync()
{
  Blynk.virtualWrite(V0, voltage);
  Blynk.virtualWrite(V1, currentState);
  Blynk.virtualWrite(V2, signalRSSI);
  Blynk.virtualWrite(V3, eventCounter);
  Blynk.virtualWrite(V5, queueSize);
  Blynk.virtualWrite(V6, 1);
  Blynk.virtualWrite(V7, (millis() - bootTime) / 1000);
}

void checkWiFi()
{
  if (millis() - wifiTimer < 5000) return;
  wifiTimer = millis();

  if (WiFi.status() != WL_CONNECTED)
  {
    if (!wifiWasDisconnected)
    {
      enqueueEvent("WiFi Disconnected");
      wifiWasDisconnected = true;
    }
    WiFi.disconnect();
    WiFi.begin(ssid, pass);
  }
  else
  {
    if (wifiWasDisconnected)
    {
      enqueueEvent("WiFi Reconnected RSSI=" + String(WiFi.RSSI()));
      wifiWasDisconnected = false;
    }
  }
}

void monitorRSSI()
{
  if (millis() - rssiTimer < 10000) return;
  rssiTimer  = millis();
  signalRSSI = WiFi.RSSI();

  if (signalRSSI < -80)
    enqueueEvent("Weak Signal RSSI=" + String(signalRSSI));

  if (Blynk.connected())
    Blynk.virtualWrite(V2, signalRSSI);
}

String determineState(float v)
{
  if (v < DISCONNECT_MIN)  return "SENSOR_FAULT";
  if (v < LOW_THRESHOLD)   return "LOW_VOLTAGE";
  if (v > HIGH_THRESHOLD)  return "HIGH_VOLTAGE";
  return "NORMAL";
}

void processTelemetry()
{
  if (millis() - sensorTimer < 1000) return;
  sensorTimer = millis();

  voltage      = readVoltage();
  currentState = determineState(voltage);

  if (voltage < DISCONNECT_MIN)
  {
    if (!sensorWasDisconnected)
    {
      enqueueEvent("Sensor Disconnected");
      sensorWasDisconnected = true;
    }
  }
  else
  {
    if (sensorWasDisconnected)
    {
      enqueueEvent("Sensor Reconnected");
      sensorWasDisconnected = false;
    }
  }

  if (voltage > ANOMALY_MAX)
    enqueueEvent("Voltage Anomaly >" + String(ANOMALY_MAX) + "V");

  if (currentState != previousState)
  {
    enqueueEvent("STATE -> " + currentState + " V=" + String(voltage, 2));
    if (Blynk.connected()) Blynk.virtualWrite(V1, currentState);
    previousState = currentState;
  }

  if (abs(voltage - previousVoltage) > VOLTAGE_DELTA)
  {
    enqueueEvent("VOLTAGE -> " + String(voltage, 2) + "V");
    if (Blynk.connected()) Blynk.virtualWrite(V0, voltage);
    previousVoltage = voltage;
  }

  Serial.println("--------------------------------");
  Serial.print("Voltage : "); Serial.println(voltage, 2);
  Serial.print("State   : "); Serial.println(currentState);
  Serial.print("RSSI    : "); Serial.println(signalRSSI);
  Serial.print("Queue   : "); Serial.println(queueSize);
  Serial.print("Events  : "); Serial.println(eventCounter);
}

void updateDashboard()
{
  if (millis() - dashboardTimer < 5000) return;
  dashboardTimer = millis();
  if (!Blynk.connected()) return;

  Blynk.virtualWrite(V3, eventCounter);
  Blynk.virtualWrite(V5, queueSize);
  Blynk.virtualWrite(V6, Blynk.connected() ? 1 : 0);
  Blynk.virtualWrite(V7, (millis() - bootTime) / 1000);
}

void setup()
{
  Serial.begin(115200);
  analogReadResolution(12);

  pinMode(SENSOR_PIN, INPUT);
  pinMode(LED_GREEN,  OUTPUT);
  pinMode(LED_RED,    OUTPUT);

  setLED(false);

  WiFi.begin(ssid, pass);
  Serial.print("Connecting WiFi");
  unsigned long wifiStart = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - wifiStart < 10000)
  {
    delay(200);
    Serial.print(".");
  }
  Serial.println(WiFi.status() == WL_CONNECTED ? "\nWiFi Connected" : "\nWiFi Timeout - continuing");

  Blynk.config(auth);
  Blynk.connect(3000);

  bootTime = millis();
  enqueueEvent("System Boot v2.0");
}

void loop()
{
  Blynk.run();
  checkWiFi();

  if (!Blynk.connected())
  {
    setLED(false);
    if (!blynkWasDisconnected)
    {
      enqueueEvent("Blynk Disconnected");
      blynkWasDisconnected = true;
    }
    Blynk.connect(1000);
  }
  else
  {
    setLED(true);
    if (blynkWasDisconnected)
    {
      enqueueEvent("Blynk Reconnected");
      blynkWasDisconnected = false;
      postReconnectSync();
    }
  }

  processTelemetry();
  monitorRSSI();
  updateDashboard();
  drainOneEvent();
}
