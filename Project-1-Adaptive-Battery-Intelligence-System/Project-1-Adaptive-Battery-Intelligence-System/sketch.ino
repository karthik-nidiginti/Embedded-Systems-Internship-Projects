#define BLYNK_TEMPLATE_ID   "TMPL35I2TEQxx"
#define BLYNK_TEMPLATE_NAME "Adaptive Battery Intelligence System"
#define BLYNK_AUTH_TOKEN    "hMDAwt5ID8lHcmDVprpva5dtXdZygRRN"

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
float packAverage;
float highestVoltage;
float lowestVoltage;
float imbalancePercent;
int strongestCell;
int weakestCell;
int healthCode;
String healthStatus;

unsigned long lastUpdate = 0;

float readVoltage(int pin)
{
  long sum = 0;

  for (int i = 0; i < 8; i++)
    sum += analogRead(pin);

  float adcVolt = (sum / 8.0 / 4095.0) * 3.3;
  return adcVolt * 1.303;
}

void beepAlert()
{
  for (int i = 0; i < 3; i++)
  {
    digitalWrite(BUZZER, HIGH);
    delay(150);

    digitalWrite(BUZZER, LOW);
    delay(150);
  }
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
  lcd.print("Initializing...");

  Blynk.begin(auth, ssid, pass);

  delay(2000);

  lcd.clear();
}

void loop()
{
  Blynk.run();

  if (millis() - lastUpdate < 1000)
    return;

  lastUpdate = millis();

  cell1 = readVoltage(CELL1_PIN);
  cell2 = readVoltage(CELL2_PIN);
  cell3 = readVoltage(CELL3_PIN);
  cell4 = readVoltage(CELL4_PIN);

  packAverage = (cell1 + cell2 + cell3 + cell4) / 4.0;

  highestVoltage = cell1;
  lowestVoltage = cell1;

  strongestCell = 1;
  weakestCell = 1;

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

  imbalancePercent =
      ((highestVoltage - lowestVoltage) / packAverage) * 100.0;

  if (lowestVoltage < 2.5)
  {
    healthStatus = "PACK FAILURE";
    healthCode = 4;
  }
  else if (highestVoltage > 4.2)
  {
    healthStatus = "OVERVOLTAGE";
    healthCode = 3;
  }
  else if (imbalancePercent >= 20)
  {
    healthStatus = "CRITICAL";
    healthCode = 2;
  }
  else if (imbalancePercent >= 5)
  {
    healthStatus = "MINOR IMBAL.";
    healthCode = 1;
  }
  else
  {
    healthStatus = "HEALTHY";
    healthCode = 0;
  }

  digitalWrite(GREEN_LED, LOW);
  digitalWrite(YELLOW_LED, LOW);
  digitalWrite(RED_LED, LOW);
  digitalWrite(RELAY, LOW);
  digitalWrite(BUZZER, LOW);

  if (healthCode == 0)
  {
    digitalWrite(GREEN_LED, HIGH);
  }
  else if (healthCode == 1)
  {
    digitalWrite(YELLOW_LED, HIGH);
  }
  else
  {
    digitalWrite(RED_LED, HIGH);
    digitalWrite(RELAY, HIGH);
    beepAlert();
  }

  lcd.setCursor(0, 0);
  lcd.print("AVG:");
  lcd.print(packAverage, 2);
  lcd.print("V    ");

  lcd.setCursor(0, 1);

  char hbuf[17];
  snprintf(hbuf, sizeof(hbuf), "%-16s", healthStatus.c_str());

  lcd.print(hbuf);

  Serial.println("====================");
  Serial.print("Cell1: ");
  Serial.println(cell1, 2);

  Serial.print("Cell2: ");
  Serial.println(cell2, 2);

  Serial.print("Cell3: ");
  Serial.println(cell3, 2);

  Serial.print("Cell4: ");
  Serial.println(cell4, 2);

  Serial.print("Avg  : ");
  Serial.println(packAverage, 2);

  Serial.print("Imbal: ");
  Serial.print(imbalancePercent, 1);
  Serial.println("%");

  Serial.print("Best : Cell");
  Serial.println(strongestCell);

  Serial.print("Weak : Cell");
  Serial.println(weakestCell);

  Serial.print("Health: ");
  Serial.println(healthStatus);

  Blynk.virtualWrite(V0, cell1);
  Blynk.virtualWrite(V1, cell2);
  Blynk.virtualWrite(V2, cell3);
  Blynk.virtualWrite(V3, cell4);
  Blynk.virtualWrite(V4, packAverage);
  Blynk.virtualWrite(V5, imbalancePercent);
  Blynk.virtualWrite(V6, healthCode);
  Blynk.virtualWrite(V7, healthStatus);
  Blynk.virtualWrite(V8, strongestCell);
  Blynk.virtualWrite(V9, weakestCell);
}
