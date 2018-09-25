#include <ESP8266WiFi.h>
#include <ESPHTTPClient.h>
#include <JsonListener.h>
#include <stdio.h>
#include <time.h>                   // struct timeval
#include <coredecls.h>                  // settimeofday_cb()
#include <Arduino.h>
#include <U8g2lib.h>
#include <SPI.h>
#include <WiFiManager.h>

#define USE_WIFI_MANAGER 0    // 0 to NOT use WiFi manager, 1 to use
#define USE_HIGH_ALARM 1      // 0 - LOW alarm sounds, 1 - HIGH alarm sounds
#define ALARMPIN 5

#define BACKLIGHTPIN 2
#define BUTTONPIN  4

const char CompileDate[] = __DATE__;
const char* WIFI_SSID[] = {"ibehome", "ibetest", "ibehomen", "TYCP", "Tenda_301"};
const char* WIFI_PWD[] = {"tianwanggaidihu", "tianwanggaidihu", "tianwanggaidihu", "5107458970", "5107458970"};
#define numWIFIs (sizeof(WIFI_SSID)/sizeof(char *))
#define WIFI_TRY 30

#define TZ              8       // (utc+) TZ in hours
#define DST_MN          0      // use 60mn for summer time in some countries
#define TZ_MN           ((TZ)*60)
#define TZ_SEC          ((TZ)*3600)
#define DST_SEC         ((DST_MN)*60)

// 1 CS, 2 Reset, 3 RS/DC, 4 Clock, 5 SID(data)
U8G2_ST7565_LM6059_1_4W_SW_SPI display(U8G2_R2, /* clock=*/ 14, /* data=*/ 12, /* cs=*/ 13, /* dc=*/ 15, /* reset=*/ 16);

time_t nowTime;
uint8_t draw_state = 0;
int lightLevel[10];

int buttonState;             // the current reading from the input pin
int lastButtonState = LOW;   // the previous reading from the input pin
// the following variables are unsigned longs because the time, measured in
// milliseconds, will quickly become a bigger number than can be stored in an int.
unsigned long lastDebounceTime = 0;  // the last time the output pin was toggled
unsigned long debounceDelay = 30;    // the debounce time; increase if the output flickers
int questionCount = 0;
int questionTotal = 100;
String divideSign = String((char)247);
int currentMode = 0; // 0 - show question, 1 - show answer
String currentQuestion = "";
String currentAnswer = "";

void generateQuestion() {
  int questionType = random(1, 4); // 1 - plus, 2 - minus, 3 - multiply
  int firstNumber = 1;
  int secondNumber = 1;
  if (questionType == 1 || questionType == 2)
  {
    // plus or minus
    firstNumber = random(1, 100);
    secondNumber = random(1, 100);
    if (questionType == 1)
    {
      currentQuestion = String(firstNumber) + "+" + String(secondNumber) + "=";
      currentAnswer = currentQuestion + String(firstNumber + secondNumber);
    }
    else
    {
      if (secondNumber > firstNumber)
      {
        int tempNumber = firstNumber;
        firstNumber = secondNumber;
        secondNumber = tempNumber;
      }
      currentQuestion = String(firstNumber) + "-" + String(secondNumber) + "=";
      currentAnswer = currentQuestion + String(firstNumber - secondNumber);
    }
  }
  else if (questionType == 3)
  {
    // multiply
    firstNumber = random(2, 10);
    secondNumber = random(2, 10);
    currentQuestion = String(firstNumber) + "X" + String(secondNumber) + "=";
    currentAnswer = currentQuestion + String(firstNumber * secondNumber);
  }
}

void setup() {
  delay(100);
  pinMode(ALARMPIN, OUTPUT);
  pinMode(BACKLIGHTPIN, OUTPUT);
  analogWrite(BACKLIGHTPIN, 900); // Maximum is 1023
  shortBeep();
  for (int i = 0; i < 10; ++i)
  {
    lightLevel[i] = analogRead(A0); // initialize array with values
  }
  display.begin();
  display.setFontPosTop();

  Serial.begin(115200);
  Serial.println("Scan WIFI");

#if USE_WIFI_MANAGER > 0
  WiFi.persistent(true);
  WiFiManager wifiManager;
  wifiManager.setConfigPortalTimeout(600);
  wifiManager.autoConnect("IBEClock12864-HW");
  Serial.println("Please connect WiFi IBEClock12864-HW");
#else
  int intPreferredWIFI = 0;
  WiFi.persistent(false);
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();

  int n = WiFi.scanNetworks();
  if (n == 0)
  {
  }
  else
  {
    for (int i = 0; i < n; ++i)
    {
      for (int j = 0; j < numWIFIs; j++)
      {
        if (strcmp(WIFI_SSID[j], string2char(WiFi.SSID(i))) == 0)
        {
          intPreferredWIFI = j;
          break;
        }
      }
    }
  }
  Serial.println("Connect WIFI");

  WiFi.persistent(true);
  WiFi.begin(WIFI_SSID[intPreferredWIFI], WIFI_PWD[intPreferredWIFI]);
  int WIFIcounter = intPreferredWIFI;
  while (WiFi.status() != WL_CONNECTED) {
    int counter = 0;
    while (counter < WIFI_TRY && WiFi.status() != WL_CONNECTED)
    {
      if (WiFi.status() == WL_CONNECTED) break;
      delay(500);
      if (WiFi.status() == WL_CONNECTED) break;
      counter++;
    }
    if (WiFi.status() == WL_CONNECTED) break;
    WIFIcounter++;
    if (WIFIcounter >= numWIFIs) WIFIcounter = 0;
    WiFi.begin(WIFI_SSID[WIFIcounter], WIFI_PWD[WIFIcounter]);
  }
#endif

  if (WiFi.status() != WL_CONNECTED) ESP.restart();

  // Get time from network time service
  Serial.println("WIFI Connected");
  configTime(TZ_SEC, DST_SEC, "pool.ntp.org");
  generateQuestion();
  questionCount = 1;
}

void loop() {
  for (int i = 0; i < 9; ++i)
  {
    lightLevel[i] = lightLevel[i + 1]; // shift value forward
  }
  lightLevel[9] = analogRead(A0); // 0 very strong light, 200 and above dark, read new value
  int lightLevelSum = 0;
  for (int i = 0; i < 10; ++i)
  {
    lightLevelSum += lightLevel[i];
  }

  lightLevelSum = (lightLevelSum / 10) - 50;
  Serial.print("Backlight: ");
  Serial.println(lightLevelSum);
  if (lightLevelSum < 10)
  {
    analogWrite(BACKLIGHTPIN, 1023); // Maximum is 1023
  }
  else if (lightLevelSum < 15)
  {
    analogWrite(BACKLIGHTPIN, 1000); // Maximum is 1023
  }
  else if (lightLevelSum < 20)
  {
    analogWrite(BACKLIGHTPIN, 900); // Maximum is 1023
  }
  else if (lightLevelSum < 25)
  {
    analogWrite(BACKLIGHTPIN, 800); // Maximum is 1023
  }
  else if (lightLevelSum < 30)
  {
    analogWrite(BACKLIGHTPIN, 700); // Maximum is 1023
  }
  else if (lightLevelSum < 35)
  {
    analogWrite(BACKLIGHTPIN, 600); // Maximum is 1023
  }
  else if (lightLevelSum < 40)
  {
    analogWrite(BACKLIGHTPIN, 500); // Maximum is 1023
  }
  else if (lightLevelSum < 45)
  {
    analogWrite(BACKLIGHTPIN, 400); // Maximum is 1023
  }
  else if (lightLevelSum < 50)
  {
    analogWrite(BACKLIGHTPIN, 300); // Maximum is 1023
  }
  else if (lightLevelSum < 100)
  {
    analogWrite(BACKLIGHTPIN, 200); // Maximum is 1023
  }
  else if (lightLevelSum < 200)
  {
    analogWrite(BACKLIGHTPIN, 100); // Maximum is 1023
  }
  else
  {
    analogWrite(BACKLIGHTPIN, 50); // Maximum is 1023
  }

  int reading;
  display.firstPage();
  do {
    reading = digitalRead(BUTTONPIN);
    if (reading != lastButtonState) {
      lastDebounceTime = millis();
    }
    if ((millis() - lastDebounceTime) > debounceDelay)
    {
      if (reading != buttonState)
      {
        buttonState = reading;
        if (buttonState == HIGH)
        {
          shortBeep();
          if (currentMode == 0)
          {
            currentMode = 1;
          }
          else
          {
            currentMode = 0;
            generateQuestion();
            questionCount++;
            if (questionCount >= questionTotal + 1)
            {
              questionCount = 1;
            }
          }
        }
      }
    }
    lastButtonState = reading;
    draw();
    delay(100);
    draw_state++;
  } while (display.nextPage());

  reading = digitalRead(BUTTONPIN);
  if (reading != lastButtonState) {
    lastDebounceTime = millis();
  }
  if ((millis() - lastDebounceTime) > debounceDelay)
  {
    if (reading != buttonState)
    {
      buttonState = reading;
      if (buttonState == HIGH)
      {
        shortBeep();
        if (currentMode == 0)
        {
          currentMode = 1;
        }
        else
        {
          currentMode = 0;
          generateQuestion();
          questionCount++;
          if (questionCount >= questionTotal + 1)
          {
            questionCount = 1;
          }
        }
      }
    }
  }
  lastButtonState = reading;

  if (draw_state >= 10)
  {
    draw_state = 0;
  }
}

void draw(void) {
  nowTime = time(nullptr);
  struct tm* timeInfo;
  timeInfo = localtime(&nowTime);
  char buff[20];
  sprintf_P(buff, PSTR("%02d:%02d"), timeInfo->tm_hour, timeInfo->tm_min);

  display.setFont(u8g2_font_helvB10_tf); // u8g2_font_helvB08_tf, u8g2_font_6x13_tn
  display.setCursor(1, 1);
  display.print(questionCount);
  display.print("/");
  display.print(questionTotal);
  
  display.setCursor(90, 1);
  display.print(buff);

  display.setFont(u8g2_font_helvB18_tf); // u8g2_font_helvB08_tf, u8g2_font_10x20_tf
  int stringWidth = display.getStrWidth(string2char(currentAnswer));
  display.setCursor((128 - stringWidth) / 2, 28);
  if (currentMode == 0)
  {
    display.print(currentQuestion);
  }
  else
  {
    display.print(currentAnswer);
  }
}


/*

  display.setFont(u8g2_font_helvR24_tn); // u8g2_font_inb21_ mf, u8g2_font_helvR24_tn
  //  sprintf_P(buff, PSTR("%02d:%02d:%02d"), timeInfo->tm_hour, timeInfo->tm_min, timeInfo->tm_sec);
  sprintf_P(buff, PSTR("%02d:%02d"), timeInfo->tm_hour, timeInfo->tm_min);
  stringWidth = display.getStrWidth(buff);
  display.drawStr((128 - 30 - stringWidth) / 2, 11, buff);
  display.setFont(u8g2_font_helvB08_tf);
  display.drawHLine(0, 51, 128);
  display.setFont(u8g2_font_helvR24_tn);
*/
// each Chinese character's length is 3 in UTF-8

void shortBeep() {
#if (USE_HIGH_ALARM > 0)
  digitalWrite(ALARMPIN, HIGH);
  delay(150);
  digitalWrite(ALARMPIN, LOW);
#else
  digitalWrite(ALARMPIN, LOW);
  delay(150);
  digitalWrite(ALARMPIN, HIGH);
#endif
}

char* string2char(String command) {
  if (command.length() != 0) {
    char *p = const_cast<char*>(command.c_str());
    return p;
  }
}

