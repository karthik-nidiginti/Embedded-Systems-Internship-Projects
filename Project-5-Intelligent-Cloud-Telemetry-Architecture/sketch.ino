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

float voltage         = 0;
float previousVoltage = -1;

String currentState  = "NORMAL";
String previousState = "";

unsigned long sensorTimer = 0;
unsigned long wifiTimer   = 0;
unsigned long rssiTimer   = 0;
unsigned long bootTime;

int           signalRSSI   = 0;
unsigned long eventCounter = 0;

const float LOW_THRESHOLD  = 1.00;
const float HIGH_THRESHOLD = 2.80;

bool wifiWasDisconnected  = false;
bool blynkWasDisconnected = false;

const int MAX_QUEUE = 30;
String eventQueue[MAX_QUEUE];
int queueFront = 0;
int queueRear  = 0;
int queueSize  = 0;

float readVoltage()
{
  return analogRead(SENSOR_PIN) * 3.3 / 4095.0;
}

void setLED(bool connected)
{
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
  Serial.print("[EVENT] ");
  Serial.println(eventText);
}

void syncEventQueue()
{
  if (!Blynk.connected()) return;
  while (queueSize > 0)
  {
    Blynk.virtualWrite(V4, eventQueue[queueFront]);
    queueFront++;
    if (queueFront >= MAX_QUEUE) queueFront = 0;
    queueSize--;
  }
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

void connectWiFi()
{
  WiFi.begin(ssid, pass);
  Serial.print("Connecting WiFi");
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi Connected");
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
  if (v < LOW_THRESHOLD)  return "LOW_VOLTAGE";
  if (v > HIGH_THRESHOLD) return "HIGH_VOLTAGE";
  return "NORMAL";
}

void processTelemetry()
{
  if (millis() - sensorTimer < 1000) return;
  sensorTimer  = millis();
  voltage      = readVoltage();
  currentState = determineState(voltage);

  if (currentState != previousState)
  {
    enqueueEvent("STATE CHANGE -> " + currentState + " V=" + String(voltage, 2));
    if (Blynk.connected()) Blynk.virtualWrite(V1, currentState);
    previousState = currentState;
  }

  if (abs(voltage - previousVoltage) > 0.15)
  {
    enqueueEvent("VOLTAGE UPDATE -> " + String(voltage, 2) + "V");
    if (Blynk.connected()) Blynk.virtualWrite(V0, voltage);
    previousVoltage = voltage;
  }

  if (voltage < 0.05) enqueueEvent("Sensor Disconnected");
  if (voltage > 3.25) enqueueEvent("Voltage Anomaly");

  Serial.println("--------------------------------");
  Serial.print("Voltage : "); Serial.println(voltage);
  Serial.print("State   : "); Serial.println(currentState);
  Serial.print("RSSI    : "); Serial.println(signalRSSI);
  Serial.print("Queue   : "); Serial.println(queueSize);
}

void updateDashboard()
{
  static unsigned long dashboardTimer = 0;
  if (millis() - dashboardTimer < 5000) return;
  dashboardTimer = millis();
  if (!Blynk.connected()) return;

  Blynk.virtualWrite(V3, eventCounter);
  Blynk.virtualWrite(V5, queueSize);
  Blynk.virtualWrite(V6, 1);
  Blynk.virtualWrite(V7, (millis() - bootTime) / 1000);
}

void setup()
{
  Serial.begin(115200);
  pinMode(SENSOR_PIN, INPUT);
  pinMode(LED_GREEN,  OUTPUT);
  pinMode(LED_RED,    OUTPUT);

  setLED(false);

  connectWiFi();
  Blynk.config(auth);
  Blynk.connect();

  bootTime = millis();
  enqueueEvent("System Boot");
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
      syncEventQueue();
      postReconnectSync();
    }
    else
    {
      syncEventQueue();
    }
  }

  processTelemetry();
  monitorRSSI();
  updateDashboard();
}
