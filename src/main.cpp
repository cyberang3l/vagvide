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

#define TIME_BETWEEN_MEASUREMENTS 1000 // In milliseconds

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
unsigned long lastTimeButtonWasPressed = millis();

/* The variable lastTimeExecutedOpStateFunc is updated with the time in millis
 * since the last time an opState function was executed. We do not want to
 * to execute these functions more than 10 times a second (every 100ms) because
 *  1) We have a delay for button reading (otherwise with one button press we
 *     will be recording multiple button presses)
 *  2) The LCD takes some time to clear and get rewritten.
 *
 * We use this variable instead of having delays, because we do not want to block
 * the microntroller since it is service the ethernet and other things.
 */
unsigned long lastTimeExecutedOpStateFunc = millis();

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
int32_t lcdBacklightTimeOut = 20000; /* in milliseconds */
uint16_t menuReturnTimeOut = 30000;  /* In milliseconds */

/* Use the variable netInitialized to know when the network has been initialized */
bool netInitialized = false;

/* Sometimes I may need to print larger messages that cannot fit in one go
 * in a 2x16 LCD. In this case I want to be able to alternate through the
 * messages every 2000ms and the variable "lastTimeAlternatedMessage" keeps
 * a log of when the last alternation happened. This variable is getting checked
 * by the function increaseMessageAlternationIndexes.
 */
unsigned long lastTimeAlternatedMessage = millis();

/* If we want to alternate messages, we need an alternation index for each series
 * of messages that we want to alter. Then with a mod operation, we can display
 * different messages in order. This index is for the messages:
 * LCD_STR_PUT_DEVICE_IN_WATER and LCD_STR_OR_TURN_OFF.
 * Add all the alternation indexes in the function increaseMessageAlternationIndexes
 */
uint8_t deviceInWaterOrTurnOffAlternationIndex = 0;

char *LCD_STR_PRESS_OK_TO_START[LCD_ROWS] = {"Press the \"OK\"", "button to start"};
char *LCD_STR_PUT_DEVICE_IN_WATER[LCD_ROWS] = {"Please put", "device in water"};
char *LCD_STR_OR_TURN_OFF[LCD_ROWS] = {"Or press \"OK\"", "to turn off"};

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

/***f* increaseMessageAlternationIndexes
 *
 * Function to increase all the alternation indexes
 * every 2 seconds (change the 2000ms in the if statement
 * if you want larger alternation interval)
 */
void increaseMessageAlternationIndexes() {
  if (millis() - lastTimeAlternatedMessage > 2000) {
    deviceInWaterOrTurnOffAlternationIndex++;
    lastTimeAlternatedMessage = millis();
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
    /* Turn on the device only if it is in the water */
    if (deviceIsInWater(buttonsPressed)) {
      printLcdLine(LCD_STR_PRESS_OK_TO_START);
      /* Once the device is in the water, turn it on only if the
       * user has pressed the OK button. */
      if (buttonsPressed & BTN_OK) {
        /* Do things to turn the device on */
        /* TODO: Read the stored desired temperature
         *       and set the opState to OPSTATE_MENU_TEMP
         */
        opState = OPSTATE_MENU_TEMP;
      }
    } else {
      /* Else print in the LCD the message "put device in water" */
      printLcdLine(LCD_STR_PUT_DEVICE_IN_WATER);
    }
  }
}

/***f* off
 *
 * The Sous Vide is ON and we may want to turn it off.
 * This function only works if the current opstate is
 * in the state OPSTATE_ON_TURN_OFF
 */
void off(IN uint8_t buttonsPressed) {
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

/***f* preset
 *
 * We run this function only in the opstates
 * OPSTATE_MENU_PRESET and OPSTATE_MENU_PRESET_CHOOSE
 */
void preset(IN uint8_t buttonsPressed) {
  if (opState == OPSTATE_MENU_PRESET) {
    /* TODO */
  } else if (opState == OPSTATE_MENU_PRESET_CHOOSE) {
    /* TODO */
  }
}

/***f* tempMenu
 *
 * We run this function only in the opstates
 * OPSTATE_MENU_TEMP and OPSTATE_MENU_TEMP_SETUP
 */
unsigned long lastSincePrintTemp = millis();
void tempMenu(IN uint8_t buttonsPressed) {
  if (opState == OPSTATE_MENU_TEMP) {
    /* When I print the Celcius sign very fast, I see some flickering in the LCD
     * That's why I use this timer to not print the temperature so often. At
     * least not until I find a better solution */
    if (millis() - lastSincePrintTemp > 1000) {
      char *str[LCD_ROWS] = {"Xinoula Sola PSI!", "Vangelis C"};
      printLcdLine(str);
      lcd.setCursor(10, 1);
      lcd.write(byte(0));
      lastSincePrintTemp = millis();
    }
  } else if (opState == OPSTATE_MENU_TEMP_SETUP) {
    /* TODO */
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
  lcd.begin(LCD_COLS, LCD_ROWS); //  <<----- My LCD is 16x2, or 20x4
  lcd.createChar(0, degree_symbol); // Store in byte 0 in LCD the degree_symbol (available bytes are 0-7)

  /* Turn on the backlight during initialization */
  setLcdBacklight(LCD_ON);
}

/***f* loop
 *
 * Main Arduino loop function
 */
void loop() {
  /* Increase all the LCD message alternation indexes */
  increaseMessageAlternationIndexes();

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
     * floating switch that will be constantly pressed once the
     * sous vide is in the water, switch on the backlight if off. */
    lastTimeButtonWasPressed = millis();
    if (!isLcdBacklightOn()) {
      Serial.println("Backlight is OFF. Turning on.");
      setLcdBacklight(LCD_ON);
      /* Although we haven't executed an opState function at this
       * point, set the lastTimeExecutedOpStateFunc to the current
       * time since we do not want to execute anything if the lcd
       * backlight was off. We just want to show the current menu
       * to the user. */
      lastTimeExecutedOpStateFunc = millis();
    }
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

  /* If the float_switch is out of the water, then turn off the pump and SSR
   * no matter what is the curent opState and return from the loop function.
   * If the device is in the water, just toggle the proper RGB LED color.
   */
  if (!deviceIsInWater(buttonsPressed))
  {
    /* If the current opState is OPSTATE_OFF_TURN_ON, then
     * we turn the RGB led off. Otherwise we turn it red.
     * A red RGB led indicates that the Sous Vide is ON, but
     * out of water. An Orange RGB led indicates that the immersion
     * heaters are on for more than 20% of the interval window and
     * a green RGB led indicates that the immersion heaters are either
     * not working, or working up to 20% of the interval window.
     */
    pump_operate(false);
    ssr_operate(0);
    if (opState == OPSTATE_OFF_TURN_ON) {
      setRgbLed(RGB_LED_OFF);
      printLcdLine(LCD_STR_PUT_DEVICE_IN_WATER);
    } else {
      setRgbLed(RGB_LED_RED);
      if (deviceInWaterOrTurnOffAlternationIndex % 2 == 0)
        printLcdLine(LCD_STR_PUT_DEVICE_IN_WATER);
      else
        printLcdLine(LCD_STR_OR_TURN_OFF);

      /* If we are in this state we have to look for user action.
       * If the user presses the OK button, we have to turn the
       * device off.
       */
      if (buttonsPressed & BTN_OK)
        opState = OPSTATE_OFF_TURN_ON;
    }

    return;
  } else {
    if (opState == OPSTATE_OFF_TURN_ON)
      setRgbLed(RGB_LED_OFF);
    else
      setRgbLed(RGB_LED_GREEN);
  }

  /* And based on the current state, call the corresponding functions
   * if at least 100ms has passed since the last time an opState function
   * was executed.
   */
  if (millis() - lastTimeExecutedOpStateFunc > 100) {
    Serial.println(buttonsPressed);
    /* Update the lastTimeExecutedOpStateFunc variable since we are just
     * ready to run an opState function */
    lastTimeExecutedOpStateFunc = millis();

    /* Execute the corresponding opState function based on the current state
     */
    switch (opState) {
      case OPSTATE_OFF_TURN_ON:
        on(buttonsPressed);
        break;
      case OPSTATE_ON_TURN_OFF:
        off(buttonsPressed);
        break;
      case OPSTATE_MENU_PRESET:
      case OPSTATE_MENU_PRESET_CHOOSE:
        preset(buttonsPressed);
        break;
      case OPSTATE_MENU_TEMP:
      case OPSTATE_MENU_TEMP_SETUP:
      default:
        tempMenu(buttonsPressed);
        break;
    }
  }

  return;
  //Serial.print("Buttons: ");
  //Serial.println(readButtons());

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
  if(deviceIsInWater(buttonsPressed)) {
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
