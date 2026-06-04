#define BLYNK_TEMPLATE_ID   "TMPL3mwUJ8F7q"
#define BLYNK_TEMPLATE_NAME "Fault Tolerant Embedded Runtime System"
#define BLYNK_AUTH_TOKEN    "VyMXJD6QD0_RcGQ8OdrafVeiCkFk9pET"

#include <WiFi.h>
#include <BlynkSimpleEsp32.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

LiquidCrystal_I2C lcd(0x27, 16, 2);

char auth[] = BLYNK_AUTH_TOKEN;
char ssid[] = "Wokwi-GUEST";
char pass[] = "";

#define CELL1_PIN  32
#define CELL2_PIN  33
#define CELL3_PIN  34
#define CELL4_PIN  35

#define GREEN_LED  12
#define YELLOW_LED 13
#define RED_LED    14
#define BUZZER     15
#define RELAY      16

float cell[4];
float prev[4];
unsigned long freezeStart[4] = {0, 0, 0, 0};
bool  cellFrozen[4]          = {false, false, false, false};

String runtimeMode = "NORMAL";
String lastFault   = "NONE";

unsigned long faultCounter   = 0;
unsigned long bootTime       = 0;

unsigned long sensorTimer    = 0;
unsigned long lcdTimer       = 0;
unsigned long blynkTimer     = 0;
unsigned long buzzerTimer    = 0;
unsigned long recoveryTimer  = 0;

bool buzzerState    = false;
bool inRecovery     = false;
int  screen         = 0;

const unsigned long FREEZE_TIMEOUT   = 10000;
const unsigned long RECOVERY_HOLD    = 5000;

float readVoltage(int pin)
{
  long sum = 0;
  for (int i = 0; i < 8; i++) sum += analogRead(pin);
  float adcVolt = (sum / 8.0 / 4095.0) * 3.3;
  return adcVolt * 1.303;
}

void logFault(String fault)
{
  faultCounter++;
  lastFault = fault;
  Serial.print("[");
  Serial.print((millis() - bootTime) / 1000);
  Serial.print("s] FAULT: ");
  Serial.println(fault);
}

void setMode(String mode)
{
  if (runtimeMode != mode)
  {
    runtimeMode = mode;
    Serial.print("[");
    Serial.print((millis() - bootTime) / 1000);
    Serial.print("s] MODE -> ");
    Serial.println(mode);
  }
}

void pulseBuzzer(unsigned long now)
{
  if (now - buzzerTimer >= 250)
  {
    buzzerTimer = now;
    buzzerState = !buzzerState;
    digitalWrite(BUZZER, buzzerState);
  }
}

void lcdRow(int row, const char* fmt, ...)
{
  char buf[17];
  va_list args;
  va_start(args, fmt);
  vsnprintf(buf, sizeof(buf), fmt, args);
  va_end(args);
  char padded[17];
  snprintf(padded, sizeof(padded), "%-16s", buf);
  lcd.setCursor(0, row);
  lcd.print(padded);
}

void setup()
{
  Serial.begin(115200);
  analogReadResolution(12);
  Wire.begin(21, 22);

  lcd.init();
  lcd.backlight();

  pinMode(GREEN_LED,  OUTPUT);
  pinMode(YELLOW_LED, OUTPUT);
  pinMode(RED_LED,    OUTPUT);
  pinMode(BUZZER,     OUTPUT);
  pinMode(RELAY,      OUTPUT);

  digitalWrite(GREEN_LED,  LOW);
  digitalWrite(YELLOW_LED, LOW);
  digitalWrite(RED_LED,    LOW);
  digitalWrite(BUZZER,     LOW);
  digitalWrite(RELAY,      LOW);

  lcd.setCursor(0, 0); lcd.print("Runtime System  ");
  lcd.setCursor(0, 1); lcd.print("Initializing... ");

  Blynk.begin(auth, ssid, pass);
  bootTime = millis();
  delay(2000);

  lcd.setCursor(0, 0); lcd.print("                ");
  lcd.setCursor(0, 1); lcd.print("                ");
}

void loop()
{
  Blynk.run();

  unsigned long now = millis();

  if (now - sensorTimer >= 500)
  {
    sensorTimer = now;

    for (int i = 0; i < 4; i++) prev[i] = cell[i];

    int pins[4] = {CELL1_PIN, CELL2_PIN, CELL3_PIN, CELL4_PIN};
    for (int i = 0; i < 4; i++) cell[i] = readVoltage(pins[i]);

    bool sensorFault  = false;
    bool invalidFault = false;
    bool frozenFault  = false;
    bool relayFault   = false;

    for (int i = 0; i < 4; i++)
    {
      if (cell[i] < 0.1)
      {
        sensorFault = true;
        char msg[20];
        snprintf(msg, sizeof(msg), "Cell%d Disconnected", i + 1);
        logFault(String(msg));
      }

      if (cell[i] > 4.5)
      {
        invalidFault = true;
        char msg[20];
        snprintf(msg, sizeof(msg), "Cell%d Invalid ADC", i + 1);
        logFault(String(msg));
      }

      if (abs(cell[i] - prev[i]) < 0.001)
      {
        if (freezeStart[i] == 0) freezeStart[i] = now;
        if (now - freezeStart[i] > FREEZE_TIMEOUT)
        {
          cellFrozen[i] = true;
          frozenFault   = true;
          char msg[20];
          snprintf(msg, sizeof(msg), "ADC%d Frozen", i + 1);
          logFault(String(msg));
        }
      }
      else
      {
        freezeStart[i] = 0;
        cellFrozen[i]  = false;
      }
    }

    bool expectedRelay = (invalidFault || frozenFault);
    digitalWrite(RELAY, expectedRelay ? HIGH : LOW);
    bool actualRelay = digitalRead(RELAY);

    if (expectedRelay && !actualRelay)
    {
      relayFault = true;
      logFault("Relay Mismatch Detected");
    }

    if (runtimeMode != "SHUTDOWN")
    {
      if (relayFault)
      {
        setMode("SHUTDOWN");
        recoveryTimer = 0;
      }
      else if (invalidFault || frozenFault)
      {
        setMode("FAILSAFE");
        recoveryTimer = 0;
      }
      else if (sensorFault)
      {
        setMode("DEGRADED");
        recoveryTimer = 0;
      }
      else
      {
        if (runtimeMode != "NORMAL")
        {
          if (!inRecovery)
          {
            inRecovery    = true;
            recoveryTimer = now;
          }
          else if (now - recoveryTimer >= RECOVERY_HOLD)
          {
            setMode("NORMAL");
            inRecovery = false;
          }
        }
      }
    }

    digitalWrite(GREEN_LED,  LOW);
    digitalWrite(YELLOW_LED, LOW);
    digitalWrite(RED_LED,    LOW);

    if      (runtimeMode == "NORMAL")   digitalWrite(GREEN_LED,  HIGH);
    else if (runtimeMode == "DEGRADED") digitalWrite(YELLOW_LED, HIGH);
    else                                digitalWrite(RED_LED,    HIGH);

    if (runtimeMode == "NORMAL" || runtimeMode == "DEGRADED")
    {
      digitalWrite(BUZZER, LOW);
      buzzerState = false;
    }

    Serial.println("---------------");
    Serial.print("Mode   : "); Serial.println(runtimeMode);
    Serial.print("Faults : "); Serial.println(faultCounter);
    Serial.print("Last   : "); Serial.println(lastFault);
    for (int i = 0; i < 4; i++)
    {
      Serial.print("Cell"); Serial.print(i + 1);
      Serial.print("  : "); Serial.println(cell[i], 2);
    }
  }

  if (runtimeMode == "FAILSAFE" || runtimeMode == "SHUTDOWN")
    pulseBuzzer(now);

  if (now - lcdTimer >= 3000)
  {
    lcdTimer = now;

    if (runtimeMode == "FAILSAFE" || runtimeMode == "SHUTDOWN")
    {
      lcdRow(0, "!!! WARNING !!!");
      lcdRow(1, "%s", runtimeMode.c_str());
    }
    else
    {
      screen++;
      if (screen > 3) screen = 0;

      switch (screen)
      {
        case 0:
          lcdRow(0, "MODE: %s", runtimeMode.c_str());
          lcdRow(1, inRecovery ? "Recovering..." : "All OK");
          break;

        case 1:
          lcdRow(0, "Faults: %lu", faultCounter);
          lcdRow(1, "%s", lastFault.c_str());
          break;

        case 2:
          lcdRow(0, "C1:%.2f C2:%.2f", cell[0], cell[1]);
          lcdRow(1, "C3:%.2f C4:%.2f", cell[2], cell[3]);
          break;

        case 3:
          lcdRow(0, "Relay: %s", digitalRead(RELAY) ? "ON " : "OFF");
          lcdRow(1, "Up: %lus", (now - bootTime) / 1000);
          break;
      }
    }
  }

  if (now - blynkTimer >= 1000)
  {
    blynkTimer = now;

    Blynk.virtualWrite(V0, cell[0]);
    Blynk.virtualWrite(V1, cell[1]);
    Blynk.virtualWrite(V2, cell[2]);
    Blynk.virtualWrite(V3, cell[3]);
    Blynk.virtualWrite(V4, runtimeMode);
    Blynk.virtualWrite(V5, faultCounter);
    Blynk.virtualWrite(V6, digitalRead(RELAY) ? 1 : 0);
    Blynk.virtualWrite(V7, lastFault);
    Blynk.virtualWrite(V8, (now - bootTime) / 1000);
  }
}
