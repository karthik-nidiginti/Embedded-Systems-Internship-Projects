#define BLYNK_TEMPLATE_ID "TMPL3iD_qUOVk"
#define BLYNK_TEMPLATE_NAME "Intelligent Embedded HMI and Diagnostic Interface"
#define BLYNK_AUTH_TOKEN "LHQgvaIjitfP1h-yZSx3nfN_SpmrmX5m"

#include <WiFi.h>
#include <BlynkSimpleEsp32.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

LiquidCrystal_I2C lcd(0x27, 16, 2);

char auth[] = BLYNK_AUTH_TOKEN;
char ssid[] = "Wokwi-GUEST";
char pass[] = "";

// Analog Inputs
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

float packAverage;
float imbalancePercent;

float highestVoltage;
float lowestVoltage;

String healthStatus = "";

int strongestCell = 0;
int weakestCell = 0;

bool criticalFault = false;

// Timers
unsigned long sensorTimer = 0;
unsigned long screenTimer = 0;

const unsigned long sensorInterval = 500;
const unsigned long screenInterval = 2500;

int currentScreen = 0;

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
  lcd.print("Smart HMI");
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

  // Sensor Reading
  if (currentMillis - sensorTimer >= sensorInterval)
  {
    sensorTimer = currentMillis;

    cell1 = readVoltage(CELL1);
    cell2 = readVoltage(CELL2);
    cell3 = readVoltage(CELL3);
    cell4 = readVoltage(CELL4);

    packAverage =
      (cell1 + cell2 + cell3 + cell4) / 4.0;

    highestVoltage =
      max(max(cell1, cell2), max(cell3, cell4));

    lowestVoltage =
      min(min(cell1, cell2), min(cell3, cell4));

    imbalancePercent =
      ((highestVoltage - lowestVoltage) /
       packAverage) * 100;

    // Strongest Cell
    if (highestVoltage == cell1) strongestCell = 1;
    else if (highestVoltage == cell2) strongestCell = 2;
    else if (highestVoltage == cell3) strongestCell = 3;
    else strongestCell = 4;

    // Weakest Cell
    if (lowestVoltage == cell1) weakestCell = 1;
    else if (lowestVoltage == cell2) weakestCell = 2;
    else if (lowestVoltage == cell3) weakestCell = 3;
    else weakestCell = 4;

    // Reset Outputs
    digitalWrite(GREEN_LED, LOW);
    digitalWrite(YELLOW_LED, LOW);
    digitalWrite(RED_LED, LOW);

    digitalWrite(BUZZER, LOW);

    criticalFault = false;

    // Health Status
    if (imbalancePercent < 5)
    {
      healthStatus = "HEALTHY";

      digitalWrite(GREEN_LED, HIGH);

      digitalWrite(RELAY, LOW);
    }
    else if (imbalancePercent < 20)
    {
      healthStatus = "MINOR";

      digitalWrite(YELLOW_LED, HIGH);

      digitalWrite(RELAY, LOW);
    }
    else if (imbalancePercent < 40)
    {
      healthStatus = "CRITICAL";

      digitalWrite(RED_LED, HIGH);

      digitalWrite(BUZZER, HIGH);

      digitalWrite(RELAY, HIGH);

      criticalFault = true;
    }
    else
    {
      healthStatus = "PACK FAILURE";

      digitalWrite(RED_LED, HIGH);

      digitalWrite(BUZZER, HIGH);

      digitalWrite(RELAY, HIGH);

      criticalFault = true;
    }

    // Serial Monitor
    Serial.println("\n===================");

    Serial.print("Cell1 : ");
    Serial.println(cell1);

    Serial.print("Cell2 : ");
    Serial.println(cell2);

    Serial.print("Cell3 : ");
    Serial.println(cell3);

    Serial.print("Cell4 : ");
    Serial.println(cell4);

    Serial.print("Average : ");
    Serial.println(packAverage);

    Serial.print("Imbalance : ");
    Serial.print(imbalancePercent);
    Serial.println("%");

    Serial.print("Strongest : Cell");
    Serial.println(strongestCell);

    Serial.print("Weakest : Cell");
    Serial.println(weakestCell);

    Serial.print("Status : ");
    Serial.println(healthStatus);
  }

  // LCD HMI
  if (currentMillis - screenTimer >= screenInterval)
  {
    screenTimer = currentMillis;

    // Fault Override Screen
    if (criticalFault)
    {
      lcd.clear();

      lcd.setCursor(0, 0);
      lcd.print("!!! WARNING !!!");

      lcd.setCursor(0, 1);
      lcd.print(healthStatus);
    }
    else
    {
      currentScreen++;

      if (currentScreen > 4)
      {
        currentScreen = 0;
      }

      lcd.clear();

      // Screen 1
      if (currentScreen == 0)
      {
        lcd.setCursor(0, 0);
        lcd.print("C1:");
        lcd.print(cell1, 1);

        lcd.print(" C2:");
        lcd.print(cell2, 1);

        lcd.setCursor(0, 1);
        lcd.print("C3:");
        lcd.print(cell3, 1);

        lcd.print(" C4:");
        lcd.print(cell4, 1);
      }

      // Screen 2
      else if (currentScreen == 1)
      {
        lcd.setCursor(0, 0);
        lcd.print("AVG:");
        lcd.print(packAverage, 2);

        lcd.setCursor(0, 1);
        lcd.print("IMB:");
        lcd.print(imbalancePercent, 1);
        lcd.print("%");
      }

      // Screen 3
      else if (currentScreen == 2)
      {
        lcd.setCursor(0, 0);
        lcd.print("MAX:C");
        lcd.print(strongestCell);

        lcd.print(" ");
        lcd.print(highestVoltage, 2);

        lcd.setCursor(0, 1);
        lcd.print("MIN:C");
        lcd.print(weakestCell);

        lcd.print(" ");
        lcd.print(lowestVoltage, 2);
      }

      // Screen 4
      else if (currentScreen == 3)
      {
        lcd.setCursor(0, 0);
        lcd.print("STATUS:");

        lcd.setCursor(0, 1);
        lcd.print(healthStatus);
      }

      // Screen 5
      else if (currentScreen == 4)
      {
        lcd.setCursor(0, 0);
        lcd.print("Relay:");

        if (digitalRead(RELAY))
          lcd.print("OFF");
        else
          lcd.print("ON");

        lcd.setCursor(0, 1);
        lcd.print("Prot Active");
      }
    }
  }

  // Blynk
  Blynk.virtualWrite(V0, cell1);
  Blynk.virtualWrite(V1, cell2);
  Blynk.virtualWrite(V2, cell3);
  Blynk.virtualWrite(V3, cell4);

  Blynk.virtualWrite(V4, packAverage);

  Blynk.virtualWrite(V5, imbalancePercent);

  Blynk.virtualWrite(V6, healthStatus);

  Blynk.virtualWrite(V7, strongestCell);

  Blynk.virtualWrite(V8, weakestCell);
}
