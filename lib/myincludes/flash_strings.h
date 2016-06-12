#include "Arduino.h"
#include "common.h"

/* Read the following link to understand why we define the PROGMEM variables the way we do
 * http://www.atmel.com/webdoc/AVRLibcReferenceManual/pgmspace_1pgmspace_strings.html */

const char str0_1[] PROGMEM = "Press OK";
const char str0_2[] PROGMEM = "to start";

const char * const LCD_STR_PRESS_OK_TO_START[LCD_ROWS] PROGMEM = {str0_1, str0_2};

const char str1_1[] PROGMEM = "Please put";
const char str1_2[] PROGMEM = "device in water";
const char str2_1[] PROGMEM = "or press OK";
const char str2_2[] PROGMEM = "to turn off";

const char * const LCD_STR_PUT_DEVICE_IN_WATER[2][LCD_ROWS] PROGMEM = {
  {str1_1, str1_2},
  {str2_1, str2_2}
};

const char str3[] PROGMEM = "Current Temp";
const char str4[] PROGMEM = "Target Temp";
const char str5[] PROGMEM = "C             ";

const char * const LCD_DISPLAY_TEMPERATURE[2][LCD_ROWS] PROGMEM = {
  {str3, str5},
  {str4, str5}
};

const char str4_1[] PROGMEM = "Set target temp";

const char * const LCD_STR_SET_TARGET_TEMP[LCD_ROWS] PROGMEM = {str4_1, str5};

const char str6_1[] PROGMEM = "Turn off";
const char str6_2[] PROGMEM = "device";
const char str7_1[] PROGMEM = "Setup target";
const char str7_2[] PROGMEM = "temperature";
const char str8_1[] PROGMEM = "Choose cooking";
const char str8_2[] PROGMEM = "preset";
const char str9_1[] PROGMEM = "Show network";
const char str9_2[] PROGMEM = "settings";

const char * const LCD_TOP_LEVEL_MENU_LABELS[4][LCD_ROWS] PROGMEM = {
  {str6_1, str6_2},
  {str7_1, str7_2},
  {str8_1, str8_2},
  {str9_1, str9_2}
};


const char str10_1[] PROGMEM = "Development";
const char str10_2[] PROGMEM = "mode is now ON";

const char * const LCD_STR_DEVMODE_NOW_ON[LCD_ROWS] PROGMEM = {str10_1, str10_2};


const char str11_1[] PROGMEM = "Initializing";
const char str11_2[] PROGMEM = "network";
const char str11_3[] PROGMEM = "temp sensors";

const char * const LCD_STR_INIT_NETWORK[LCD_ROWS] PROGMEM = {str11_1, str11_2};
const char * const LCD_STR_INIT_TEMP_SENSORS[LCD_ROWS] PROGMEM = {str11_1, str11_3};
