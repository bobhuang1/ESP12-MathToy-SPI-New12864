#include <ESP8266WiFi.h>
#include <stdio.h>
#include <time.h>                   // struct timeval
#include <coredecls.h>                  // settimeofday_cb()
#include <Arduino.h>
#include <U8g2lib.h>
#include <SPI.h>
#include <WiFiManager.h>
#include "StringHelpers.h"
#include "AlarmBeeper.h"
#include "MathQuizGenerator.h"
#include "BacklightController.h"
#include "WiFiMultiConnect.h"
#include "BootSplashBitmap.h"

//#define USE_WIFI_MANAGER     // disable to NOT use WiFi manager, enable to use
#define USE_HIGH_ALARM       // disable - LOW alarm sounds, enable - HIGH alarm sounds

#define ALARMPIN 5
#define BACKLIGHTPIN 0
#define BUTTONPIN  4
#define NUMBER_CEILING 100 // max random operand value for generated questions

#ifdef USE_HIGH_ALARM
const bool ALARM_ACTIVE_HIGH = true;
#else
const bool ALARM_ACTIVE_HIGH = false;
#endif

// How long setup() tries to join WiFi before starting the quiz offline. WiFi is
// only needed for the clock in the corner.
#define WIFI_CONNECT_TIMEOUT_MS 30000
// time() values below this mean NTP hasn't synced yet (2020-09-13).
#define MIN_VALID_EPOCH 1600000000

// Fill in your own SSID/password pairs (or better, use USE_WIFI_MANAGER above
// instead of hardcoding any of this). Never commit real WiFi credentials.
const char* const WIFI_SSIDS[] = {"YOUR_SSID_1", "YOUR_SSID_2", "YOUR_SSID_3"};
const char* const WIFI_PASSWORDS[] = {"YOUR_PASSWORD_1", "YOUR_PASSWORD_2", "YOUR_PASSWORD_3"};

U8G2_ST7565_LM6059_F_4W_SW_SPI display(U8G2_R2, /* clock=*/ 14, /* data=*/ 12, /* cs=*/ 13, /* dc=*/ 2, /* reset=*/ 16);

BacklightController backlight;

int buttonState;             // the current reading from the input pin
int lastButtonState = LOW;   // the previous reading from the input pin
// the following variables are unsigned longs because the time, measured in
// milliseconds, will quickly become a bigger number than can be stored in an int.
unsigned long lastDebounceTime = 0;  // the last time the output pin was toggled
unsigned long debounceDelay = 30;    // the debounce time; increase if the output flickers
int questionCount = 0;
int questionTotal = 100;
int currentMode = 0; // 0 - show question, 1 - show answer
String currentQuestion = "";
String currentAnswer = "";

void setup() {
  delay(100);
  Serial.begin(115200);
  Serial.println("Begin");
  // Seed from the ESP8266 hardware RNG (A0 is the backlight's light sensor, so
  // analogRead(A0) only gave the room's light level).
  randomSeed(ESP.random());

  pinMode(BUTTONPIN, INPUT);
  pinMode(ALARMPIN, OUTPUT);
  backlight.begin(BACKLIGHTPIN);
  beepOff(ALARMPIN, ALARM_ACTIVE_HIGH);

  display.begin();
  display.setFontPosTop();
  display.setContrast(133);

  display.clearBuffer();
  display.drawXBM(31, 0, 66, 64, garfield);
  display.sendBuffer();
  beepShort(ALARMPIN, ALARM_ACTIVE_HIGH);
  delay(1000);

#ifdef USE_WIFI_MANAGER
  // Show the instructions while the portal is open, not after it closes.
  drawProgress("请用手机设置本机WIFI", "SSID ESP8266-Setup");
  bool wifiOk = connectWiFiWithManager("ESP8266-Setup", WIFI_CONNECT_TIMEOUT_MS / 1000);
#else
  Serial.println("Scan WIFI");
  drawProgress("正在连接WIFI...", "");
  bool wifiOk = connectWiFi(WIFI_SSIDS, WIFI_PASSWORDS, 3, 30, WIFI_CONNECT_TIMEOUT_MS);
#endif

  if (wifiOk)
  {
    // Get time from network time service
    Serial.println("WIFI Connected");
    drawProgress("连接WIFI成功,", "正在同步时间...");
    configTime(TZ_SEC_FOR(8), DST_SEC_FOR(0), DefaultNtpServer);
  }
  else
  {
    Serial.println("No WIFI, running offline");
  }
  currentQuestion = generateMathQuestion(currentAnswer, NUMBER_CEILING, false);
  questionCount = 1;
}

void detectButtonPush() {
  int reading;
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
        beepShort(ALARMPIN, ALARM_ACTIVE_HIGH);
        if (currentMode == 0)
        {
          currentMode = 1;
        }
        else
        {
          currentMode = 0;
          currentQuestion = generateMathQuestion(currentAnswer, NUMBER_CEILING, false);
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
}

void loop() {
  backlight.update();

  display.firstPage();
  do {
    detectButtonPush();
    draw();
  } while (display.nextPage());

  detectButtonPush();
}

void draw(void) {
  time_t nowTime = time(nullptr);
  char buff[20];
  if (nowTime < MIN_VALID_EPOCH)
  {
    strcpy(buff, "--:--"); // no NTP time yet (or no WiFi)
  }
  else
  {
    struct tm* timeInfo = localtime(&nowTime);
    sprintf_P(buff, PSTR("%02d:%02d"), timeInfo->tm_hour, timeInfo->tm_min);
  }

  display.setFont(u8g2_font_helvB10_tf); // u8g2_font_helvB08_tf, u8g2_font_6x13_tn
  display.setCursor(1, 1);
  display.print(questionCount);
  display.print("/");
  display.print(questionTotal);

  display.setCursor(90, 1);
  display.print(buff);

  display.setFont(u8g2_font_helvB12_tf); // u8g2_font_helvB08_tf, u8g2_font_10x20_tf
  const String& shown = currentMode == 0 ? currentQuestion : currentAnswer;
  int stringWidth = display.getStrWidth(string2char(shown));
  display.setCursor((128 - stringWidth) / 2, 28);
  display.print(shown);
}

void drawProgress(String labelLine1, String labelLine2) {
  display.clearBuffer();
  display.enableUTF8Print();
  display.setFont(u8g2_font_wqy12_t_gb2312); // u8g2_font_wqy12_t_gb2312, u8g2_font_helvB08_tf
  int stringWidth = 1;
  if (labelLine1 != "")
  {
    stringWidth = display.getUTF8Width(string2char(labelLine1));
    display.setCursor((128 - stringWidth) / 2, 13);
    display.print(labelLine1);
  }
  if (labelLine2 != "")
  {
    stringWidth = display.getUTF8Width(string2char(labelLine2));
    display.setCursor((128 - stringWidth) / 2, 36);
    display.print(labelLine2);
  }
  display.disableUTF8Print();
  display.sendBuffer();
}

// each Chinese character's length is 3 in UTF-8

