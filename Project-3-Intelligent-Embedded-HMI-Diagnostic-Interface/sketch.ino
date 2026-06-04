#define BLYNK_TEMPLATE_ID   "TMPL3iD_qUOVk"
#define BLYNK_TEMPLATE_NAME "Intelligent Embedded HMI and Diagnostic Interface"
#define BLYNK_AUTH_TOKEN    "LHQgvaIjitfP1h-yZSx3nfN_SpmrmX5m"

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
float packAverage, imbalancePercent;
float highestVoltage, lowestVoltage;

int strongestCell = 0;
int weakestCell   = 0;
int healthCode    = 0;
int currentScreen = 0;

String healthStatus = "";

bool criticalFault  = false;
bool buzzerState    = false;
unsigned long buzzerTimer = 0;

unsigned long sensorTimer = 0;
unsigned long screenTimer = 0;
unsigned long blynkTimer  = 0;

const unsigned long sensorInterval = 500;
const unsigned long screenInterval = 2500;
const unsigned long blynkInterval  = 1000;

float readVoltage(int pin)
{
  long sum = 0;
  for (int i = 0; i < 8; i++) sum += analogRead(pin);
  float adcVolt = (sum / 8.0 / 4095.0) * 3.3;
  return adcVolt * 1.303;
}

void pulseBuzzer(unsigned long now)
{
  if (now - buzzerTimer >= 300)
  {
    buzzerTimer = now;
    buzzerState = !buzzerState;
    digitalWrite(BUZZER, buzzerState);
  }
}

void lcdPrint(int col, int row, const char* fmt, ...)
{
  char buf[17];
  va_list args;
  va_start(args, fmt);
  vsnprintf(buf, sizeof(buf), fmt, args);
  va_end(args);
  lcd.setCursor(col, row);
  lcd.print(buf);
}

void lcdRow(int row, const char* fmt, ...)
{
  char buf[17];
  va_list args;
  va_start(args, fmt);
  vsnprintf(buf, sizeof(buf), fmt, args);
  va_end(args);
  lcd.setCursor(0, row);
  char padded[17];
  snprintf(padded, sizeof(padded), "%-16s", buf);
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

  lcd.setCursor(0, 0); lcd.print("Smart HMI       ");
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

    cell1 = readVoltage(CELL1_PIN);
    cell2 = readVoltage(CELL2_PIN);
    cell3 = readVoltage(CELL3_PIN);
    cell4 = readVoltage(CELL4_PIN);

    packAverage    = (cell1 + cell2 + cell3 + cell4) / 4.0;
    highestVoltage = max(max(cell1, cell2), max(cell3, cell4));
    lowestVoltage  = min(min(cell1, cell2), min(cell3, cell4));

    if (packAverage > 0)
      imbalancePercent = ((highestVoltage - lowestVoltage) / packAverage) * 100.0;
    else
      imbalancePercent = 0;

    if      (highestVoltage == cell1) strongestCell = 1;
    else if (highestVoltage == cell2) strongestCell = 2;
    else if (highestVoltage == cell3) strongestCell = 3;
    else                              strongestCell = 4;

    if      (lowestVoltage == cell1) weakestCell = 1;
    else if (lowestVoltage == cell2) weakestCell = 2;
    else if (lowestVoltage == cell3) weakestCell = 3;
    else                             weakestCell = 4;

    if (lowestVoltage < 2.5)
    {
      healthStatus = "PACK FAILURE";
      healthCode   = 4;
    }
    else if (highestVoltage > 4.2)
    {
      healthStatus = "OVERVOLTAGE";
      healthCode   = 3;
    }
    else if (imbalancePercent >= 20)
    {
      healthStatus = "CRITICAL";
      healthCode   = 2;
    }
    else if (imbalancePercent >= 5)
    {
      healthStatus = "MINOR IMBAL.";
      healthCode   = 1;
    }
    else
    {
      healthStatus = "HEALTHY";
      healthCode   = 0;
    }

    criticalFault = (healthCode >= 2);

    digitalWrite(GREEN_LED,  LOW);
    digitalWrite(YELLOW_LED, LOW);
    digitalWrite(RED_LED,    LOW);

    if (healthCode == 0)      { digitalWrite(GREEN_LED,  HIGH); }
    else if (healthCode == 1) { digitalWrite(YELLOW_LED, HIGH); }
    else                      { digitalWrite(RED_LED,    HIGH); }

    if (criticalFault)
      digitalWrite(RELAY, HIGH);
    else
    {
      digitalWrite(RELAY,  LOW);
      digitalWrite(BUZZER, LOW);
      buzzerState = false;
    }

    Serial.println("====================");
    Serial.print("Cell1     : "); Serial.println(cell1, 2);
    Serial.print("Cell2     : "); Serial.println(cell2, 2);
    Serial.print("Cell3     : "); Serial.println(cell3, 2);
    Serial.print("Cell4     : "); Serial.println(cell4, 2);
    Serial.print("Average   : "); Serial.println(packAverage, 2);
    Serial.print("Imbalance : "); Serial.print(imbalancePercent, 1); Serial.println("%");
    Serial.print("Strongest : Cell"); Serial.println(strongestCell);
    Serial.print("Weakest   : Cell"); Serial.println(weakestCell);
    Serial.print("Status    : "); Serial.println(healthStatus);
  }

  if (criticalFault) pulseBuzzer(now);

  if (now - screenTimer >= screenInterval)
  {
    screenTimer = now;

    if (criticalFault)
    {
      lcdRow(0, "!!! WARNING !!!");
      lcdRow(1, "%s", healthStatus.c_str());
    }
    else
    {
      currentScreen++;
      if (currentScreen > 4) currentScreen = 0;

      if (currentScreen == 0)
      {
        lcdRow(0, "C1:%.1f C2:%.1f", cell1, cell2);
        lcdRow(1, "C3:%.1f C4:%.1f", cell3, cell4);
      }
      else if (currentScreen == 1)
      {
        lcdRow(0, "AVG: %.2fV", packAverage);
        lcdRow(1, "IMB: %.1f%%", imbalancePercent);
      }
      else if (currentScreen == 2)
      {
        lcdRow(0, "MAX:C%d  %.2fV", strongestCell, highestVoltage);
        lcdRow(1, "MIN:C%d  %.2fV", weakestCell,   lowestVoltage);
      }
      else if (currentScreen == 3)
      {
        lcdRow(0, "STATUS:");
        lcdRow(1, "%s", healthStatus.c_str());
      }
      else if (currentScreen == 4)
      {
        bool relayOn = digitalRead(RELAY);
        lcdRow(0, "Relay: %s", relayOn ? "ON " : "OFF");
        lcdRow(1, relayOn ? "Prot Active" : "System OK");
      }
    }
  }

  if (now - blynkTimer >= blynkInterval)
  {
    blynkTimer = now;

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
}
