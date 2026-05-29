#define BLYNK_TEMPLATE_ID "TMPL32k0Ff7sL"
#define BLYNK_TEMPLATE_NAME "Event Driven Safety Protection Kernel"
#define BLYNK_AUTH_TOKEN "lR5ydJel4btC0d1C_PreFV0tUZvkTN4Y"

#include <WiFi.h>
#include <BlynkSimpleEsp32.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

LiquidCrystal_I2C lcd(0x27, 16, 2);

char auth[] = BLYNK_AUTH_TOKEN;
char ssid[] = "Wokwi-GUEST";
char pass[] = "";

// Cell Inputs
#define CELL1 32
#define CELL2 33
#define CELL3 34
#define CELL4 35

// Outputs
#define GREEN_LED   12
#define YELLOW_LED  13
#define RED_LED     14

#define BUZZER      15
#define RELAY       16

// Variables
float cell1, cell2, cell3, cell4;
float prev1, prev2, prev3, prev4;

String systemState = "NORMAL";

bool relayState = LOW;

int warningCount = 0;

// millis timers
unsigned long sensorTimer = 0;
unsigned long lcdTimer = 0;
unsigned long relayTimer = 0;

const unsigned long sensorInterval = 500;
const unsigned long lcdInterval = 1000;
const unsigned long relayDelay = 3000;

// Read Voltage
float readVoltage(int pin)
{
  int adc = analogRead(pin);
  return (adc / 4095.0) * 3.3;
}

void setup()
{
  Serial.begin(115200);

  analogReadResolution(12);

  Wire.begin(21, 22);

  lcd.init();
  lcd.backlight();

  pinMode(GREEN_LED, OUTPUT);
  pinMode(YELLOW_LED, OUTPUT);
  pinMode(RED_LED, OUTPUT);

  pinMode(BUZZER, OUTPUT);
  pinMode(RELAY, OUTPUT);

  digitalWrite(RELAY, LOW);

  lcd.setCursor(0, 0);
  lcd.print("Safety Kernel");

  lcd.setCursor(0, 1);
  lcd.print("Initializing");

  Blynk.begin(auth, ssid, pass);

  delay(2000);

  lcd.clear();
}

void loop()
{
  Blynk.run();

  unsigned long currentMillis = millis();

  // Sensor Event
  if (currentMillis - sensorTimer >= sensorInterval)
  {
    sensorTimer = currentMillis;

    prev1 = cell1;
    prev2 = cell2;
    prev3 = cell3;
    prev4 = cell4;

    cell1 = readVoltage(CELL1);
    cell2 = readVoltage(CELL2);
    cell3 = readVoltage(CELL3);
    cell4 = readVoltage(CELL4);

    bool weakCell =
      (cell1 < 1.5 || cell2 < 1.5 ||
       cell3 < 1.5 || cell4 < 1.5);

    bool overVoltage =
      (cell1 > 3.0 || cell2 > 3.0 ||
       cell3 > 3.0 || cell4 > 3.0);

    bool sensorFailure =
      (cell1 <= 0 || cell2 <= 0 ||
       cell3 <= 0 || cell4 <= 0);

    bool rapidFluctuation =
      (abs(cell1 - prev1) > 0.5 ||
       abs(cell2 - prev2) > 0.5 ||
       abs(cell3 - prev3) > 0.5 ||
       abs(cell4 - prev4) > 0.5);

    // Reset Outputs
    digitalWrite(GREEN_LED, LOW);
    digitalWrite(YELLOW_LED, LOW);
    digitalWrite(RED_LED, LOW);

    digitalWrite(BUZZER, LOW);

    // Default State
    systemState = "NORMAL";

    digitalWrite(GREEN_LED, HIGH);

    // Weak Cell
    if (weakCell)
    {
      systemState = "LOW CELL";

      digitalWrite(YELLOW_LED, HIGH);

      warningCount++;
    }

    // Over Voltage
    if (overVoltage)
    {
      systemState = "OVERVOLTAGE";

      digitalWrite(RED_LED, HIGH);
      digitalWrite(BUZZER, HIGH);

      warningCount++;
    }

    // Sensor Failure
    if (sensorFailure)
    {
      systemState = "SENSOR FAIL";

      digitalWrite(RED_LED, HIGH);
      digitalWrite(BUZZER, HIGH);

      warningCount++;
    }

    // Rapid Fluctuation
    if (rapidFluctuation)
    {
      systemState = "FLUCTUATION";

      digitalWrite(RED_LED, HIGH);
      digitalWrite(BUZZER, HIGH);

      warningCount++;
    }

    // Relay Protection
    if ((overVoltage || sensorFailure || rapidFluctuation)
        && (currentMillis - relayTimer >= relayDelay))
    {
      relayTimer = currentMillis;

      relayState = HIGH;

      digitalWrite(RELAY, HIGH);
    }

    // Recovery Logic
    if (!weakCell &&
        !overVoltage &&
        !sensorFailure &&
        !rapidFluctuation)
    {
      systemState = "NORMAL";

      relayState = LOW;

      digitalWrite(RELAY, LOW);

      digitalWrite(GREEN_LED, HIGH);
    }

    // Serial Monitor
    Serial.println("\n====================");

    Serial.print("Cell1 : ");
    Serial.println(cell1);

    Serial.print("Cell2 : ");
    Serial.println(cell2);

    Serial.print("Cell3 : ");
    Serial.println(cell3);

    Serial.print("Cell4 : ");
    Serial.println(cell4);

    Serial.print("State : ");
    Serial.println(systemState);

    Serial.print("Relay : ");
    Serial.println(relayState);

    Serial.print("Warnings : ");
    Serial.println(warningCount);
  }

  // LCD Event
  if (currentMillis - lcdTimer >= lcdInterval)
  {
    lcdTimer = currentMillis;

    lcd.clear();

    lcd.setCursor(0, 0);
    lcd.print(systemState);

    lcd.setCursor(0, 1);
    lcd.print("R:");
    lcd.print(relayState);

    lcd.print(" W:");
    lcd.print(warningCount);
  }

  // Blynk Update
  Blynk.virtualWrite(V0, cell1);
  Blynk.virtualWrite(V1, cell2);
  Blynk.virtualWrite(V2, cell3);
  Blynk.virtualWrite(V3, cell4);

  Blynk.virtualWrite(V4, systemState);

  Blynk.virtualWrite(V5, relayState);

  Blynk.virtualWrite(V6, warningCount);
}
