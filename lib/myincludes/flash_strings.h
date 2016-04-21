#include "Arduino.h"
#include "common.h"

/* Read the following link to understand why we define the PROGMEM variables the way we do
 * http://www.atmel.com/webdoc/AVRLibcReferenceManual/pgmspace_1pgmspace_strings.html */

const char str1[] PROGMEM = "Press OK";
const char str2[] PROGMEM = "to start";

const char * const LCD_STR_PRESS_OK_TO_START[LCD_ROWS] PROGMEM = {str1, str2};

const char str3[] PROGMEM = "Please put";
const char str4[] PROGMEM = "device in water";
const char str5[] PROGMEM = "or press OK";
const char str6[] PROGMEM = "to turn off";

const char * const LCD_STR_PUT_DEVICE_IN_WATER[2][LCD_ROWS] PROGMEM = {
  {str3, str4},
  {str5, str6}
};

const char str7[] PROGMEM = "Current Temp";
const char str8[] PROGMEM = "Target Temp";
const char str9[] PROGMEM = "C             ";

const char * const LCD_DISPLAY_TEMPERATURE[2][LCD_ROWS] PROGMEM = {
  {str7, str9},
  {str8, str9}
};

const char str10[] PROGMEM = "Turn off";
const char str11[] PROGMEM = "device";
const char str12[] PROGMEM = "Setup target";
const char str13[] PROGMEM = "temperature";
const char str14[] PROGMEM = "Choose cooking";
const char str15[] PROGMEM = "preset";
const char str16[] PROGMEM = "Show network";
const char str17[] PROGMEM = "settings";

const char * const LCD_TOP_LEVEL_MENU_LABELS[4][LCD_ROWS] PROGMEM = {
  {str10, str11},
  {str12, str13},
  {str14, str15},
  {str16, str17}
};


// const char LCD_STR_PRESS_OK_TO_START[LCD_ROWS][LCD_COLS] PROGMEM = {"Press the \"OK\"", "button to start"};
// const char *LCD_STR_PUT_DEVICE_IN_WATER[LCD_ROWS] = {"Please put", "device in water"};
// const char *LCD_STR_PUT_DEVICE_IN_WATER_2[LCD_ROWS] = {"Or press \"OK\"", "to turn off"};
//
// const char *LCD_DISPLAY_TEMPERATURE[2][LCD_ROWS] = {
//   {"Current Temp", "C               "},
//   {"Target Temp", "C               "}
// };
//
// const char *LCD_TOP_LEVEL_MENU_LABELS[4][LCD_ROWS] = {
//   {"Turn off", "device"},
//   {"Setup target", "temperature"},
//   {"Choose cooking", "preset"},
//   {"Show network", "settings"}
// };
