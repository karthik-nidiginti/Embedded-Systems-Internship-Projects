#define BLYNK_TEMPLATE_ID "TMPL3mwUJ8F7q"
#define BLYNK_TEMPLATE_NAME "Fault Tolerant Embedded Runtime System"
#define BLYNK_AUTH_TOKEN "VyMXJD6QD0_RcGQ8OdrafVeiCkFk9pET"

#include <WiFi.h>
#include <BlynkSimpleEsp32.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

LiquidCrystal_I2C lcd(0x27, 16, 2);

char auth[] = BLYNK_AUTH_TOKEN;
char ssid[] = "Wokwi-GUEST";
char pass[] = "";

// Inputs
#define CELL1 32
#define CELL2 33
#define CELL3 34
#define CELL4 35

// Outputs
#define GREEN_LED 12
#define YELLOW_LED 13
#define RED_LED 14
#define BUZZER 15
#define RELAY 16

float cell1, cell2, cell3, cell4;
float prev1, prev2, prev3, prev4;

String runtimeMode = "NORMAL";
String lastFault = "NONE";

unsigned long sensorTimer = 0;
unsigned long lcdTimer = 0;

unsigned long freezeStart1 = 0;
unsigned long freezeStart2 = 0;
unsigned long freezeStart3 = 0;
unsigned long freezeStart4 = 0;

unsigned long faultCounter = 0;
unsigned long bootTime;

int screen = 0;

float readVoltage(int pin)
{
  return (analogRead(pin) / 4095.0) * 3.3;
}

void logFault(String fault)
{
  faultCounter++;
  lastFault = fault;

  Serial.print("[");
  Serial.print(millis() / 1000);
  Serial.print("s] ");

  Serial.println(fault);
}

void setup()
{
  Serial.begin(115200);

  pinMode(GREEN_LED, OUTPUT);
  pinMode(YELLOW_LED, OUTPUT);
  pinMode(RED_LED, OUTPUT);

  pinMode(BUZZER, OUTPUT);
  pinMode(RELAY, OUTPUT);

  Wire.begin(21, 22);

  lcd.init();
  lcd.backlight();

  lcd.print("Runtime System");

  Blynk.begin(auth, ssid, pass);

  bootTime = millis();
}

void loop()
{
  Blynk.run();

  unsigned long now = millis();

  if (now - sensorTimer >= 500)
  {
    sensorTimer = now;

    prev1 = cell1;
    prev2 = cell2;
    prev3 = cell3;
    prev4 = cell4;

    cell1 = readVoltage(CELL1);
    cell2 = readVoltage(CELL2);
    cell3 = readVoltage(CELL3);
    cell4 = readVoltage(CELL4);

    runtimeMode = "NORMAL";

    digitalWrite(GREEN_LED, LOW);
    digitalWrite(YELLOW_LED, LOW);
    digitalWrite(RED_LED, LOW);
    digitalWrite(BUZZER, LOW);

    bool sensorFault = false;
    bool frozenFault = false;
    bool relayFault = false;
    bool invalidFault = false;

    // Sensor Disconnection
    if (cell1 < 0.05 || cell2 < 0.05 ||
        cell3 < 0.05 || cell4 < 0.05)
    {
      sensorFault = true;
      runtimeMode = "DEGRADED";
      logFault("Sensor Disconnected");
    }

    // Invalid Reading
    if (cell1 > 3.3 || cell2 > 3.3 ||
        cell3 > 3.3 || cell4 > 3.3)
    {
      invalidFault = true;
      runtimeMode = "FAILSAFE";
      logFault("Invalid ADC Reading");
    }

    // Frozen ADC Detection
    if (abs(cell1 - prev1) < 0.001)
    {
      if (freezeStart1 == 0) freezeStart1 = now;

      if (now - freezeStart1 > 10000)
      {
        frozenFault = true;
        runtimeMode = "DEGRADED";
        logFault("ADC1 Frozen");
      }
    }
    else freezeStart1 = 0;

    if (abs(cell2 - prev2) < 0.001)
    {
      if (freezeStart2 == 0) freezeStart2 = now;

      if (now - freezeStart2 > 10000)
      {
        frozenFault = true;
        runtimeMode = "DEGRADED";
        logFault("ADC2 Frozen");
      }
    }
    else freezeStart2 = 0;

    // Simulated Relay Mismatch
    if (runtimeMode == "FAILSAFE")
    {
      digitalWrite(RELAY, HIGH);

      bool relayFeedback = false;

      if (!relayFeedback)
      {
        relayFault = true;
        runtimeMode = "SHUTDOWN";
        logFault("Relay Mismatch");
      }
    }
    else
    {
      digitalWrite(RELAY, LOW);
    }

    // Mode Indicators
    if (runtimeMode == "NORMAL")
    {
      digitalWrite(GREEN_LED, HIGH);
    }
    else if (runtimeMode == "DEGRADED")
    {
      digitalWrite(YELLOW_LED, HIGH);
    }
    else
    {
      digitalWrite(RED_LED, HIGH);
      digitalWrite(BUZZER, HIGH);
    }

    // Serial Output
    Serial.println("---------------");
    Serial.print("Mode: ");
    Serial.println(runtimeMode);

    Serial.print("C1: ");
    Serial.println(cell1);

    Serial.print("C2: ");
    Serial.println(cell2);

    Serial.print("C3: ");
    Serial.println(cell3);

    Serial.print("C4: ");
    Serial.println(cell4);
  }

  // LCD Screens
  if (now - lcdTimer >= 3000)
  {
    lcdTimer = now;

    screen++;

    if (screen > 3) screen = 0;

    lcd.clear();

    if (runtimeMode == "FAILSAFE" ||
        runtimeMode == "SHUTDOWN")
    {
      lcd.setCursor(0, 0);
      lcd.print("!!! WARNING !!!");

      lcd.setCursor(0, 1);
      lcd.print(runtimeMode);
    }
    else
    {
      switch (screen)
      {
        case 0:
          lcd.setCursor(0, 0);
          lcd.print("MODE:");
          lcd.print(runtimeMode);

          lcd.setCursor(0, 1);
          lcd.print("Healthy");
          break;

        case 1:
          lcd.setCursor(0, 0);
          lcd.print("Faults:");
          lcd.print(faultCounter);

          lcd.setCursor(0, 1);
          lcd.print(lastFault);
          break;

        case 2:
          lcd.setCursor(0, 0);
          lcd.print("ADC Runtime");

          lcd.setCursor(0, 1);
          lcd.print("Running");
          break;

        case 3:
          lcd.setCursor(0, 0);
          lcd.print("Relay:");

          if (digitalRead(RELAY))
            lcd.print("ON");
          else
            lcd.print("OFF");

          lcd.setCursor(0, 1);
          lcd.print(runtimeMode);
          break;
      }
    }
  }

  // Blynk
  Blynk.virtualWrite(V0, cell1);
  Blynk.virtualWrite(V1, cell2);
  Blynk.virtualWrite(V2, cell3);
  Blynk.virtualWrite(V3, cell4);

  Blynk.virtualWrite(V4, runtimeMode);
  Blynk.virtualWrite(V5, faultCounter);
  Blynk.virtualWrite(V6, digitalRead(RELAY));
  Blynk.virtualWrite(V7, lastFault);
  Blynk.virtualWrite(V8, (millis() - bootTime) / 1000);
}
