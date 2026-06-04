#define BLYNK_TEMPLATE_ID   "TMPL32k0Ff7sL"
#define BLYNK_TEMPLATE_NAME "Event Driven Safety Protection Kernel"
#define BLYNK_AUTH_TOKEN    "lR5ydJel4btC0d1C_PreFV0tUZvkTN4Y"

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

float cell1, cell2, cell3, cell4;
float prev1, prev2, prev3, prev4;

String systemState = "NORMAL";

bool relayState    = false;
bool relayArmed    = false;

int warningCount   = 0;

unsigned long sensorTimer = 0;
unsigned long lcdTimer    = 0;
unsigned long blynkTimer  = 0;
unsigned long relayTimer  = 0;

const unsigned long sensorInterval = 500;
const unsigned long lcdInterval    = 1000;
const unsigned long blynkInterval  = 1000;
const unsigned long relayHoldTime  = 3000;

float readVoltage(int pin)
{
  long sum = 0;
  for (int i = 0; i < 8; i++) sum += analogRead(pin);
  float adcVolt = (sum / 8.0 / 4095.0) * 3.3;
  return adcVolt * 1.303;
}

void setLEDs(bool green, bool yellow, bool red)
{
  digitalWrite(GREEN_LED,  green);
  digitalWrite(YELLOW_LED, yellow);
  digitalWrite(RED_LED,    red);
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

  lcd.setCursor(0, 0); lcd.print("Safety Kernel   ");
  lcd.setCursor(0, 1); lcd.print("Initializing... ");

  Blynk.begin(auth, ssid, pass);
  delay(2000);

  lcd.setCursor(0, 0); lcd.print("                ");
  lcd.setCursor(0, 1); lcd.print("                ");
}

void loop()
{
  Blynk.run();

  unsigned long now = millis();

  if (now - sensorTimer >= sensorInterval)
  {
    sensorTimer = now;

    prev1 = cell1;
    prev2 = cell2;
    prev3 = cell3;
    prev4 = cell4;

    cell1 = readVoltage(CELL1_PIN);
    cell2 = readVoltage(CELL2_PIN);
    cell3 = readVoltage(CELL3_PIN);
    cell4 = readVoltage(CELL4_PIN);

    bool weakCell         = (cell1 < 2.5 || cell2 < 2.5 || cell3 < 2.5 || cell4 < 2.5);
    bool overVoltage      = (cell1 > 4.2 || cell2 > 4.2 || cell3 > 4.2 || cell4 > 4.2);
    bool sensorFailure    = (cell1 < 0.1 || cell2 < 0.1 || cell3 < 0.1 || cell4 < 0.1);
    bool rapidFluctuation = (abs(cell1 - prev1) > 0.5 || abs(cell2 - prev2) > 0.5 ||
                             abs(cell3 - prev3) > 0.5 || abs(cell4 - prev4) > 0.5);

    bool criticalFault = (overVoltage || sensorFailure || rapidFluctuation);

    warningCount = 0;
    if (weakCell)         warningCount++;
    if (overVoltage)      warningCount++;
    if (sensorFailure)    warningCount++;
    if (rapidFluctuation) warningCount++;

    if (sensorFailure)
    {
      systemState = "SENSOR FAIL";
      setLEDs(false, false, true);
      digitalWrite(BUZZER, HIGH);
    }
    else if (overVoltage)
    {
      systemState = "OVERVOLTAGE";
      setLEDs(false, false, true);
      digitalWrite(BUZZER, HIGH);
    }
    else if (rapidFluctuation)
    {
      systemState = "FLUCTUATION";
      setLEDs(false, false, true);
      digitalWrite(BUZZER, HIGH);
    }
    else if (weakCell)
    {
      systemState = "LOW CELL";
      setLEDs(false, true, false);
      digitalWrite(BUZZER, LOW);
    }
    else
    {
      systemState = "NORMAL";
      setLEDs(true, false, false);
      digitalWrite(BUZZER, LOW);
    }

    if (criticalFault && !relayArmed)
    {
      relayTimer = now;
      relayArmed = true;
    }

    if (relayArmed && (now - relayTimer >= relayHoldTime))
    {
      relayState = true;
      digitalWrite(RELAY, HIGH);
    }

    if (!criticalFault && !weakCell)
    {
      if (relayState && (now - relayTimer >= relayHoldTime))
      {
        relayState = false;
        relayArmed = false;
        digitalWrite(RELAY, LOW);
      }
    }

    Serial.println("====================");
    Serial.print("Cell1 : "); Serial.println(cell1, 2);
    Serial.print("Cell2 : "); Serial.println(cell2, 2);
    Serial.print("Cell3 : "); Serial.println(cell3, 2);
    Serial.print("Cell4 : "); Serial.println(cell4, 2);
    Serial.print("State : "); Serial.println(systemState);
    Serial.print("Relay : "); Serial.println(relayState ? "ON" : "OFF");
    Serial.print("Warnings : "); Serial.println(warningCount);
  }

  if (now - lcdTimer >= lcdInterval)
  {
    lcdTimer = now;

    lcd.setCursor(0, 0);
    char line0[17];
    snprintf(line0, sizeof(line0), "%-16s", systemState.c_str());
    lcd.print(line0);

    lcd.setCursor(0, 1);
    char line1[17];
    snprintf(line1, sizeof(line1), "R:%-3s W:%d        ", relayState ? "ON" : "OFF", warningCount);
    lcd.print(line1);
  }

  if (now - blynkTimer >= blynkInterval)
  {
    blynkTimer = now;

    Blynk.virtualWrite(V0, cell1);
    Blynk.virtualWrite(V1, cell2);
    Blynk.virtualWrite(V2, cell3);
    Blynk.virtualWrite(V3, cell4);
    Blynk.virtualWrite(V4, systemState);
    Blynk.virtualWrite(V5, relayState ? 1 : 0);
    Blynk.virtualWrite(V6, warningCount);
  }
}
