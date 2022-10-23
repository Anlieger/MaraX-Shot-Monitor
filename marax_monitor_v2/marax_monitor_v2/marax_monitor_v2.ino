//Includes
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <SoftwareSerial.h>
#include <Timer.h>
#include "bitmaps.h"

//Defines
#define SCREEN_WIDTH 128  //Width in px
#define SCREEN_HEIGHT 64  // Height in px
#define OLED_RESET -1
#define SCREEN_ADDRESS 0x3C  // or 0x3D Check datasheet or Oled Display
#define BUFFER_SIZE 32

#define D5 (14)
#define D6 (12)
#define D7 (13)

#define PUMP_PIN D7

//Internals
bool reedOpenSensor = true;

long timerStartMillis = 0;
long timerStopMillis = 0;
long timerDisplayOffMillis = 0;
int timerCount = 0;
bool timerStarted = false;
bool displayOn = false;

int prevTimerCount = 0;
long serialTimeout = 0;
char buffer[BUFFER_SIZE];
int bufferIndex = 0;
int isMaraOff = 0;
long lastToggleTime = 0;
int HeatDisplayToggle = 0;
int pumpInValue = 0;
const int Sim = 0;
int tt = 8;

//Mara Data
String maraData[7];

//Instances
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
SoftwareSerial mySerial(D5, D6);
Timer t;

void SetSim() {
  if (Sim == 1) {
    //C1.06,116,124,093,0840,1,0
    maraData[0] = String("C1.06");
    maraData[1] = String("198");
    maraData[2] = String("124");
    maraData[3] = String("199");
    maraData[4] = String("0840");
    maraData[5] = String("0");
    maraData[6] = String("1");
  }
}

void setup() {
  display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS);
  display.clearDisplay();
  display.display();
  Serial.begin(9600);
  mySerial.begin(9600);
  mySerial.write(0x11);

  pinMode(PUMP_PIN, INPUT_PULLUP);
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);

  t.every(1000, updateView);
}

void getMaraData() {
  /*
    Example Data: C1.06,116,124,093,0840,1,0\n every ~400-500ms
    Length: 26
    [Pos] [Data] [Describtion]
    0)      C     Coffee Mode (C) or SteamMode (V)
    -        1.06  Software Version
    1)      116   current steam temperature (Celsisus)
    2)      124   target steam temperature (Celsisus)
    3)      093   current hx temperature (Celsisus)
    4)      0840  countdown for 'boost-mode'
    5)      1     heating element on or off
    6)      0     pump on or off
  */

  while (mySerial.available()) {
    isMaraOff = 0;
    serialTimeout = millis();
    char rcv = mySerial.read();
    if (rcv != '\n')
      buffer[bufferIndex++] = rcv;
    else {
      bufferIndex = 0;
      Serial.println(buffer);
      char* ptr = strtok(buffer, ",");
      int idx = 0;
      while (ptr != NULL) {
        maraData[idx++] = String(ptr);
        ptr = strtok(NULL, ",");
      }
    }
  }
  if (millis() - serialTimeout > 1000) {
    isMaraOff = 1;
    SetSim();
    serialTimeout = millis();
    mySerial.write(0x11);
  }
}

void detectChanges() {
  // if (maraData[0] == 0 && Sim == 0) {
  //   isMaraOff = 1;
  // }
  digitalWrite(LED_BUILTIN, digitalRead(PUMP_PIN));
  if (reedOpenSensor) {
    pumpInValue = digitalRead(PUMP_PIN);
  } else {
    pumpInValue = !digitalRead(PUMP_PIN);
  }

  if (maraData[6].toInt() == 1) {
  // if (!pumpInValue) {
    if (!timerStarted) {
      timerStartMillis = millis();
      timerStarted = true;
      displayOn = true;
      Serial.println("Start pump");
    }
  }
  if (maraData[6].toInt() == 0) {
  // if (pumpInValue) {
    if (timerStarted) {
      if (timerStopMillis == 0) {
        timerStopMillis = millis();
      }
      if (millis() - timerStopMillis > 500) {
        timerStarted = false;
        timerStopMillis = 0;
        timerDisplayOffMillis = millis();
        display.invertDisplay(false);
        Serial.println("Stop pump");
        tt = 8;

        delay(4000);
      }
    }
  } else {
    timerStopMillis = 0;
  }
}

String getTimer() {
  char outMin[2];
  if (timerStarted) {
    timerCount = (millis() - timerStartMillis) / 1000;
    if (timerCount > 4) {
      prevTimerCount = timerCount;
    }
  } else {
    timerCount = prevTimerCount;
  }
  if (timerCount > 99) {
    return "99";
  }
  sprintf(outMin, "%02u", timerCount);
  return outMin;
}

void updateView() {
  display.clearDisplay();
  display.setTextColor(WHITE);

  if (isMaraOff == 99) {
    display.setCursor(30, 6);
    display.setTextSize(2);
    display.print("MARA X");
    display.setCursor(30, 28);
    display.setTextSize(4);
    display.print("OFF");
  } else {
    if (timerStarted) {
      // draw the timer on the right
      display.fillRect(60, 9, 63, 55, BLACK);
      display.setTextSize(5);
      display.setCursor(68, 20);
      display.print(getTimer());

      if (timerCount >= 20 && timerCount <= 24) {
      display.setTextSize(5);
      display.setCursor(68, 20);
        display.print(getTimer());

        display.setTextSize(1);
        display.setCursor(38, 2);
        display.print("Get ready");
      }
      if (timerCount > 24) {
        display.setTextSize(5);
        display.setCursor(68, 20);
        display.print(getTimer());

        display.setTextSize(1);
        display.setCursor(35, 2);
        display.print("You missed");
      }      

      if (tt >= 1 && timerCount <= 23) {
        if (tt == 8) {
          display.drawBitmap(17, 14, coffeeCup30_01, 30, 30, WHITE);
          Serial.println(tt);
        } else if (tt == 7) {
          display.drawBitmap(17, 14, coffeeCup30_02, 30, 30, WHITE);
          Serial.println(tt);
        } else if (tt == 6) {
          display.drawBitmap(17, 14, coffeeCup30_03, 30, 30, WHITE);
          Serial.println(tt);
        } else if (tt == 5) {
          display.drawBitmap(17, 14, coffeeCup30_04, 30, 30, WHITE);
          Serial.println(tt);
        } else if (tt == 4) {
          display.drawBitmap(17, 14, coffeeCup30_05, 30, 30, WHITE);
          Serial.println(tt);
        } else if (tt == 3) {
          display.drawBitmap(17, 14, coffeeCup30_06, 30, 30, WHITE);
          Serial.println(tt);
        } else if (tt == 2) {
          display.drawBitmap(17, 14, coffeeCup30_07, 30, 30, WHITE);
          Serial.println(tt);
        } else if (tt == 1) {
          display.drawBitmap(17, 14, coffeeCup30_08, 30, 30, WHITE);
          Serial.println(tt);
        }
        if (tt == 1 && timerCount <= 24) {
          tt = 8;
        } else {
          tt--;
        }
      } else {
        if (tt == 8) {
          display.drawBitmap(17, 14, coffeeCup30_09, 30, 30, WHITE);
          Serial.println(tt);
        } else if (tt == 7) {
          display.drawBitmap(17, 14, coffeeCup30_10, 30, 30, WHITE);
          Serial.println(tt);
        } else if (tt == 6) {
          display.drawBitmap(17, 14, coffeeCup30_11, 30, 30, WHITE);
          Serial.println(tt);
        } else if (tt == 5) {
          display.drawBitmap(17, 14, coffeeCup30_12, 30, 30, WHITE);
          Serial.println(tt);
        } else if (tt == 4) {
          display.drawBitmap(17, 14, coffeeCup30_13, 30, 30, WHITE);
          Serial.println(tt);
        } else if (tt == 3) {
          display.drawBitmap(17, 14, coffeeCup30_14, 30, 30, WHITE);
          Serial.println(tt);
        } else if (tt == 2) {
          display.drawBitmap(17, 14, coffeeCup30_15, 30, 30, WHITE);
          Serial.println(tt);
        } else if (tt == 1) {
          display.drawBitmap(17, 14, coffeeCup30_16, 30, 30, WHITE);
          Serial.println(tt);
        }
        if (tt == 1) {
          tt = 8;
        } else {
          tt--;
        }
      }

      if (maraData[3].toInt() < 100) {
        display.setCursor(19, 50);
      } else {
        display.setCursor(9, 50);
      }
      display.setTextSize(2);
      display.print(maraData[3].toInt());
      display.setTextSize(1);
      display.print((char)247);
      display.setTextSize(1);
      display.print("C");
    } else {
      //Coffee temperature and bitmap
      display.drawBitmap(17, 14, coffeeCup30_00, 30, 30, WHITE);
      if (maraData[3].toInt() < 100) {
        display.setCursor(19, 50);
      } else {
        display.setCursor(9, 50);
      }
      display.setTextSize(2);
      display.print(maraData[3].toInt());
      display.setTextSize(1);
      display.print((char)247);
      display.setTextSize(1);
      display.print("C");

      //Steam temperature and bitmap
      display.drawBitmap(83, 14, steam30, 30, 30, WHITE);
      if (maraData[1].toInt() < 100) {
        display.setCursor(88, 50);
      } else {
        display.setCursor(78, 50);
      }
      display.setTextSize(2);
      display.print(maraData[1].toInt());
      display.setTextSize(1);
      display.print((char)247);
      display.setTextSize(1);
      display.print("C");

      //Draw Line
      display.drawLine(66, 14, 66, 64, WHITE);

      //Boiler
      if (maraData[5].toInt() == 1) {
        display.setCursor(13, 0);
        display.setTextSize(1);
        display.print("Heating up");
        //Steam target temperature
        // display.print(maraData[2].toInt());
        // display.print((char)247);
        // display.print("C");

        if ((millis() - lastToggleTime) > 1000) {
          lastToggleTime = millis();
          if (HeatDisplayToggle == 1) {
            HeatDisplayToggle = 0;
          } else {
            HeatDisplayToggle = 1;
          }
        }
        if (HeatDisplayToggle == 1) {
          display.fillRect(0, 0, 12, 12, BLACK);
          display.drawCircle(3, 3, 3, WHITE);
          display.fillCircle(3, 3, 2, WHITE);

        } else {
          display.fillRect(0, 0, 12, 12, BLACK);
          display.drawCircle(3, 3, 3, WHITE);
          // display.print("");
        }
      } else {
        display.print("");
        display.fillCircle(3, 3, 3, BLACK);

        //Draw machine mode
        if (maraData[0].substring(0, 1) == "C") {
          // Coffee mode
          display.drawBitmap(0, 0, coffeeCup12, 12, 12, WHITE);
        } else {
          // Steam mode
          display.drawBitmap(0, 0, steam12, 12, 12, WHITE);
        }
      }
      // ECO mode
      /*
    if (!isMaraOff == 1 && maraData[2].toInt() == 000) {
      display.clearDisplay();
      display.fillRect(4, 4, 124, 124, BLACK);
      display.setCursor(30, 30);
      display.setTextSize(4);
      display.print("ECO");
    }
    */
    }
  }

  display.display();
}

void loop() {
  t.update();
  detectChanges();
  getMaraData();
}