#define BLYNK_TEMPLATE_ID   "TMPL3_45Wk_yb"
#define BLYNK_TEMPLATE_NAME "Executive Battery Intelligence Dashboard"
#define BLYNK_AUTH_TOKEN    "c4X84S9A38UUVrSIXaAjGmXuz1IJL6cA"

#include <WiFi.h>
#include <BlynkSimpleEsp32.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

char ssid[] = "Wokwi-GUEST";
char pass[] = "";

LiquidCrystal_I2C lcd(0x27, 16, 2);

#define CELL1_PIN  32
#define CELL2_PIN  33
#define CELL3_PIN  34
#define CELL4_PIN  35

#define GREEN_LED  18
#define YELLOW_LED 19
#define RED_LED    23
#define BUZZER_PIN 25

BlynkTimer timer;

int  faultCount       = 0;
bool buzzerState      = false;
bool lastFaultState   = false;
bool lastWarnState    = false;

unsigned long buzzerTimer = 0;

float readCellVoltage(int pin)
{
  long sum = 0;
  for (int i = 0; i < 8; i++) sum += analogRead(pin);
  float adcVolt = (sum / 8.0 / 4095.0) * 3.3;
  return adcVolt * 1.303;
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

void sendBatteryData()
{
  float cell1 = readCellVoltage(CELL1_PIN);
  float cell2 = readCellVoltage(CELL2_PIN);
  float cell3 = readCellVoltage(CELL3_PIN);
  float cell4 = readCellVoltage(CELL4_PIN);

  float packVoltage = cell1 + cell2 + cell3 + cell4;
  float avgVoltage  = packVoltage / 4.0;

  float highest = max(max(cell1, cell2), max(cell3, cell4));
  float lowest  = min(min(cell1, cell2), min(cell3, cell4));

  float imbalance = 0;
  if (avgVoltage > 0)
    imbalance = ((highest - lowest) / avgVoltage) * 100.0;

  int health = (int)constrain(
    ((avgVoltage - 2.5) / (4.2 - 2.5)) * 100.0, 0, 100);

  String weakestCell   = "";
  String strongestCell = "";

  if      (lowest == cell1) weakestCell = "Cell 1";
  else if (lowest == cell2) weakestCell = "Cell 2";
  else if (lowest == cell3) weakestCell = "Cell 3";
  else                      weakestCell = "Cell 4";

  if      (highest == cell1) strongestCell = "Cell 1";
  else if (highest == cell2) strongestCell = "Cell 2";
  else if (highest == cell3) strongestCell = "Cell 3";
  else                       strongestCell = "Cell 4";

  String riskLevel;
  String systemStatus;
  String recommendation;
  bool   isFault = false;
  bool   isWarn  = false;

  if (lowest < 2.5)
  {
    riskLevel      = "HIGH";
    systemStatus   = "PACK FAILURE";
    recommendation = "Replace Battery Pack";
    isFault        = true;

    digitalWrite(RED_LED,    HIGH);
    digitalWrite(YELLOW_LED, LOW);
    digitalWrite(GREEN_LED,  LOW);

    if (!lastFaultState)
    {
      faultCount++;
      Blynk.logEvent("pack_failure");
      lastFaultState = true;
    }
  }
  else if (highest > 4.2)
  {
    riskLevel      = "HIGH";
    systemStatus   = "OVERVOLTAGE";
    recommendation = "Stop Charging Now";
    isFault        = true;

    digitalWrite(RED_LED,    HIGH);
    digitalWrite(YELLOW_LED, LOW);
    digitalWrite(GREEN_LED,  LOW);

    if (!lastFaultState)
    {
      faultCount++;
      Blynk.logEvent("battery_critical");
      lastFaultState = true;
    }
  }
  else if (imbalance > 10)
  {
    riskLevel      = "HIGH";
    systemStatus   = "CRIT IMBALANCE";
    recommendation = "Immediate Maintenance";
    isFault        = true;

    digitalWrite(RED_LED,    HIGH);
    digitalWrite(YELLOW_LED, LOW);
    digitalWrite(GREEN_LED,  LOW);

    if (!lastFaultState)
    {
      faultCount++;
      Blynk.logEvent("battery_critical");
      lastFaultState = true;
    }
  }
  else if (imbalance > 5)
  {
    riskLevel      = "MEDIUM";
    systemStatus   = "MINOR IMBALANCE";
    recommendation = "Monitor Battery";
    isWarn         = true;

    digitalWrite(RED_LED,    LOW);
    digitalWrite(YELLOW_LED, HIGH);
    digitalWrite(GREEN_LED,  LOW);

    if (!lastWarnState)
    {
      Blynk.logEvent("battery_warning");
      lastWarnState = true;
    }
  }
  else
  {
    riskLevel      = "LOW";
    systemStatus   = "HEALTHY";
    recommendation = "No Action Required";

    digitalWrite(RED_LED,    LOW);
    digitalWrite(YELLOW_LED, LOW);
    digitalWrite(GREEN_LED,  HIGH);

    lastFaultState = false;
    lastWarnState  = false;
  }

  if (!isFault && !isWarn)
  {
    digitalWrite(BUZZER_PIN, LOW);
    buzzerState = false;
  }

  lcdRow(0, "Pack:%.1fV %s", packVoltage, isFault ? "!" : isWarn ? "~" : "OK");
  lcdRow(1, "%s", systemStatus.c_str());

  Serial.println("====================");
  Serial.print("Cell1       : "); Serial.println(cell1, 2);
  Serial.print("Cell2       : "); Serial.println(cell2, 2);
  Serial.print("Cell3       : "); Serial.println(cell3, 2);
  Serial.print("Cell4       : "); Serial.println(cell4, 2);
  Serial.print("Pack Voltage: "); Serial.println(packVoltage, 2);
  Serial.print("Average     : "); Serial.println(avgVoltage, 2);
  Serial.print("Imbalance   : "); Serial.print(imbalance, 1); Serial.println("%");
  Serial.print("Health      : "); Serial.println(health);
  Serial.print("Risk        : "); Serial.println(riskLevel);
  Serial.print("Status      : "); Serial.println(systemStatus);
  Serial.print("Recommend   : "); Serial.println(recommendation);
  Serial.print("Faults      : "); Serial.println(faultCount);

  Blynk.virtualWrite(V0,  cell1);
  Blynk.virtualWrite(V1,  cell2);
  Blynk.virtualWrite(V2,  cell3);
  Blynk.virtualWrite(V3,  cell4);
  Blynk.virtualWrite(V4,  packVoltage);
  Blynk.virtualWrite(V5,  avgVoltage);
  Blynk.virtualWrite(V6,  imbalance);
  Blynk.virtualWrite(V7,  health);
  Blynk.virtualWrite(V8,  riskLevel);
  Blynk.virtualWrite(V9,  systemStatus);
  Blynk.virtualWrite(V10, recommendation);
  Blynk.virtualWrite(V11, faultCount);
  Blynk.virtualWrite(V12, weakestCell);
  Blynk.virtualWrite(V13, strongestCell);
  Blynk.virtualWrite(V14, isFault ? 1 : 0);
}

void pulseBuzzer()
{
  if (digitalRead(RED_LED))
  {
    unsigned long now = millis();
    if (now - buzzerTimer >= 300)
    {
      buzzerTimer = now;
      buzzerState = !buzzerState;
      digitalWrite(BUZZER_PIN, buzzerState);
    }
  }
}

void setup()
{
  Serial.begin(115200);
  analogReadResolution(12);
  Wire.begin(21, 22);

  pinMode(GREEN_LED,  OUTPUT);
  pinMode(YELLOW_LED, OUTPUT);
  pinMode(RED_LED,    OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);

  digitalWrite(GREEN_LED,  LOW);
  digitalWrite(YELLOW_LED, LOW);
  digitalWrite(RED_LED,    LOW);
  digitalWrite(BUZZER_PIN, LOW);

  lcd.init();
  lcd.backlight();

  lcd.setCursor(0, 0); lcd.print("Battery System  ");
  lcd.setCursor(0, 1); lcd.print("Initializing... ");

  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass);

  delay(2000);

  lcd.setCursor(0, 0); lcd.print("                ");
  lcd.setCursor(0, 1); lcd.print("                ");

  timer.setInterval(2000L, sendBatteryData);
  timer.setInterval(300L,  pulseBuzzer);
}

void loop()
{
  Blynk.run();
  timer.run();
}
