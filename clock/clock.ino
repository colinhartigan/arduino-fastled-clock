#include <FastLED.h>
#include <DS3232RTC.h> //note that <v2.0.0 must be used

#define nextPin 12
#define prevPin 11
#define actionPin 10
#define lightPin A0
#define pin 5
#define LED_TYPE WS2812B
#define COLOR_ORDER GRB
#define NUM_LEDS 58
#define BRIGHTNESS 75
CRGB leds[NUM_LEDS];
DS3232RTC myRTC(false);
tmElements_t tm;

CHSV colorD1(0, 255, 180);
CHSV colorD2(20, 255, 180);
CHSV colorD3(40, 255, 180);
CHSV colorD4(60, 255, 180);
CRGB convertedD1;
CRGB convertedD2;
CRGB convertedD3;
CRGB convertedD4;


//master variables
int mode = 0; //0=clock, 1=date, 2=stopwatch, 3=temp, 4=lightshow
int nextMode;
const int modeNames[][4] = {
  {29, 18, 22, 14}, //TIME
  {13, 10, 29, 14}, //DATE
  {32, 29, 12, 17}, //WTCH
  {12, 24, 23, 29}, //CONT
  {29, 14, 22, 25}, //TEMP
  {28, 17, 24, 32}, //SHOW
};
const int maxMode = 5;
bool rainbowEnabled = true;//whether to run the digit rainbow cycle
bool darkEnabled = false;//enables during a certain range
int counter = 0;//used to know when to change the rainbow
int rainbowThreshold = 10;
bool nightMode = false;
bool flash = false;


//clock variables
int clockColor = 0;
const int currentYear = 2020;
CRGB clockColors[][4] = {
  {convertedD1, convertedD2, convertedD3, convertedD4}, //standard rainbow
  {CRGB::Yellow , CRGB::Yellow , CRGB::Red, CRGB::Red}, //night mode
};


//button variables
bool upDB = false;//debounces for buttons being held
bool downDB = false;
bool actionDB = false;
bool longAction = false;
long actionTimer = 0;
bool actionEnabled = false;//true when action button is being held
const int longPressThreshold = 4000;
bool alternateEnabledState = false;//enables every other time action button is pressed


//light show variables
int lightShowMode = 0; //0=rainbow
uint8_t showHue = 0; //rotating "base color" used by light show patterns


//stopwatch variables
bool watchRunning = false;
int watchTime = 0;
bool resetting = false;


//font variables
const int d1 = 0;
const int d2 = 14;
const int d3 = 30;
const int d4 = 44;
const int font[][7] = {
  //order: mid, top right, top mid, top left, bottom left, bottom mid, bottom right
  {0, 1, 1, 1, 1, 1, 1}, //0:0
  {0, 1, 0, 0, 0, 0, 1}, //1:1
  {1, 1, 1, 0, 1, 1, 0}, //2:2
  {1, 1, 1, 0, 0, 1, 1}, //3:3
  {1, 1, 0, 1, 0, 0, 1}, //4:4
  {1, 0, 1, 1, 0, 1, 1}, //5:5
  {1, 0, 1, 1, 1, 1, 1}, //6:6
  {0, 1, 1, 0, 0, 0, 1}, //7:7
  {1, 1, 1, 1, 1, 1, 1}, //8:8
  {1, 1, 1, 1, 0, 1, 1}, //9:9
  {1, 1, 1, 1, 1, 0, 1}, //A:10
  {1, 0, 0, 1, 1, 1, 1}, //B:11
  {0, 0, 1, 1, 1, 1, 0}, //C:12
  {1, 1, 0, 0, 1, 1, 1}, //D:13
  {1, 0, 1, 1, 1, 1, 0}, //E:14
  {1, 0, 1, 1, 1, 0, 0}, //F:15
  {0, 0, 1, 1, 1, 1, 1}, //G:16
  {1, 1, 0, 1, 1, 0, 1}, //H:17
  {0, 0, 0, 1, 1, 0, 0}, //I:18
  {0, 1, 0, 0, 1, 1, 1}, //J:19
  {1, 0, 1, 1, 1, 0, 1}, //K:20
  {0, 0, 0, 1, 1, 1, 0}, //L:21
  {0, 0, 1, 0, 1, 0, 1}, //M:22
  {0, 1, 1, 1, 1, 0, 1}, //N:23
  {0, 1, 1, 1, 1, 1, 1}, //O:24
  {1, 1, 1, 1, 1, 0, 0}, //P:25
  {1, 1, 1, 1, 0, 1, 0}, //Q:26
  {0, 1, 1, 1, 1, 0, 0}, //R:27
  {1, 0, 1, 1, 0, 1, 1}, //S:28
  {1, 0, 0, 1, 1, 1, 0}, //T:29
  {0, 1, 0, 1, 1, 1, 1}, //U:30
  {0, 1, 0, 1, 1, 1, 1}, //V:31
  {0, 1, 0, 1, 0, 1, 0}, //W:32
  {1, 1, 0, 1, 1, 0, 1}, //X:33
  {1, 1, 0, 1, 0, 1, 1}, //Y:34
  {1, 1, 1, 0, 1, 1, 0}, //Z:35
  {1, 1, 1, 1, 0, 0, 0}, //DEGREES SYMBOL:36
  {0, 0, 0, 0, 0, 0, 0}, //BLANK:37
};


//draw a digit given start location, color, and font code
void drawDigit(int loc, CRGB col, int code) {
  int current = 0;
  for (int i = 0; i < 7; i++) {
    if (font[code][i] == 1) {
      leds[loc + current] = col;
      leds[loc + current + 1] = col;
    } else if (font[code][i] == 0) {
      leds[loc + current].setRGB(0, 0, 0);
      leds[loc + current + 1].setRGB(0, 0, 0);
    }
    current += 2;
  }
}


//draw seconds indicator
void secondsIndicator(bool state) {
  if (state == true) {
    leds[d3 - 2] = CRGB(255, 255, 255);
    leds[d3 - 1] = CRGB(255, 255, 255);
  } else {
    leds[d3 - 2].setRGB(0, 0, 0);
    leds[d3 - 1].setRGB(0, 0, 0);
  }
}


//add glitter (from light show mode)
void addGlitter(fract8 chanceOfGlitter)
{
  if ( random8() < chanceOfGlitter) {
    leds[ random16(NUM_LEDS) ] += CRGB::White;
  }
}


void setup() {

  Serial.begin(9600);

  delay(3000); // 3 second delay for recovery

  Serial.println("something to test");

  pinMode(nextPin, INPUT);//set up buttons
  pinMode(prevPin, INPUT);//set up buttons
  pinMode(actionPin, INPUT);//set up buttons

  myRTC.begin(); //start clock connection

  // tell FastLED about the LED strip configuration
  FastLED.addLeds<LED_TYPE, pin, COLOR_ORDER>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);

  // set master brightness control
  FastLED.setBrightness(BRIGHTNESS);
  fill_solid(leds, NUM_LEDS, CRGB(0, 0, 0));
}


void loop() {

  EVERY_N_MILLISECONDS(20) {
    showHue++; //slowly cycle the "base color" through the rainbow for light show effects
  }


  //night time sensing
  if (nightMode == true) {
    FastLED.setBrightness(2);
    if (random8() < 50) {
      Serial.println(1);
      darkEnabled = true;
    }
  } else {
    darkEnabled = false;
    FastLED.setBrightness(BRIGHTNESS);
  }


  //button sensing
  if (digitalRead(nextPin) == HIGH) {
    if (upDB == false) {
      upDB = true;
      nextMode = mode + 1;
      if (nextMode > maxMode) {
        nextMode = 0;
      }
      mode = -1;
    }
    secondsIndicator(false);
    drawDigit(d1, CRGB::Khaki, modeNames[nextMode][0]);
    drawDigit(d2, CRGB::Khaki, modeNames[nextMode][1]);
    drawDigit(d3, CRGB::Khaki, modeNames[nextMode][2]);
    drawDigit(d4, CRGB::Khaki, modeNames[nextMode][3]);
  } else {
    if (upDB == true) {
      mode = nextMode;
      upDB = false;
    }
  }

  if (digitalRead(prevPin) == HIGH) {
    if (downDB == false) {
      downDB = true;
      nextMode = mode - 1;
      if (nextMode < 0) {
        nextMode = maxMode;
      }
      mode = -1;
    }
    secondsIndicator(false);
    drawDigit(d1, CRGB::Khaki, modeNames[nextMode][0]);
    drawDigit(d2, CRGB::Khaki, modeNames[nextMode][1]);
    drawDigit(d3, CRGB::Khaki, modeNames[nextMode][2]);
    drawDigit(d4, CRGB::Khaki, modeNames[nextMode][3]);
  } else {
    if (downDB == true) {
      mode = nextMode;
      downDB = false;
    }
  }

  if (digitalRead(actionPin) == HIGH) {
    if (actionDB == false) {
      actionDB = true;
      actionEnabled = true;
      actionTimer = millis();
    }
    if ((millis() - actionTimer > longPressThreshold) && (longAction == false)) {
      Serial.println("longpress");
      longAction = true;
    }
  } else {
    if (actionDB == true) {
      actionEnabled = false;
      actionDB = false;
      longAction = false;
      alternateEnabledState = !alternateEnabledState;
    }
  }


  //control lights and stuff
  //divide time up
  RTC.read(tm);
  int timeHour;

//  if (tm.Hour > 12) {
//    timeHour = tm.Hour - 12;
//  } else {
//    timeHour = tm.Hour;
//  }
  timeHour = tm.Hour;

  if (tm.Hour >= 23 || tm.Hour < 6) {
    nightMode = true;
  } else {
    nightMode = false;
  }


  //rainbow colors changing
  if (rainbowEnabled == true) {
    if (counter < rainbowThreshold) {
      counter++;
    } else {
      hsv2rgb_rainbow(colorD1, convertedD1);
      hsv2rgb_rainbow(colorD2, convertedD2);
      hsv2rgb_rainbow(colorD3, convertedD3);
      hsv2rgb_rainbow(colorD4, convertedD4);

      clockColors[0][0] = convertedD1;
      clockColors[0][1] = convertedD2;
      clockColors[0][2] = convertedD3;
      clockColors[0][3] = convertedD4;

      colorD1.hue = (colorD1.hue - 1) % 256;
      colorD2.hue = (colorD2.hue - 1) % 256;
      colorD3.hue = (colorD3.hue - 1) % 256;
      colorD4.hue = (colorD4.hue - 1) % 256;
      counter = 0;
    }
  }


  //mode control

  //clock
  if (mode == 0) {

    int tensHour = timeHour / 10;
    int onesHour = timeHour % 10;
    int tensMin = tm.Minute / 10;
    int onesMin = tm.Minute % 10;
    int tensSec = tm.Second / 10;
    int onesSec = tm.Second % 10;

    rainbowEnabled = true;
    int tensDigit = 37;

//    if (tensHour != 0) {
//      tensDigit = tensHour;
//    } else {
//      tensDigit = 37;
//    }

    if(tensHour == 0){
      tensDigit = 37;
    } else {
      tensDigit = tensHour;
    }

    drawDigit(d1, clockColors[clockColor][0], tensDigit);
    drawDigit(d2, clockColors[clockColor][1], onesHour);
    drawDigit(d3, clockColors[clockColor][2], tensMin);
    drawDigit(d4, clockColors[clockColor][3], onesMin);

    if (onesSec % 2 == 0) {
      secondsIndicator(true);
    } else {
      secondsIndicator(false);
    }

    if (actionEnabled == true) {
      drawDigit(d1, clockColors[clockColor][0], 37);
      drawDigit(d2, clockColors[clockColor][1], 37);
      drawDigit(d3, clockColors[clockColor][2], tensSec);
      drawDigit(d4, clockColors[clockColor][3], onesSec);
    }

    if (darkEnabled == true) {
      secondsIndicator(false);
      clockColor = 1;
    } else {
      clockColor = 0;
    }

    if (longAction == true) {
      Serial.println("long");
    }
  }

  //date
  if (mode == 1) {

    rainbowEnabled = true;

    int tensMonth = tm.Month / 10;
    int onesMonth = tm.Month % 10;
    int tensDay = tm.Day / 10;
    int onesDay = tm.Day % 10;

    drawDigit(d1, convertedD1, tensMonth);
    drawDigit(d2, convertedD2, onesMonth);
    drawDigit(d3, convertedD3, tensDay);
    drawDigit(d4, convertedD4, onesDay);

    secondsIndicator(true);

    if (actionEnabled == true) {
      secondsIndicator(false);
      drawDigit(d1, convertedD1, currentYear / 1000 % 10);
      drawDigit(d2, convertedD2, currentYear / 100 % 10);
      drawDigit(d3, convertedD3, currentYear / 10 % 10);
      drawDigit(d4, convertedD4, currentYear % 10);
    }

  }

  //stopwatch
  if (mode == 2) {
    rainbowEnabled = false;
    int seconds = watchTime % 60;
    int hours = watchTime / 60;
    int minutes = hours % 60;
    hours = hours / 60;
    if (watchRunning == false && resetting == false) {
      if (hours == 0) {
        drawDigit(d1, CRGB::Coral, minutes / 10);
        drawDigit(d2, CRGB::Coral, minutes % 10);
        drawDigit(d3, CRGB::Coral, seconds / 10);
        drawDigit(d4, CRGB::Coral, seconds % 10);
      } else {
        drawDigit(d1, CRGB::Coral, hours / 10);
        drawDigit(d2, CRGB::Coral, hours % 10);
        drawDigit(d3, CRGB::Coral, minutes / 10);
        drawDigit(d4, CRGB::Coral, minutes % 10);
      }
      if (seconds > 0) {
        secondsIndicator(false);
      }
    }
    if (actionEnabled == true) {
      if (watchRunning == true) {
        watchRunning = false;
      } else {
        watchRunning = true;
      }
    }
    if (longAction == true) {
      resetting = true;
      drawDigit(d1, CRGB::White, 27);
      drawDigit(d2, CRGB::White, 14);
      drawDigit(d3, CRGB::White, 28);
      drawDigit(d4, CRGB::White, 29);
      watchTime = 0;
    } else {
      resetting = false;
    }
    if (actionEnabled == false && watchRunning == true && resetting == false) {
      EVERY_N_SECONDS(1) {
        watchTime++;
      }
      Serial.println(watchTime);
      if (hours == 0) {
        if (minutes / 10 == 0) {
          drawDigit(d1, CRGB::Aquamarine, 37);
        } else {
          drawDigit(d1, CRGB::Aquamarine, minutes / 10);
        }
        drawDigit(d2, CRGB::Aquamarine, minutes % 10);
        drawDigit(d3, CRGB::Aquamarine, seconds / 10);
        drawDigit(d4, CRGB::Aquamarine, seconds % 10);
      } else {
        if (hours / 10 == 0) {
          drawDigit(d1, CRGB::Aquamarine, 37);
        } else {
          drawDigit(d1, CRGB::Aquamarine, hours / 10);
        }
        drawDigit(d2, CRGB::Aquamarine, hours % 10);
        drawDigit(d3, CRGB::Aquamarine, minutes / 10);
        drawDigit(d4, CRGB::Aquamarine, minutes % 10);
      }
      if (seconds % 2 == 0) {
        secondsIndicator(true);
      } else {
        secondsIndicator(false);
      }
    }
  }

  //countdown
  if (mode == 3) {
    Serial.println("uh");
  }

  //temp
  if (mode == 4) {
    rainbowEnabled = false;

    int temp = (RTC.temperature() / 4);
    int fahrenheit =  temp * 9 / 5 + 32;
    int tens = fahrenheit / 10;
    int ones = fahrenheit % 10;

    if (fahrenheit > 70) {
      drawDigit(d1, CRGB::Yellow, tens);
      drawDigit(d2, CRGB::Yellow, ones);
    } else {
      drawDigit(d1, CRGB::Navy, tens);
      drawDigit(d2, CRGB::Navy, ones);
    }
    drawDigit(d3, CRGB::Khaki, 36);
    drawDigit(d4, CRGB::Khaki, 15);

    if (actionEnabled == true) {
      if (temp > 21) {
        drawDigit(d1, CRGB::Yellow, temp / 10);
        drawDigit(d2, CRGB::Yellow, temp % 10);
      } else {
        drawDigit(d1, CRGB::Navy, temp / 10);
        drawDigit(d2, CRGB::Navy, temp % 10);
      }
      drawDigit(d3, CRGB::LightCoral, 36);
      drawDigit(d4, CRGB::LightCoral, 12);
    }
  }


  //light show
  if (mode == 5) {

    if (actionEnabled == true) {
      actionEnabled = false;
      lightShowMode++;
      if (lightShowMode > 4) lightShowMode = 0;
    }

    if (lightShowMode == 0) {
      //rainbow
      fill_rainbow(leds, NUM_LEDS, showHue, 7);
    }
    if (lightShowMode == 1) {
      // a colored dot sweeping back and forth, with fading trails
      fadeToBlackBy(leds, NUM_LEDS, 20);
      int pos = beatsin16(13, 0, NUM_LEDS - 1 );
      leds[pos] += CHSV(showHue, 255, 192);
    }
    if (lightShowMode == 2) {
      // random colored speckles that blink in and fade smoothly
      fadeToBlackBy( leds, NUM_LEDS, 10);
      int pos = random16(NUM_LEDS);
      leds[pos] += CHSV( showHue + random8(64), 200, 255);
    }
    if (lightShowMode == 3) {
      // eight colored dots, weaving in and out of sync with each other
      fadeToBlackBy( leds, NUM_LEDS, 20);
      byte dothue = 0;
      for ( int i = 0; i < 8; i++) {
        leds[beatsin16( i + 7, 0, NUM_LEDS - 1 )] |= CHSV(dothue, 200, 255);
        dothue += 32;
      }
    }
    if (lightShowMode == 4) {
      // random colored speckles that blink in and fade smoothly
      fadeToBlackBy( leds, NUM_LEDS, 10);
      int pos = random16(NUM_LEDS);
      leds[pos] += CHSV( showHue + random8(64), 200, 255);
    }

  }

  FastLED.show();

}
