#define BLYNK_TEMPLATE_ID "TMPL35I2TEQxx"
#define BLYNK_TEMPLATE_NAME "Adaptive Battery Intelligence System"
#define BLYNK_AUTH_TOKEN "hMDAwt5ID8lHcmDVprpva5dtXdZygRRN"

#include <WiFi.h>
#include <BlynkSimpleEsp32.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

LiquidCrystal_I2C lcd(0x27, 16, 2);

char auth[] = BLYNK_AUTH_TOKEN;
char ssid[] = "Wokwi-GUEST";
char pass[] = "";

#define CELL1 32
#define CELL2 33
#define CELL3 34
#define CELL4 35

#define GREEN_LED   12
#define YELLOW_LED  13
#define RED_LED     14

#define BUZZER      15
#define RELAY       16

float cell1, cell2, cell3, cell4;
float packAverage;
float highestVoltage;
float lowestVoltage;
float imbalancePercent;

String healthStatus = "";

int strongestCell;
int weakestCell;

float readVoltage(int pin)
{
  int adcValue = analogRead(pin);
  float voltage = (adcValue / 4095.0) * 3.3;
  return voltage;
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

  lcd.setCursor(0, 0);
  lcd.print("Adaptive BMS");

  lcd.setCursor(0, 1);
  lcd.print("Initializing");

  Blynk.begin(auth, ssid, pass);

  delay(2000);

  lcd.clear();
}

void loop()
{
  Blynk.run();

  // Read voltages
  cell1 = readVoltage(CELL1);
  cell2 = readVoltage(CELL2);
  cell3 = readVoltage(CELL3);
  cell4 = readVoltage(CELL4);

  // Average voltage
  packAverage =
    (cell1 + cell2 + cell3 + cell4) / 4.0;

  // Strongest cell
  highestVoltage = cell1;
  strongestCell = 1;

  if (cell2 > highestVoltage)
  {
    highestVoltage = cell2;
    strongestCell = 2;
  }

  if (cell3 > highestVoltage)
  {
    highestVoltage = cell3;
    strongestCell = 3;
  }

  if (cell4 > highestVoltage)
  {
    highestVoltage = cell4;
    strongestCell = 4;
  }

  // Weakest cell
  lowestVoltage = cell1;
  weakestCell = 1;

  if (cell2 < lowestVoltage)
  {
    lowestVoltage = cell2;
    weakestCell = 2;
  }

  if (cell3 < lowestVoltage)
  {
    lowestVoltage = cell3;
    weakestCell = 3;
  }

  if (cell4 < lowestVoltage)
  {
    lowestVoltage = cell4;
    weakestCell = 4;
  }

  // Imbalance %
  imbalancePercent =
    ((highestVoltage - lowestVoltage) / packAverage) * 100.0;

  digitalWrite(GREEN_LED, LOW);
  digitalWrite(YELLOW_LED, LOW);
  digitalWrite(RED_LED, LOW);

  digitalWrite(BUZZER, LOW);
  digitalWrite(RELAY, LOW);

  // Health condition
  if (lowestVoltage < 0.5)
  {
    healthStatus = "PACK FAILURE";

    digitalWrite(RED_LED, HIGH);
    digitalWrite(BUZZER, HIGH);
    digitalWrite(RELAY, HIGH);
  }

  else if (imbalancePercent < 5)
  {
    healthStatus = "HEALTHY";

    digitalWrite(GREEN_LED, HIGH);
  }

  else if (imbalancePercent < 20)
  {
    healthStatus = "MINOR";

    digitalWrite(YELLOW_LED, HIGH);
  }

  else
  {
    healthStatus = "CRITICAL";

    digitalWrite(RED_LED, HIGH);
    digitalWrite(BUZZER, HIGH);
    digitalWrite(RELAY, HIGH);
  }

  // LCD
  lcd.clear();

  lcd.setCursor(0, 0);
  lcd.print("AVG:");
  lcd.print(packAverage, 2);

  lcd.setCursor(10, 0);
  lcd.print("I:");
  lcd.print((int)imbalancePercent);

  lcd.setCursor(0, 1);
  lcd.print(healthStatus);

  // Serial monitor
  Serial.println("\n====================");

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

  Serial.print("Imbalance % : ");
  Serial.println(imbalancePercent);

  Serial.print("Strongest Cell : ");
  Serial.println(strongestCell);

  Serial.print("Weakest Cell : ");
  Serial.println(weakestCell);

  Serial.print("Health : ");
  Serial.println(healthStatus);

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

  delay(1000);
}
