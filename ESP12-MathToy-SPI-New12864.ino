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

// Fill in your own SSID/password pairs (or better, use USE_WIFI_MANAGER above
// instead of hardcoding any of this). Never commit real WiFi credentials.
const char* const WIFI_SSIDS[] = {"YOUR_SSID_1", "YOUR_SSID_2", "YOUR_SSID_3"};
const char* const WIFI_PASSWORDS[] = {"YOUR_PASSWORD_1", "YOUR_PASSWORD_2", "YOUR_PASSWORD_3"};

U8G2_ST7565_LM6059_F_4W_SW_SPI display(U8G2_R2, /* clock=*/ 14, /* data=*/ 12, /* cs=*/ 13, /* dc=*/ 2, /* reset=*/ 16);

BacklightController backlight;

time_t nowTime;
uint8_t draw_state = 0;

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

  pinMode(BUTTONPIN, INPUT);
  pinMode(ALARMPIN, OUTPUT);
  backlight.begin(BACKLIGHTPIN);
#ifdef USE_HIGH_ALARM
  digitalWrite(ALARMPIN, LOW); // Turn off alarm
#else
  digitalWrite(ALARMPIN, HIGH); // Turn off alarm
#endif

  display.begin();
  display.setFontPosTop();
  display.setContrast(133);

  display.clearBuffer();
  display.drawXBM(31, 0, 66, 64, garfield);
  display.sendBuffer();
  beepShort(ALARMPIN, true);
  delay(1000);

#ifdef USE_WIFI_MANAGER
  connectWiFiWithManager("ESP8266-Setup");
  drawProgress("请用手机设置本机WIFI", "SSID ESP8266-Setup");
#else
  Serial.println("Scan WIFI");
  drawProgress("正在连接WIFI...", "");
  connectWiFi(WIFI_SSIDS, WIFI_PASSWORDS, 3);
#endif

  if (WiFi.status() != WL_CONNECTED) ESP.restart();

  // Get time from network time service
  Serial.println("WIFI Connected");
  drawProgress("连接WIFI成功,", "正在同步时间...");
  configTime(TZ_SEC_FOR(8), DST_SEC_FOR(0), DefaultNtpServer);
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
        beepShort(ALARMPIN, true);
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
    //    delay(100);
    draw_state++;
  } while (display.nextPage());

  detectButtonPush();

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

  display.setFont(u8g2_font_helvB12_tf); // u8g2_font_helvB08_tf, u8g2_font_10x20_tf
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

