//Includes
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <SoftwareSerial.h>
#include <Timer.h>

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

// //Pins
int d5 = 5;  //orange PIN 4 Mara TX to Arduino RX D5
int d6 = 6;  //black  PIN 3 Mara RX to Arduino TX D6

//Internals
bool reedOpenSensor = true;
long lastPumpOnMillis = 0;
long timerStartMillis = 0;
long timerStopMillis = 0;
long timerDisplayOffMillis = 0;
int timerCount = 0;
bool timerStarted = false;
bool displayOn = false;
int seconds = 0;
int prevTimerCount = 0;
long serialTimeout = 0;
char buffer[BUFFER_SIZE];
int bufferIndex = 0;
int isMaraOff = 0;
long lastToggleTime = 0;
int HeatDisplayToggle = 0;

int pumpInValue = 0;


const int Sim = 0;

//Bitmap Data
// 'steam30, 30x30px
const unsigned char steam30[] PROGMEM = {
  0x00, 0x18, 0x30, 0x00, 0x00, 0x18, 0x30, 0x00, 0x00, 0x30, 0x18, 0x00, 0x00, 0x70, 0x1c, 0x00,
  0x00, 0x60, 0x0c, 0x00, 0x00, 0x61, 0x0c, 0x00, 0x00, 0x73, 0x1c, 0x00, 0x00, 0x33, 0x18, 0x00,
  0x00, 0x39, 0xb8, 0x00, 0x00, 0x19, 0xb0, 0x00, 0x00, 0x1b, 0xb0, 0x00, 0x07, 0x1b, 0x31, 0xc0,
  0x0f, 0x3b, 0x39, 0xe0, 0x3c, 0x71, 0x9c, 0x70, 0x30, 0x61, 0x8c, 0x30, 0x60, 0xe1, 0x0e, 0x18,
  0x60, 0xc3, 0x06, 0x18, 0xc0, 0x03, 0x00, 0x0c, 0xc0, 0x03, 0x80, 0x0c, 0x60, 0x01, 0x80, 0x18,
  0x60, 0x01, 0x80, 0x18, 0x30, 0x00, 0x00, 0x30, 0x3c, 0x00, 0x00, 0xf0, 0x0f, 0x80, 0x07, 0xc0,
  0x07, 0xc0, 0x0f, 0x80, 0x00, 0xc0, 0x0c, 0x00, 0x00, 0xe0, 0x1c, 0x00, 0x00, 0x70, 0x38, 0x00,
  0x00, 0x3f, 0xf0, 0x00, 0x00, 0x0f, 0xc0, 0x00
};

// 'steam12, 12x12px
const unsigned char steam12[] PROGMEM = {
  0x10, 0x80, 0x09, 0x00, 0x09, 0x00, 0x10, 0x80, 0x09, 0x00, 0x69, 0x60, 0x81, 0x10, 0x82, 0x10,
  0x60, 0x60, 0x10, 0x80, 0x19, 0x80, 0x06, 0x00
};

// 'coffeeCup12', 12x12px
const unsigned char coffeeCup12[] PROGMEM = {
  0x02, 0x00, 0x11, 0x00, 0x0a, 0x00, 0x10, 0x00, 0x08, 0x00, 0x7f, 0x80, 0x80, 0x60, 0x80, 0x50,
  0xc0, 0xe0, 0x40, 0x80, 0x61, 0x80, 0x3f, 0x00
};

// 'coffeeCup30', 30x30px
const unsigned char coffeeCup30[] PROGMEM = {
  0x00, 0x06, 0x00, 0x00, 0x00, 0x06, 0x00, 0x00, 0x00, 0x0c, 0x00, 0x00, 0x00, 0x0e, 0x00, 0x00,
  0x00, 0x67, 0x00, 0x00, 0x00, 0x73, 0x80, 0x00, 0x00, 0x39, 0x80, 0x00, 0x00, 0x39, 0x80, 0x00,
  0x00, 0x73, 0x80, 0x00, 0x00, 0xe3, 0x00, 0x00, 0x00, 0xc0, 0x00, 0x00, 0x00, 0xe0, 0x00, 0x00,
  0x00, 0x60, 0x00, 0x00, 0x3f, 0xff, 0xfc, 0x00, 0x7f, 0xff, 0xfe, 0x00, 0xc0, 0x00, 0x03, 0xc0,
  0xc0, 0x00, 0x03, 0xf0, 0xc0, 0x00, 0x03, 0x38, 0xe0, 0x00, 0x07, 0x1c, 0x60, 0x00, 0x07, 0x0c,
  0x60, 0x00, 0x06, 0x0c, 0x70, 0x00, 0x0e, 0x1c, 0x30, 0x00, 0x0c, 0x38, 0x38, 0x00, 0x1f, 0xf0,
  0x18, 0x00, 0x1f, 0xc0, 0x1c, 0x00, 0x38, 0x00, 0x0e, 0x00, 0x70, 0x00, 0x07, 0x00, 0xe0, 0x00,
  0x03, 0xff, 0xc0, 0x00, 0x01, 0xff, 0x80, 0x00
};

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
    maraData[6] = String("0");
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
  if (millis() - serialTimeout > 6000) {
    isMaraOff = 1;
    SetSim();
    serialTimeout = millis();
    mySerial.write(0x11);
  }
}

void detectChanges() {
  if (maraData[0] == 0) {
    isMaraOff = 1;
  }
  digitalWrite(LED_BUILTIN, digitalRead(PUMP_PIN));
  if (reedOpenSensor) {
    pumpInValue = digitalRead(PUMP_PIN);
  } else {
    pumpInValue = !digitalRead(PUMP_PIN);
  }

  if (maraData[6].toInt() == 1) {
    if (!timerStarted) {
      timerStartMillis = millis();
      timerStarted = true;
      displayOn = true;
      Serial.println("Start pump");
    }
  }
  if (maraData[6].toInt() == 0) {
    if (timerStarted) {
      if (timerStopMillis == 0) {
        timerStopMillis = millis();
        //delay(4000);
      }
      if (millis() - timerStopMillis > 500) {
        timerStarted = false;
        timerStopMillis = 0;
        timerDisplayOffMillis = millis();
        display.invertDisplay(false);
        Serial.println("Stop pump");
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

  if (isMaraOff == 1) {
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
      display.setCursor(68, 16);
      display.print(getTimer());
      //Coffee temperature and bitmap
      display.drawBitmap(17, 14, coffeeCup30, 30, 30, WHITE);
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
      display.drawBitmap(17, 14, coffeeCup30, 30, 30, WHITE);
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
      // display.drawLine(74, 0, 74, 64, WHITE);
      // display.drawLine(0, 18, 128, 18, WHITE);
      display.drawLine(display.width() / 2, 12, display.width() / 2, 64, WHITE);

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