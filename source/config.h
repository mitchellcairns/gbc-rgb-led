// USER PREFERENCES STRUCTURE
// ----------------
typedef struct 
{
  byte saved = 0;
  byte brightness;
  byte colorMode;
  byte rainbowTime;
  byte hue;
  byte saturation;
  CRGB ledPreset[4];
} UserPreference;
