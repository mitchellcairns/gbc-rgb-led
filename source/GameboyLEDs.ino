#include <FastLED.h>
#include <EEPROM.h>
#include "config.h"

#define LED_PIN 4

#define LED_COUNT 9

#define CHECK_COUNT 8 // How many times to poll buttons for averages
#define CHECK_SAVE 8

#define MAX_BRIGHTNESS 80
#define RCV 180

// PIN setup
// ----------------
const int selPIN = 0;
const int leftPIN = 2;
const int rightPIN = 1;
// ----------------
// ----------------

// Button states setup
#define NO_PRESS        0
#define RELEASED_PRESS  1
#define HELD_PRESS      2
// ----------------
// ----------------

// Dpad states setup
#define DPAD_LEFT       0
#define DPAD_RIGHT      1
#define DPAD_NEUTRAL    2
byte dpadPresses = 0;

// Release state setup
#define HELD_START      100
#define HELD_ONE        100
#define HELD_TWO        101
#define HELD_THREE      102
// ----------------
// ----------------

// Definitions for the different color modes
byte colorMode = 0;
#define COLORMODE_RAINBOW 0
#define COLORMODE_USER 1
#define COLORMODE_SOLID 2
// ----------------
// ----------------

int brightness = 20;

int selValue = HIGH;
int leftValue = HIGH;
int rightValue = HIGH;

bool leftPressed = false;
bool rightPressed = false;
byte selPressed = 0;
bool finishedHold = true;

byte timer = 0;
byte buttonTime = 250;
byte buttonPresses = 0;


bool colorDirty = true;
bool colorSet = false; // Set true to start a fade
bool colorInit = false; // Set true after changing and setting up your mode
float colorIncrement = 0; // Used to determine how to fade

byte colorStep = 15;
byte fadeTime = 50;
byte rainbowTime = 50;
#define COLOR_TIME    50
CHSV rainbowColor = CHSV(0,255,RCV);

CRGB leds[LED_COUNT] = { CRGB::Black, CRGB::Black, CRGB::Black, CRGB::Black, CRGB::Black, CRGB::Black, CRGB::Black, CRGB::Black, CRGB::Black};;
CRGB lastLeds[LED_COUNT];
CRGB nextLeds[LED_COUNT];
CRGB ledPreset[LED_COUNT] = { 0xff0505, 0xff8605, 0xfff700, 0x22ff00, 0x0022ff, 0x0022ff, 0x0022ff, 0x0022ff, 0xAE00ff};
CRGB editPresetLeds[LED_COUNT];

UserPreference upref;

void setup() {
  
  // put your setup code here, to run once:
  FastLED.addLeds<WS2812B, LED_PIN, RGB>(leds, LED_COUNT);
  FastLED.setBrightness(brightness);
  rainbowColor.value = RCV;

  pinMode(selPIN, INPUT_PULLUP);
  pinMode(leftPIN, INPUT_PULLUP);
  pinMode(rightPIN, INPUT_PULLUP);
  pinMode(3, OUTPUT);
  digitalWrite(3, LOW);

  delay(100);
  EEPROM.get(0, upref);
  delay(100);

  // Try to load upref data
  if (upref.saved == CHECK_SAVE) {
    brightness = upref.brightness;
    colorMode = upref.colorMode;
    rainbowTime = upref.rainbowTime;
    rainbowColor.hue = upref.hue;
    rainbowColor.saturation = upref.saturation;
    for (byte i = 0; i < LED_COUNT; i++) {
      ledPreset[i] = upref.ledPreset[i];
    }
  }

  FastLED.show();
  
}

void loop() {
  byte butt = buttonCounter();
  idle(butt);
}

// Functions to handle looking at the different button inputs

void smooth() { // Smooth read of values
  selValue = HIGH;
  rightValue = HIGH;
  leftValue = HIGH;

  for (byte i = 0; i < CHECK_COUNT; i++) {
    if (digitalRead(selPIN) == LOW) selValue = LOW;
    if (digitalRead(rightPIN) == LOW) rightValue = LOW;
    if (digitalRead(leftPIN) == LOW) leftValue = LOW;
  }
  FastLED.delay(1);

}

byte buttonCheck(bool timeOver) {
  smooth();

  dpadPresses = dpadCheck();

  if (selValue==LOW and !selPressed) {
    selPressed = true;
  }
  else if (selPressed and selValue==HIGH and finishedHold) {
    selPressed = false;
    return RELEASED_PRESS;
  }
  else if (!finishedHold and selValue==HIGH){
    selPressed = false;
    finishedHold = true;
  }
  
  if (selPressed and timeOver) {
    selPressed = false;
    finishedHold = false;
    return HELD_PRESS;
  }

  return NO_PRESS;
}

byte dpadCheck()  {
  
  if (!leftPressed and leftValue==LOW) {
    leftPressed = true;
  }
  else if (leftPressed and leftValue==HIGH) {
    leftPressed = false;
    return DPAD_LEFT;
  }

  if (!rightPressed and rightValue==LOW) {
    rightPressed = true;
  }
  else if (rightPressed and rightValue==HIGH) {
    rightPressed = false;
    return DPAD_RIGHT;
  }
  return DPAD_NEUTRAL; 
}

byte buttonCounter() {
  if (timer > 0 and timer < buttonTime) { // If we're in a time loop, do the following
    timer += 1;
  }

  byte btChk = 0;
  if (timer >= buttonTime) btChk = buttonCheck(true);
  else btChk = buttonCheck(false);
  
  if (btChk == RELEASED_PRESS) { // When a button is pressed what do we do?
    if (timer == 0 and buttonPresses == 0) { // If we haven't started a timer, lets add a button press and begin adding to the timer.
      timer += 1;
      buttonPresses = 1;
    }
    else if (timer < buttonTime) { // If we are still in the allotted timeframe, add another button press.
      buttonPresses += 1;
    }
  }
  
  if (timer >= buttonTime) { // What do we do if the timer is full? We reset the timer, return the button press count and reset that.
    byte toReturn = buttonPresses;
    if (btChk == HELD_PRESS){
      toReturn += HELD_START;
    }
    buttonPresses = 0;
    timer = 0;
    return toReturn;
  }

  // Default return 0
  return 0;
}
// ----------------
// ----------------

// Definitions for device modes
byte configMode = 0;
#define DEFAULT_MODE      0
#define CONFIG_MODE       1
#define EDIT_MODE         2
#define BRIGHTNESS_MODE   3
// ----------------
// ----------------

// Definitions for dit modes
byte editMode = 0;
#define EDIT_CHOOSE       0
#define EDIT_HUE          1
#define EDIT_SATURATION   2

byte presetEdit = false;

// Definitions for different button functions
#define SATURATION_UP     0 //
#define SATURATION_DOWN   1 //
#define HUE_UP            2 //
#define HUE_DOWN          3 //
#define COLOR_MODE_UP     4 //
#define COLOR_MODE_DWN    5 //
#define RAINBOW_UP        6 //
#define RAINBOW_DOWN      7 //
#define BRIGHT_UP         8 //
#define BRIGHT_DOWN       9 //
#define SAVE_ALL          10
#define ENABLE_CONFIG     11 
#define NEXT_BUTTON       12
#define PREV_BUTTON       13
#define ENABLE_EDIT       14
#define ENABLE_BRIGHTNESS 15
#define RESET_CALIBRATION 16
#define ENABLE_HUESELECT  17
#define ENABLE_SATSELECT  18

// ----------------
// ----------------

// Definitions for modifying user preset
byte buttonEditing = 0;
#define A_EDIT            0
#define B_EDIT            1
#define DPAD_EDIT         2
#define SEL_EDIT          3
#define START_EDIT        4
#define POWER_EDIT        5

void buttonFunction(byte in)  {
  if (in == COLOR_MODE_UP) {
    if (colorMode + 1 > 2) colorMode = 0;
    else colorMode += 1;
    if (colorMode == COLORMODE_SOLID)
    {
      if (upref.saved == CHECK_SAVE) {
        rainbowColor.hue = upref.hue;
        rainbowColor.saturation = upref.saturation;
      }
      else rainbowColor = CHSV(0,255,RCV);
    }
    resetColor(false);
  }
  else if (in == COLOR_MODE_DWN) {
    if (colorMode - 1 < 0) colorMode = 2;
    else colorMode -= 1;
    if (colorMode == COLORMODE_SOLID)
    {
      if (upref.saved == CHECK_SAVE) {
        rainbowColor.hue = upref.hue;
        rainbowColor.saturation = upref.saturation;
      }
      else rainbowColor = CHSV(0,255,RCV);
    }
    resetColor(false);
  }
  else if (in == SATURATION_UP) {
    if (rainbowColor.saturation + 20 >= 255)
    rainbowColor.saturation = 255;
    else rainbowColor.saturation += 20;
    if (configMode == EDIT_MODE and colorMode == COLORMODE_USER)
      setButtonGroup(buttonEditing, false);
    resetColor(colorMode == COLORMODE_RAINBOW);
  }
  else if (in == SATURATION_DOWN) {
    if (rainbowColor.saturation - 20 <= 0)
    rainbowColor.saturation = 0;
    else rainbowColor.saturation -= 20;
    if (configMode == EDIT_MODE and colorMode == COLORMODE_USER)
      setButtonGroup(buttonEditing, false);
    resetColor(colorMode == COLORMODE_RAINBOW);
  }
  else if (in == BRIGHT_UP) {
    if (brightness + 10 >= MAX_BRIGHTNESS)
    brightness = MAX_BRIGHTNESS;
    else brightness += 10;
    FastLED.setBrightness(brightness);
    FastLED.show();
  }
  else if (in == BRIGHT_DOWN) {
    if (brightness - 10 <= 0)
    brightness = 0;
    else brightness -= 10;
    FastLED.setBrightness(brightness);
    FastLED.show();
  }
  else if (in == NEXT_BUTTON) {
    if (buttonEditing + 1 > 5) buttonEditing = 0;
    else buttonEditing += 1;
    rainbowColor = CHSV(0,255,RCV);
    setButtonGroup(buttonEditing, true);
  }
  else if (in == PREV_BUTTON) {
    if (buttonEditing - 1 < 0) buttonEditing = 5;
    else buttonEditing -= 1; 
    rainbowColor = CHSV(0,255,RCV);
    setButtonGroup(buttonEditing, true);
  }
  else if (in == HUE_UP) {
    rainbowColor.hue += 15;
    if (rainbowColor.hue > 255) rainbowColor.hue = 0;
    if (configMode == EDIT_MODE and colorMode == COLORMODE_USER)
      setButtonGroup(buttonEditing, false);
    resetColor(colorMode == COLORMODE_RAINBOW);
  }
  else if (in == HUE_DOWN) {
    rainbowColor.hue -= 15;
    if (rainbowColor.hue < 0) rainbowColor.hue = 255;
    if (configMode == EDIT_MODE and colorMode == COLORMODE_USER)
      setButtonGroup(buttonEditing, false);
    resetColor(colorMode == COLORMODE_RAINBOW);
  }
  else if (in == RAINBOW_UP) {
    rainbowTime += 10;
    if (rainbowTime > 60) rainbowTime = 60;
  }
  else if (in == RAINBOW_DOWN) {
    rainbowTime -= 10;
    if (rainbowTime <= 10) rainbowTime = 10;
  }
  else if (in == ENABLE_CONFIG) {
    configMode = CONFIG_MODE;
    switchIndicator(CRGB::Teal);
    resetColor(false);
  }
  else if (in == ENABLE_EDIT) {
    configMode = EDIT_MODE;
    if (colorMode == COLORMODE_USER) {
      rainbowColor = CHSV(0,255,RCV); // Set to red
      editMode = EDIT_CHOOSE;
      setButtonGroup(buttonEditing, true);
    }
    else if (colorMode == COLORMODE_SOLID) {
      editMode = EDIT_HUE;
    }
    switchIndicator(CRGB::Pink);
    resetColor(false);
  }
  else if (in == ENABLE_BRIGHTNESS) {
    configMode = BRIGHTNESS_MODE;
    switchIndicator(CRGB::Yellow);
    resetColor(false);
  }
  else if (in == ENABLE_HUESELECT) {
    editMode = EDIT_HUE;
    if (colorMode == COLORMODE_USER)
    {
      setButtonGroup(buttonEditing, false);
    }
    switchIndicator(CRGB::Pink);
    resetColor(false);
  }
  else if (in == ENABLE_SATSELECT) {
    editMode = EDIT_SATURATION;
    if (colorMode == COLORMODE_USER)
    {
      setButtonGroup(buttonEditing, false);
    }
    switchIndicator(CRGB::Red);
    resetColor(false);
  }
  else if (in == SAVE_ALL)  {
    upref.saved = CHECK_SAVE;
    upref.brightness = brightness;
    upref.colorMode = colorMode;
    upref.rainbowTime = rainbowTime;
    upref.hue = rainbowColor.hue;
    upref.saturation = rainbowColor.saturation;
    for (byte i = 0; i < LED_COUNT; i++) {
      upref.ledPreset[i] = ledPreset[i];
    }
    
    EEPROM.put(0, upref);
    delay(100);
    configMode = DEFAULT_MODE;
    switchIndicator(CRGB::White);
    resetColor(false);
  }

}

void switchIndicator(CRGB col) {
  FastLED.setBrightness(25);
  for (byte i = 0; i < 2; i++)
  {
    fill_solid(leds, LED_COUNT, col);
    FastLED.show();
    delay(500);
    FastLED.clear();
    FastLED.show();
    delay(500);
  }
  FastLED.setBrightness(brightness);
}

void setNextColors(CRGB colors[LED_COUNT]) {
  for (byte i = 0; i < LED_COUNT; i++) {
    lastLeds[i] = leds[i];
    nextLeds[i] = colors[i];
  }
  resetColor(false);
}

void setNextColor(CRGB color) {
  for (byte i = 0; i < LED_COUNT; i++) {
    lastLeds[i] = leds[i];
    nextLeds[i] = color;
  }
  resetColor(colorMode==COLORMODE_RAINBOW);
}

// Button groups
// 0,1 are A button
// 2 are B button
// 3 reserved for Power indicator for debug

void setButtonGroup(byte buttonGroup, bool loadDefault) {
    
  for (byte i = 0; i < LED_COUNT; i++) {
    editPresetLeds[i] = CHSV(0,25,25);
  }
  
  if (buttonGroup == A_EDIT) {
    if (loadDefault) {
      editPresetLeds[0] = ledPreset[0];
    }
    else {
      editPresetLeds[0] = rainbowColor;
      ledPreset[0] = rainbowColor;
    }
  }
  else if (buttonGroup == B_EDIT) {
    if (loadDefault) {
      editPresetLeds[1] = ledPreset[1];
    }
    else {
      editPresetLeds[1] = rainbowColor;
      ledPreset[1] = rainbowColor;
    }
  }
  else if (buttonGroup == DPAD_EDIT) {
    if (loadDefault) {
      editPresetLeds[4] = ledPreset[4];
      editPresetLeds[5] = ledPreset[5];
      editPresetLeds[6] = ledPreset[6];
      editPresetLeds[7] = ledPreset[7];
    }
    else {
      editPresetLeds[4] = rainbowColor;
      ledPreset[4] = rainbowColor;
      editPresetLeds[5] = rainbowColor;
      ledPreset[5] = rainbowColor;
      editPresetLeds[6] = rainbowColor;
      ledPreset[6] = rainbowColor;
      editPresetLeds[7] = rainbowColor;
      ledPreset[7] = rainbowColor;
    }
  }
  else if (buttonGroup == START_EDIT) {
    if (loadDefault) {
      editPresetLeds[2] = ledPreset[2];
    }
    else {
      editPresetLeds[2] = rainbowColor;
      ledPreset[2] = rainbowColor;
    }
  }
  else if (buttonGroup == SEL_EDIT) {
    if (loadDefault) {
      editPresetLeds[3] = ledPreset[3];
    }
    else {
      editPresetLeds[3] = rainbowColor;
      ledPreset[3] = rainbowColor;
    }
  }
  else if (buttonGroup == POWER_EDIT) {
    if (loadDefault) {
      editPresetLeds[8] = ledPreset[8];
    }
    else {
      editPresetLeds[8] = rainbowColor;
      ledPreset[8] = rainbowColor;
    }
  }
  resetColor(false);
}

void colorTick(float changeTime) { // Handle fading between colors for transition purposes
  if (colorDirty and !colorSet) {

    colorIncrement += (255 / changeTime);
    byte inc = (byte) colorIncrement;

    for (byte i = 0; i < LED_COUNT; i++) { // Set the next color on all LEDs blended
      leds[i] = blend(lastLeds[i], nextLeds[i], inc);
    }

    if (inc >= 255) { // If the fade is complete, reset the increment and set as not dirty!
      for (byte i = 0; i < LED_COUNT; i++) {
        leds[i] = nextLeds[i]; // Set all colors to their final form
      }
      colorIncrement = 0;
      colorDirty = false; // Unmark that the color is dirty
      colorSet = true; // Mark that the color has been 'set'.
    }

  }
  FastLED.show(); // Send update to LEDs
}

void resetColor(bool init) {
  FastLED.setBrightness(brightness);
  colorDirty = true;
  colorSet = false;
  colorInit = init;
}

void idle(byte pressCount) {
  // COLOR RUNTIME
  if (!colorSet and colorDirty and !colorInit) {
    fadeTime = COLOR_TIME;
    if (colorMode == COLORMODE_USER) {
      if (configMode == EDIT_MODE)
      {
        setNextColors(editPresetLeds);
      }
      else setNextColors(ledPreset);
      
    }
    else if (colorMode == COLORMODE_SOLID) {
      setNextColor(rainbowColor);
    }
    else if (colorMode == COLORMODE_RAINBOW and !colorInit) {
      fadeTime = rainbowTime;
      rainbowColor = CHSV(0,255,RCV);
      setNextColor(rainbowColor);
      colorInit = true;
    }
  }
  else if (colorSet and (colorMode == COLORMODE_RAINBOW) and !colorDirty and colorInit) { // Special rule for rainbow mode
    fadeTime = rainbowTime;
    buttonFunction(HUE_UP);
    setNextColor(rainbowColor);
    colorInit = true;
  }

  colorTick(fadeTime);

  // USER INTERFACE
  if (configMode == DEFAULT_MODE) {
    if (pressCount == HELD_THREE) buttonFunction(ENABLE_CONFIG);
  }
  else if (configMode == CONFIG_MODE) {
    if (dpadPresses == DPAD_LEFT) buttonFunction(COLOR_MODE_DWN);
    else if (dpadPresses == DPAD_RIGHT) buttonFunction(COLOR_MODE_UP);
    else if (pressCount == HELD_THREE) buttonFunction(ENABLE_EDIT);
  }
  else if (configMode == BRIGHTNESS_MODE) {
    if (dpadPresses == DPAD_LEFT) buttonFunction(BRIGHT_DOWN);
    else if (dpadPresses == DPAD_RIGHT) buttonFunction(BRIGHT_UP);
    else if (pressCount == HELD_THREE) buttonFunction(SAVE_ALL);
  }
  else if (configMode == EDIT_MODE) {

    // user preset mode edit options
    if (colorMode == COLORMODE_USER) {
      if (editMode == EDIT_CHOOSE) {
        if (pressCount == HELD_TWO) buttonFunction(ENABLE_HUESELECT);
        else if (dpadPresses == DPAD_LEFT) buttonFunction(PREV_BUTTON);
        else if (dpadPresses == DPAD_RIGHT) buttonFunction(NEXT_BUTTON);
      }
      else if (editMode == EDIT_HUE) {
        if (dpadPresses == DPAD_LEFT) buttonFunction(HUE_DOWN);
        else if (dpadPresses == DPAD_RIGHT) buttonFunction(HUE_UP);
        else if (pressCount == HELD_TWO) buttonFunction(ENABLE_SATSELECT);
      }
      else if (editMode == EDIT_SATURATION) {
        if (dpadPresses == DPAD_LEFT) buttonFunction(SATURATION_DOWN);
        else if (dpadPresses == DPAD_RIGHT) buttonFunction(SATURATION_UP);
        else if (pressCount == HELD_TWO) buttonFunction(ENABLE_EDIT);
      }
    } 
    // ----------------
    // ----------------

    // solid color edit mode
    else if (colorMode == COLORMODE_SOLID) {
      if (editMode == EDIT_HUE) {
        if (dpadPresses == DPAD_LEFT) buttonFunction(HUE_DOWN);
        else if (dpadPresses == DPAD_RIGHT) buttonFunction(HUE_UP);
        else if (pressCount == HELD_TWO) buttonFunction(ENABLE_SATSELECT);
      }
      else if (editMode == EDIT_SATURATION) {
        if (dpadPresses == DPAD_LEFT) buttonFunction(SATURATION_DOWN);
        else if (dpadPresses == DPAD_RIGHT) buttonFunction(SATURATION_UP);
        else if (pressCount == HELD_TWO) buttonFunction(ENABLE_HUESELECT);
      }
    }
    // ----------------
    // ----------------

    // rainbow color edit mode
    else if (colorMode == COLORMODE_RAINBOW) {
      if (dpadPresses == DPAD_LEFT) buttonFunction(RAINBOW_DOWN);
      else if (dpadPresses == DPAD_RIGHT) buttonFunction(RAINBOW_UP);
    }

    if (pressCount == HELD_THREE) buttonFunction(ENABLE_BRIGHTNESS);
  }

}
