/* Common things between files such as generic helper
 * functions and definitions
 */
#include "common.h"
/* Custom network functions */
#include "network.h"
/* Custom functions/data-structs related to the temperature sensors */
#include "temperature.h"

/* The PID and PID Autotune library */
#include <PID_v1.h>
#include <PID_AutoTune_v0.h>

/* Includes for the LCD */
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

#define TIME_BETWEEN_MEASUREMENTS 1000 // In milliseconds

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
/* I want to have an operating state for the LCD, but I want it to
 * be non-blocking because I want to be able to serve the network
 * at all times.
 * So as long as we are not in the "OPSTATE_OFF_TURN_ON", the SousVide is
 * running, but displays different things according to the current state.
 * If the user doesn't press a button for X (X is defined by the variable
 * menuReturnTimeOut) seconds, return to the OPSTATE_MENU_TEMP state and
 * show the current and desired temperatures.
 *
 * In order to setup the temperature, the user can click the
 * OK button when in the OPSTATE_MENU_TEMP and the menu will transition
 * in the OPSTATE_MENU_TEMP_SETUP where by using the UP and DOWN buttons the
 * user can set the desired temperature. The OK button has to be pressed
 * in order to save the changes, or the BACK to abort. It the user doesn't
 * press anything for X seconds, the operation is aborted.
 *
 * The temperature can also be set by choosing a preset from the web interface
 * TODO: finish the description of the operation.
 *       Add a menu to show the IP address settings/link state
 */
enum operatingState {
  OPSTATE_OFF_TURN_ON = 0,      // Displays a Press OK to turn ON
  OPSTATE_ON_TURN_OFF,          // If Sous Vide is ON, display Press OK to turn Off
  OPSTATE_MENU_TEMP,            // This opstate displays the current and the desired
                                // temperatures.
  OPSTATE_MENU_TEMP_SETUP,      // We can jump in this menu only from OPSTATE_TEMP_MENU
  OPSTATE_MENU_PRESET,          // Just displays the menu label: "Presets"
  OPSTATE_MENU_PRESET_CHOOSE    // With the UP/DOWN buttons navigate through the presets
                                // and with the OK button select the current preset.
                                // New presets can be defined only from the web interface.
};

/* The variable opState keeps the current state */
operatingState opState = OPSTATE_OFF_TURN_ON;

/* The variable lastTimeButtonWasPressed is updated with the time in millis since
 * the last time a button was pressed. We use this variable to determine when the
 * time we stay in a menu has timed out in order to return to the OPSTATE_MENU_TEMP
 * and show the temperature, as well as know when we have to turn off the lcd
 * backlight.
 */
unsigned long lastTimeButtonWasPressed;

/* TODO: Allow the user to configure the LCD backlight timeout from the web
 *       interface. A value of -1 means that the backlight should be permanently
 *       on. Otherwise, the number should represent the number in seconds that
 *       the backlight will be staying on after a button has been pressed.
 *
 * The variable lcdBacklightTimeOut is used to configure the number of seconds
 * that the backlight will be staying on after a button has been pressed.
 *
 * The variable menuReturnTimeOut is used to configure the timeout for staying in
 * a menu without any user action (without the user pressing a button). After this
 * timeout, we return to the OPSTATE_MENU_TEMP
 */
int32_t lcdBacklightTimeOut = 30000; /* in milliseconds */
uint16_t menuReturnTimeOut = 30000;  /* In milliseconds */

/* lcdBacklightIsOn must be in sync with the current status of the backlight.
 * When the backlight is off and we press any button, we just turn on the backlight
 * and do not perform any operation (we just show the current temperature to the
 * user, for example). The user can operate the menu with buttons only when the
 * backlight is on.
 */
bool lcdBacklightIsOn = false;

/* Use the variable netInitialized to know when the network has been initialized */
bool netInitialized = false;

/* PID variables */
/* TODO: Make the PID variables "variable" and read them from EEPROM */
double PID_Output;

/* Instantiate the PID */
/* Specify the links and initial tuning parameters */
PID SousPID(&current_temperature, &PID_Output, &desired_temperature, 850, 0.5, 0.1, DIRECT);
//PID SousPID(&current_temperature, &PID_Output, &desired_temperature, 2, 5, 1, DIRECT);

/* Instantiate the LCD */
LiquidCrystal_I2C lcd(LCD_I2C_ADDR, LCD_EN_PIN, LCD_RW_PIN,
  LCD_RS_PIN, LCD_D4_PIN, LCD_D5_PIN, LCD_D6_PIN,
  LCD_D7_PIN, LCD_BACKLIGHT_PIN, POSITIVE);

/************************************************************************************/
/********************************** MAIN FUNCTIONS **********************************/
/************************************************************************************/

/***f* setLcdBacklight
 *
 * Use this function to change the status of the backlight.
 * Do not change it directly from the lcd object because this function
 * takes care of the lcdBacklightIsOn variable as well.
 */
 void setLcdBacklight(uint8_t backlight_on) {
   if (backlight_on) {
     lcd.setBacklight(HIGH);
     lcdBacklightIsOn = true;
   } else {
     lcd.setBacklight(LOW);
     lcdBacklightIsOn = false;
   }
 }

/***f* on
 *
 * The Sous Vide is OFF and we may want to turn it on.
 * This function only works if the current opstate is
 * in the state OPSTATE_OFF_TURN_ON
 */
void on(IN uint8_t buttonsPressed) {
  if (opState == OPSTATE_OFF_TURN_ON) {

  }
}

/***f* off
 *
 * The Sous Vide is ON and we may want to turn it off.
 * This function only works if the current opstate is
 * in the state OPSTATE_ON_TURN_OFF
 */
void off(IN uint8_t buttonsPressed) {
  /* The Sous Vide is ON and we may want to turn it off */
  if (opState == OPSTATE_ON_TURN_OFF) {
    if (buttonsPressed & BTN_OK) {
      pump_operate(false);
      //SousPID.SetMode(MANUAL);
      ssr_operate(0);
      setLcdBacklight(LCD_OFF);
      setRgbLed(RGB_LED_OFF);
    }
  }
}

/***f* setup
 *
 * Default Arduino setup function
 */
void setup() {
  /* Define INPUT/OUTPUT Pins */
  pinMode(PUSH_BTN_MENU_BACK_PIN, INPUT);
  pinMode(PUSH_BTN_MENU_OK_PIN, INPUT);
  pinMode(PUSH_BTN_MENU_DOWN_PIN, INPUT);
  pinMode(PUSH_BTN_MENU_UP_PIN, INPUT);
  pinMode(SSR_PIN, OUTPUT);
  pinMode(PUMPRELAY_PIN, OUTPUT);
  pump_operate(false);
  pinMode(FLOAT_SWITCH_PIN, INPUT);
  ssr_operate(0);

  /* Initialize the temperature sensors */
  initTempSensors();

  /* Initialize the network but do not configure IP addresses yet
   * We do that in the main loop because we need to react to link
   * state changes
   */
  initNetworkModule();

  //turn the PID on
  SousPID.SetMode(AUTOMATIC);

  /* Initialize the LCD */
  lcd.begin(16, 2); //  <<----- My LCD is 16x2, or 20x4
  lcd.createChar(0, degree_symbol); // Store in byte 0 in LCD the degree_symbol (available bytes are 0-7)

  lcd.home(); // go home

  lcd.print("Xinoula Sola PSI!!");
  lcd.setCursor(0, 1);
  lcd.print("Vangelis");
  lcd.setCursor(9, 1);
  lcd.print("C");
  lcd.write(byte(0));
}

/***f* loop
 *
 * Main Arduino loop function
 */
void loop() {
  /* If the network link is up and the netInitialized == false, we should initialize
   * the network addresses and set the netInitialized to true. If the link is down
   * and the netInitialized == true, we need to set the netInitialized to false again.
   *
   * If the network is initialized and the link is up then process ethernet packets.
   */
  if (netInitialized == false && eth_link_state_up() == true) {
    initNetworkAddr();
    netInitialized = true;
    Serial.println("Ethernet link is up.");
  } else if (netInitialized == true) {
    if (eth_link_state_up() == false) {
      netInitialized = false;
      Serial.println("Ethernet link is down.");
    } else {
      /* Copy received packets to data buffer Ethernet::buffer
       * and return the uint16_t Size of received data (which is needed by
       * ether.packetLoop). */
      uint16_t len = ether.packetReceive();
      /* Parse received data and return the uint16_t Offset of TCP payload data
       * in data buffer Ethernet::buffer, or zero if packet processed */
      uint16_t pos = ether.packetLoop(len);

      processEthernetPacket(pos);
    }
  }

  /* Now read the buttons */
  uint8_t buttonsPressed = 0;
  buttonsPressed = readButtons();

  /* Check the lcdBacklightTimeOut */
  if (buttonsPressed != 0 && buttonsPressed != BTN_FLOAT_SW) {
    /* If a button is pressed, and this button is not only the
     * floating switch, switch on the backlight. */
    lastTimeButtonWasPressed = millis();
    setLcdBacklight(LCD_ON);
  } else {
    /* Switch off the backlight if a button hasn't been pressed for
     * lcdBacklightTimeOut milliseconds. */
    if (millis() - lastTimeButtonWasPressed > lcdBacklightTimeOut)
      setLcdBacklight(LCD_OFF);
  }

  /* Check the menuReturnTimeOut only if the Sous Vide is running,
   * i.e. is no in the opState OPSTATE_OFF_TURN_ON */
  if ((millis() - lastTimeButtonWasPressed > menuReturnTimeOut) &&
      (opState != OPSTATE_OFF_TURN_ON)) {
    opState = OPSTATE_MENU_TEMP;
  }

  /* And based on the current state, call the corresponding functions */
  switch (opState) {
    case OPSTATE_OFF_TURN_ON:
      on(buttonsPressed);
      break;
    case OPSTATE_ON_TURN_OFF:
      off(buttonsPressed);
      break;
    case OPSTATE_MENU_PRESET:
    case OPSTATE_MENU_PRESET_CHOOSE:
      break;
    case OPSTATE_MENU_TEMP:
    case OPSTATE_MENU_TEMP_SETUP:
    default:
      break;
  }

  return;
  //Serial.print("Buttons: ");
  //Serial.println(readButtons());

  /* If the float_switch is out of the water, then turn off
   * the pump and SSR
   */
  if (!(buttonsPressed & BTN_FLOAT_SW))
  {
    setRgbLed(RGB_LED_RED);
    pump_operate(false);
    ssr_operate(0);
  }

  desired_temperature = 36.6;

  /* If the elapsed time since the last measurement is greater than
   * TIME_BETWEEN_MEASUREMENTS, send a new requestTemperature to all
   * of the sensors. Since we do not wait for conversion in this spot
   * (in the setup function I disable the waitForConversion) in order
   * to avoid blocking the program, before we read the actual
   * temperatures and use them, we have to make sure that the conversion
   * has finished.
   */
  if (timeElapsedSinceLastMeasurement > TIME_BETWEEN_MEASUREMENTS) {
    for (int i = 0; i < numSensors; i++)
      temp_sensor[i].requestTemperatures();
    timeElapsedSinceLastMeasurement = 0;
  }

  /* If the floating switch is on, meaning that the Sous Vide is in the water,
   * only then allow the water pump and the immersion heaters to work.
   */
  if(buttonsPressed & BTN_FLOAT_SW) {
    pump_operate(true);
    readAllTemperatures();
    Serial.print("Average temperature: ");
    Serial.println(avg_temperature);
    current_temperature = avg_temperature;
    lcd.setCursor(11, 1);
    lcd.print(current_temperature, DEC);
    SousPID.Compute();
    Serial.print("PID Output: ");
    Serial.println(PID_Output);
    ssr_operate(PID_Output);
    // if (PID_Output && (abs(desired_temperature - current_temperature) > 0.))
    //   pump_operate(true);
    // else
    //   pump_operate(false);
  }
  //}
}
