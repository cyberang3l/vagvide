#ifndef common_h
#define common_h
#ifdef __cplusplus

#include "Arduino.h"
#include "elapsedMillis.h"

/* Includes for the LCD */
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

#define IN  // Use for marking a function parameter if it is an input
#define OUT // Use for marking a function parameter if it is an output

#define DEBUG 1

/* Define the pin number where different hardware
 * components are connected to
 */
#define RGB_LED_B 2 // must be connected on a PWM pin
#define RGB_LED_G 4 // must be connected on a PWM pin
#define RGB_LED_R 6 // must be connected on a PWM pin
#define SSR_PIN 8   // Solid State Relay must be connected on a PWM pin
#define FLOAT_SWITCH_PIN 22
#define PUMPRELAY_PIN 24
#define TEMP_SENSOR_1_PIN 32   // Temperature sensor Front 1
#define TEMP_SENSOR_2_PIN 34   // Temperature sensor Front 2
#define TEMP_SENSOR_3_PIN 36   // Temperature sensor Rear 1
#define TEMP_SENSOR_4_PIN 38   // Temperature sensor Rear 2
#define PUSH_BTN_MENU_BACK_PIN 40
#define PUSH_BTN_MENU_OK_PIN 42
#define PUSH_BTN_MENU_DOWN_PIN 44
#define PUSH_BTN_MENU_UP_PIN 46

#define LCD_I2C_ADDR 0x27
#define LCD_BACKLIGHT_PIN 3
#define LCD_RS_PIN 0
#define LCD_RW_PIN 1
#define LCD_EN_PIN 2
#define LCD_D4_PIN 4
#define LCD_D5_PIN 5
#define LCD_D6_PIN 6
#define LCD_D7_PIN 7

#define LCD_ON HIGH
#define LCD_OFF LOW

#define LCD_COLS 16 // Change this if your LCD has more columns
#define LCD_ROWS 2  // Change this if your LCD has more rows
#define LCD_COLS_CHAR_LIMIT (LCD_COLS + 1)  // The LCD can print X chars, but a predefined string
                                            //in C has X+1 char because of the termination character

typedef enum _push_buttons {
  BTN_BACK     = (1 << 0),
  BTN_OK       = (1 << 1),
  BTN_DOWN     = (1 << 2),
  BTN_UP       = (1 << 3),
  BTN_FLOAT_SW = (1 << 4)
} push_buttons;

/* TODO: The degree_symbol only needs to be defined once.
 * No need for extern, because we store the symbol in LCD's
 * memory.
 */
extern byte degree_symbol[8]; // Variable to store the degree symbol (the little superscripted "o")

extern const uint8_t RGB_LED_RED[3];
extern const uint8_t RGB_LED_GREEN[3];
extern const uint8_t RGB_LED_BLUE[3];
extern const uint8_t RGB_LED_ORANGE[3];
extern const uint8_t RGB_LED_VIOLET[3];
extern const uint8_t RGB_LED_CYAN[3];
extern const uint8_t RGB_LED_OFF[3];

extern LiquidCrystal_I2C lcd;

/***f* software_Reset
 *
 * Restarts program from the very beginning
 */
void software_Reset();

/***f* pump_operate
 *
 * Turns on or off the pump
 */
void pump_operate(IN bool on);

/***f* ssr_operate
 *
 * Turns on/off the solid state relay.
 * Since the SSR is connected on a PWM pin,
 * we use the analogWrite to drive it.
 */
void ssr_operate(IN uint8_t analog_on);

/***f* _setRgbLedColor
 *
 * Sets the color in the RGB led
 */
static void _setRgbLedColor(IN uint8_t R,
                            IN uint8_t G,
                            IN uint8_t B);

/***f* setRgbLed
 *
 * Sets the color in the RGB led
 * RGB must be a 3-bytes array.
 *
 * Since the LED has a negative logic, meaning
 * that 255 is off and 0 is maximum luminance,
 * this function will reverse all the values
 * accordingly.
 */
void setRgbLed(IN const uint8_t* RGB);

/***f*
 *
 * Reads a single input button.
 */
uint8_t readButton(push_buttons button);

/***f*
 *
 * Reads all the input buttons plus the floating switch
 * that essentially works as a button.
 */
uint8_t readButtons();

/***f* deviceIsInWater
 *
 * Function that returns true if the floating switch is pressed
 * or false otherwise. This function can be used to determine if the
 * device is in the water or not.
 */
bool deviceIsInWater(IN uint8_t pressedButtons);

/***f* isLcdBacklightOn
 *
 * returns the value of the variable lcdBacklightIsOn
 */
bool isLcdBacklightOn();

/***f* setLcdBacklight
 *
 * Use this function to change the status of the backlight.
 * Do not change it directly from the lcd object because this function
 * takes care of the lcdBacklightIsOn variable as well.
 */
void setLcdBacklight(IN uint8_t backlight_on);

/***f* printLcdLine
 *
 * I do not want to use the lcd.clear() function because it is very
 * slow, so what I do is to pass the char array that needs to be printed
 * in this function, the string is printed and filled until the end of
 * each line with white spaces.
 *
 * If we want to print only one line of the string, then choose the line
 * with the printLineNo parameter. The first line is 1, 2nd line is 2 and
 * so on. When printLineNo == 0, all lines in the str array will be printed
 *
 * Whenever you call this function call like like this:
 *
 * char *str[LCD_ROWS] = {"Press the \"OK\"", "button to start"};
 * printLcdLine(str);
 */
void printLcdLine(IN const char * const str[LCD_ROWS],
                  IN uint8_t printLineNo = 0);

#endif // endif __cpluscplus
#endif // endif common_h
