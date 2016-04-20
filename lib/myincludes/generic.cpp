#include "common.h"

const uint8_t RGB_LED_RED[3] = {160, 0, 0};
const uint8_t RGB_LED_GREEN[3] = {0, 160, 0};
const uint8_t RGB_LED_ORANGE[3] = {200, 135, 0};
const uint8_t RGB_LED_OFF[3] = {0, 0, 0};

/* lcdBacklightIsOn must be in sync with the current status of the backlight.
 * When the backlight is off and we press any button, we just turn on the backlight
 * and do not perform any operation (we just show the current temperature to the
 * user, for example). The user can operate the menu with buttons only when the
 * backlight is on.
 */
static bool lcdBacklightIsOn = false;

/* The order of the button pins must be matching
 * the order of the push_buttons enum
 */
static const uint8_t buttonPins[] = {
  PUSH_BTN_MENU_BACK_PIN,
  PUSH_BTN_MENU_OK_PIN,
  PUSH_BTN_MENU_DOWN_PIN,
  PUSH_BTN_MENU_UP_PIN,
  FLOAT_SWITCH_PIN
};

static const uint8_t numButtons = sizeof(buttonPins) / sizeof(uint8_t);

byte degree_symbol[8] = {
  B00110, B01001, B01001, B00110,
  B00000, B00000, B00000, B00000
};

void software_Reset() {
  asm volatile ("  jmp 0");
}

void pump_operate(IN bool on) {
 if(on)
  digitalWrite(PUMPRELAY_PIN, LOW);
 else
  digitalWrite(PUMPRELAY_PIN, HIGH);
}

void ssr_operate(IN uint8_t analog_on) {
   analogWrite(SSR_PIN, analog_on);
}

static void _setRgbLedColor(IN byte R,
                            IN byte G,
                            IN byte B) {
    analogWrite(RGB_LED_R, R);
    analogWrite(RGB_LED_G, G);
    analogWrite(RGB_LED_B, B);
}

void setRgbLed(IN const uint8_t* RGB) {
  _setRgbLedColor(255 - RGB[0],
                  255 - RGB[1],
                  255 - RGB[2]);
}

uint8_t readButtons() {
   uint8_t reply = 0x1F;

   for (uint8_t i = 0; i < numButtons; i++)
     reply &= ~((digitalRead(buttonPins[i])) << i);

   /* The float switch has reverse logic when compared to the other
    * buttons. So flip the bit that represents the float switch, so
    * that the reply returns zero when the float switch is out of
    * the water and one when it's in the water (pressed)
    */
   reply ^= (BTN_FLOAT_SW);

   return reply;
}

bool deviceIsInWater(IN uint8_t pressedButtons) {
  if (pressedButtons & BTN_FLOAT_SW)
    return true;
  else
    return false;
}

bool isLcdBacklightOn() {
  return lcdBacklightIsOn;
}

void setLcdBacklight(IN uint8_t backlight_on) {
  if (backlight_on) {
    lcd.setBacklight(HIGH);
    lcdBacklightIsOn = true;
  } else {
    lcd.setBacklight(LOW);
    lcdBacklightIsOn = false;
  }
}

void printLcdLine(IN const char * const str[LCD_ROWS]) {
  char line_buffer[LCD_COLS_CHAR_LIMIT];
  char whiteSpaceFiller[LCD_COLS];
  memset(whiteSpaceFiller, ' ', LCD_COLS * sizeof(char));

  for (uint8_t r = 0; r < LCD_ROWS; r++) {
    lcd.setCursor(0, r);
    strcpy_P(line_buffer, (char *)pgm_read_word(&(str[r])));
    lcd.print(line_buffer);
    /* if str[r] length is less than LCD_COLS, the following line will
     * fill the remaining LCD blocks in line 'r' with whitespaces */
    lcd.print(whiteSpaceFiller);
  }
}
