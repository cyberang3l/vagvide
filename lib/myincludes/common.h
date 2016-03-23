#ifndef common_h
#define common_h
#ifdef __cplusplus

#include "Arduino.h"
#include "elapsedMillis.h"

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
#define PUSH_BTN_MENU_BACK 40
#define PUSH_BTN_MENU_OK 42
#define PUSH_BTN_MENU_DOWN 44
#define PUSH_BTN_MENU_UP 46

/* TODO: The degree_symbol only needs to be defined once.
 * No need for extern, because we store the symbol in LCD's
 * memory.
 */
extern byte degree_symbol[8]; // Variable to store the degree symbol (the little superscripted "o")

extern const uint8_t RGB_LED_GREEN[3];
extern const uint8_t RGB_LED_RED[3];
extern const uint8_t RGB_LED_ORANGE[3];

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

#endif // endif __cpluscplus
#endif // endif common_h
