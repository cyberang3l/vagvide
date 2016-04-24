/* Common things between files such as generic helper
 * functions and definitions
 */
#include "common.h"
/* Custom network functions */
#include "network.h"
/* Custom functions/data-structs related to the temperature sensors */
#include "temperature.h"
/* List of different constant strings that are stored flash memory */
#include "flash_strings.h"

/* The PID and PID Autotune library */
#include <PID_v1.h>
//#include <PID_AutoTune_v0.h>

#define TIME_BETWEEN_MEASUREMENTS 1000 // In milliseconds

/* I want to have an operating state for the LCD, but I want it to
 * be non-blocking because I want to be able to serve the network
 * at all times.
 * So as long as we are not in the "OPSTATE_OFF_TURN_ON", the SousVide is
 * running, but displays different things according to the current state.
 * If the user doesn't press a button for X seconds (X is defined by the
 * variable menuReturnTimeOut), return to the OPSTATE_DISPLAY_TEMP state
 * and show the current and desired temperatures.
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
  OPSTATE_OFF_TURN_ON = 0,        // Displays a Press OK to turn ON. The Sous Vide if off
  OPSTATE_DISPLAY_TEMP,           // This opstate displays the current and the desired
                                  // temperatures. This state is the default state if the
                                  // user hasn't pressed any button for some time.
  OPSTATE_DEFAULT = OPSTATE_DISPLAY_TEMP,
  OPSTATE_MENU_TURN_OFF,          // Display a "Turn Off" label. If OK is pressed go to the
                                  // state OPSTATE_OFF_TURN_ON
  OPSTATE_MENU_TEMP,              // Display "Configure Temperature". If OK is pressed, go
                                  // to the state OPSTATE_MENU_TEMP_SETUP
  OPSTATE_MENU_TEMP_SETUP,        // We can jump in this menu only from OPSTATE_MENU_TEMP.
                                  // If OK or back is pressed, go to OPSTATE_MENU_TEMP
  OPSTATE_MENU_PRESET,            // Displays the menu label: "Select Cooking Preset"
                                  // If OK is pressed go to state OPSTATE_MENU_PRESET_CHOOSE
  OPSTATE_MENU_PRESET_CHOOSE,     // With the UP/DOWN buttons navigate through the presets
                                  // and with the OK button select the current preset.
                                  // New presets can be defined from the web interface.
  OPSTATE_MENU_NET_SETTINGS,      // Display a "Show Network Settings" menu label. If OK is
                                  // pressed move to opstate OPSTATE_MENU_NET_SETTINGS_SHOW
  OPSTATE_MENU_NET_SETTINGS_SHOW, // Show the IP address/netmask of the device and
                                  // if the wireless or ethernet connection is enabled.
  OPSTATE_UNKNOWN                 // Put the device in this state if none of the above
                                  // opstates is received.
};

/* The variable opState keeps the current state */
operatingState opState = OPSTATE_OFF_TURN_ON;
/* The variable prevOpState keeps the previous state and I use it in order
 * to avoid LCD flickering, since I know that if the previous opState is the
 * same as the current opState, I may not need to clear and reprint some LCD
 * values */
operatingState prevOpState = opState;

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
 * the LCD takes some time to clear and get rewritten.
 *
 * We use this variable instead of having delays, because we do not want to block
 * the microntroller since it is service the ethernet and other things.
 * TODO: CHANGE THIS TO SOMETHING LIKE lastTimeSomethingPrintedOnLCD
 */
unsigned long lastTimeExecutedOpStateFunc = millis();

/* Set the interval of the opState function execution to be either
 * 150ms when the LCD backlight is on, or 300ms when off.
 */
uint16_t intervalBetweenOpStateFuncExec = 300;


/* TODO: Allow the user to configure the LCD backlight timeout from the web
 *       interface. A value of 0xFFFFFFFF means that the backlight should be
 *       permanently on. Otherwise, the number should represent the number
 *       in seconds that the backlight will be staying on after a button has
 *       been pressed.
 *
 * The variable lcdBacklightTimeOut is used to configure the number of seconds
 * that the backlight will be staying on after a button has been pressed.
 *
 * The variable menuReturnTimeOut is used to configure the timeout for staying in
 * a menu without any user action (without the user pressing a button). After this
 * timeout, we return to the OPSTATE_MENU_TEMP
 */
uint32_t lcdBacklightTimeOut = 30000;  /* in milliseconds */
uint16_t menuReturnTimeOut = 30000;  /* In milliseconds */

/* Use the variable netInitialized to know when the network has been initialized */
bool netInitialized = false;

/* Sometimes I may need to print larger messages that cannot fit in one go
 * in a 2x16 LCD. In this case I want to be able to alternate through the
 * messages every 3000ms and the variable "lastTimeAlternatedMessage" keeps
 * a log of when the last alternation happened. This variable is getting checked
 * by the function increaseMessageAlternationIndexes.
 */
unsigned long lastTimeAlternatedMessage = millis();

/* If we want to alternate messages, we need an alternation index for each series
 * of messages that we want to alter. Then with a mod operation, we can display
 * different messages in order. Use the function increaseMessageAlternationIndexes
 * to increase the alternation index once every 3000ms.
 */
uint8_t messageAlternationIndex = 0;

/* Variable to store the value of the pressed buttons */
uint8_t buttonsPressed = 0;

/* How many are the main menus? Get this dynamically
 * from the LCD_TOP_LEVEL_MENU_LABELS array */
uint8_t main_menus_count = sizeof(LCD_TOP_LEVEL_MENU_LABELS) / sizeof(char*) / LCD_ROWS;

/* We need a variable 'temporary_temperature' to use when we change the
 * temperature with the buttons. Since we need to press OK to accept the
 * new temperature, we cannot use the desired_temperature variable directly.
 */
float temporary_temperature = 36.6;

/* PID variables */
/* TODO: Make the PID variables "variable" and read them from EEPROM */
double PID_Output;

/* Duration of up and down buttons being pressed *
 * (150 is the button read interval) */
byte longKeyPressCountMin = 13;    // 13 * 150 = 1950 ms
byte mediumKeyPressCountMin = 6;    // 6 * 150 = 900 ms
byte upOrDownPressCount = 0; // How many counts of up or down

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

/***f* increaseMessageAlternationIndex
 *
 * Function to increase all the alternation index and once
 * every 2 seconds (change the 3000ms in the if statement
 * if you want larger alternation interval). This function is
 * called at the beginning of the main loop, as the alternation
 * index is common for all the alternating messages.
 *
 * Use the get getMessageAlternationIndex function to get an
 * index for message printing.
 */
void increaseMessageAlternationIndex() {
  if (millis() - lastTimeAlternatedMessage > 3000) {
    messageAlternationIndex++;
    lastTimeAlternatedMessage = millis();
  }
}

/***f* getMessageAlternationIndex
 *
 * Function to return an index for the current message that needs
 * to be printed, based on the "total_messages".
 */
uint8_t getMessageAlternationIndex(IN uint8_t total_messages) {
  return messageAlternationIndex % total_messages;
}

/***f* turnOn
 *
 * The Sous Vide is OFF and we may want to turn it on.
 * This function only works if the current opstate is
 * in the state OPSTATE_OFF_TURN_ON and we only listen
 * to the OK button press.
 */
void turnOn(IN uint8_t buttonsPressed) {
  if (opState == OPSTATE_OFF_TURN_ON) {
    setRgbLed(RGB_LED_OFF);
    printLcdLine(LCD_STR_PRESS_OK_TO_START);
    /* Once the device is in the water, turn it on only if the
     * user has pressed the OK button. */
    if (buttonsPressed & BTN_OK) {
      /* Do things to turn the device on */
      /* TODO: Read the stored desired temperature
       *       and set the opState to OPSTATE_DEFAULT
       */
      opState = OPSTATE_DEFAULT;
    }
  }
}

/***f* _turnOff
 *
 * This function forces the device to turn off.
 */
void _turnOff() {
  /* Make sure the device is turned off
   * TODO: Revisit this part of the code when
   *       The PID is working.
   */

  pump_operate(false);
  //SousPID.SetMode(MANUAL);
  ssr_operate(0);
  setLcdBacklight(LCD_OFF);
  setRgbLed(RGB_LED_OFF);
}

/***f* turnOff
 *
 * The Sous Vide is ON and we may want to turn it off.
 * This function only works if the current opstate is
 * in the state OPSTATE_MENU_TURN_OFF
 */
void turnOff(IN uint8_t buttonsPressed) {
  if (opState == OPSTATE_MENU_TURN_OFF) {
    printLcdLine(LCD_TOP_LEVEL_MENU_LABELS[0]); // Prints in LCD "Turn off device"
    if (buttonsPressed & BTN_OK) {
      /* By pressing OK, we turn off the device */
      opState = OPSTATE_OFF_TURN_ON;
      _turnOff();
    } else if (buttonsPressed & BTN_DOWN) {
      /* By pressing DOWN, we go to the next top-level menu */
      opState = OPSTATE_MENU_TEMP;
    } else if (buttonsPressed & BTN_UP) {
      /* By pressing UP, we go to the previous top-level menu */
      opState = OPSTATE_MENU_NET_SETTINGS;
    } else if (buttonsPressed & BTN_BACK) {
      /* By pressing BACK, we always go to the default menu when we are
       * in a top-level menu */
      opState = OPSTATE_DEFAULT;
    }
  }
}

/***f* tempStep
 *
 * Returns the appropriate
 * temperature jump step
 */
float tempStep() {
  float temp_increment = 0.1;

  if(upOrDownPressCount >= longKeyPressCountMin)
  {
    temp_increment = 2;
  }

  else if(upOrDownPressCount >= mediumKeyPressCountMin)
  {
    temp_increment = 1;
  }

  upOrDownPressCount++;

  return temp_increment;
}

/***f* tempMenu
 *
 * We run this function only in the opstates
 * OPSTATE_MENU_TEMP and OPSTATE_MENU_TEMP_SETUP
 */
void tempMenu(IN uint8_t buttonsPressed) {

  if (opState == OPSTATE_MENU_TEMP) {
    printLcdLine(LCD_TOP_LEVEL_MENU_LABELS[1]);
    if (buttonsPressed & BTN_OK) {
      /* By pressing OK, we turn off the device */
      opState = OPSTATE_MENU_TEMP_SETUP;
    } else if (buttonsPressed & BTN_DOWN) {
      /* By pressing DOWN, we go to the next top-level menu */
      opState = OPSTATE_MENU_PRESET;
    } else if (buttonsPressed & BTN_UP) {
      /* By pressing UP, we go to the previous top-level menu */
      opState = OPSTATE_MENU_TURN_OFF;
    } else if (buttonsPressed & BTN_BACK) {
      /* By pressing BACK, we always go to the default menu when we are
       * in a top-level menu */
      opState = OPSTATE_DEFAULT;
    }
  } else if (opState == OPSTATE_MENU_TEMP_SETUP) {
    if (prevOpState != opState) {
      printLcdLine(LCD_STR_SET_TARGET_TEMP);
      lcd.setCursor(1, 1);
      lcd.write(byte(0));
    } else {
      printLcdLine(LCD_STR_SET_TARGET_TEMP, 1);
    }
    lcd.setCursor(2, 1);
    lcd.print(temporary_temperature);

    if (buttonsPressed & BTN_OK) {
      /* TODO
       * By pressing OK, Save the desired temperature for the runtime,
       * and in EEPROM if the user has selected the restore settings
       * after shut-down option through the web.
       *
       * Then Go to the previous menu which is OPSTATE_MENU_TEMP
       */
      desired_temperature = temporary_temperature;
      opState = OPSTATE_MENU_TEMP;
    } else if (buttonsPressed & BTN_DOWN) {
      /* Decrease the temperature but respect the limits */
      if (temporary_temperature - tempStep() < MIN_TEMPERATURE)
        temporary_temperature = MIN_TEMPERATURE;
      else
        temporary_temperature -= tempStep();
    } else if (buttonsPressed & BTN_UP) {
      /* Increase the temperature but respect the limits */
      if (temporary_temperature + tempStep() > MAX_TEMPERATURE)
        temporary_temperature = MAX_TEMPERATURE;
      else
        temporary_temperature += tempStep();
    } else if (buttonsPressed & BTN_BACK) {
      /* By pressing BACK, go to the upper menu without
       * saving the changes to the temperature */
      temporary_temperature = desired_temperature;
      opState = OPSTATE_MENU_TEMP;
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
    printLcdLine(LCD_TOP_LEVEL_MENU_LABELS[2]);
    if (buttonsPressed & BTN_OK) {
      /* By pressing OK, we turn off the device */
      opState = OPSTATE_MENU_PRESET_CHOOSE;
    } else if (buttonsPressed & BTN_DOWN) {
      /* By pressing DOWN, we go to the next top-level menu */
      opState = OPSTATE_MENU_NET_SETTINGS;
    } else if (buttonsPressed & BTN_UP) {
      /* By pressing UP, we go to the previous top-level menu */
      opState = OPSTATE_MENU_TEMP;
    } else if (buttonsPressed & BTN_BACK) {
      /* By pressing BACK, we always go to the default menu when we are
       * in a top-level menu */
      opState = OPSTATE_DEFAULT;
    }
  } else if (opState == OPSTATE_MENU_PRESET_CHOOSE) {
    /* TODO: Handle all the buttons for the submenu for
     *       choosing presets */
  }
}

/***f* netSettings
 *
 * We run this function only in the opstates
 * OPSTATE_MENU_NET_SETTINGS and OPSTATE_MENU_NET_SETTINGS_SHOW
 */
void netSettings(IN uint8_t buttonsPressed) {
  if (opState == OPSTATE_MENU_NET_SETTINGS) {
    printLcdLine(LCD_TOP_LEVEL_MENU_LABELS[3]);
    if (buttonsPressed & BTN_OK) {
      /* By pressing OK, we turn off the device */
      opState = OPSTATE_MENU_NET_SETTINGS_SHOW;
    } else if (buttonsPressed & BTN_DOWN) {
      /* By pressing DOWN, we go to the next top-level menu */
      opState = OPSTATE_MENU_TURN_OFF;
    } else if (buttonsPressed & BTN_UP) {
      /* By pressing UP, we go to the previous top-level menu */
      opState = OPSTATE_MENU_PRESET;
    } else if (buttonsPressed & BTN_BACK) {
      /* By pressing BACK, we always go to the default menu when we are
       * in a top-level menu */
      opState = OPSTATE_DEFAULT;
    }
  } else if (opState == OPSTATE_MENU_NET_SETTINGS_SHOW) {
    /* TODO */
  }
}

/***f* display_temperature
 *
 * Displays the temperature in the LCD.
 * We run this function only in the opstate OPSTATE_DISPLAY_TEMP
 */
void display_temperature() {
  if (opState == OPSTATE_DISPLAY_TEMP) {
    uint8_t total_strings_str = sizeof(LCD_DISPLAY_TEMPERATURE) / sizeof(char*) / LCD_ROWS;
    uint8_t current_message_index = getMessageAlternationIndex(total_strings_str);
    /* The current temperature must be first in the next array.
     * The goal/target temperature must be second */
    // TODO: Add the correct temperatures from the variables
    float current_goal_temps[2] = {27.684, 57.450};

    if (prevOpState != opState) {
      printLcdLine(LCD_DISPLAY_TEMPERATURE[current_message_index]);
      lcd.setCursor(1, 1);
      lcd.write(byte(0));
    } else {
      /* If the previous opState was the same, then don't rewrite the
       * second line of the LCD to avoid some flickering. It will be
       * overwritten anyway once we update the temperature */
      printLcdLine(LCD_DISPLAY_TEMPERATURE[current_message_index], 1);
    }
    lcd.setCursor(2, 1);
    lcd.print(current_goal_temps[current_message_index]);
  }

  if ((buttonsPressed & BTN_OK) |
      (buttonsPressed & BTN_DOWN) |
      (buttonsPressed & BTN_UP)) {
    /* By pressing OK, UP or DOWN from the default state,
     * we go to the OPSTATE_MENU_TEMP top level menu */
    opState = OPSTATE_MENU_TEMP;
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

  /* Initialize the LCD */
  lcd.begin(LCD_COLS, LCD_ROWS); //  <<----- My LCD is 16x2, or 20x4
  lcd.createChar(0, degree_symbol); // Store in byte 0 in LCD the degree_symbol (available bytes are 0-7)

  /* Turn on the backlight during initialization */
  setLcdBacklight(LCD_ON);

  //turn the PID on
  SousPID.SetMode(AUTOMATIC);
}

/***f* loop
 *
 * Main Arduino loop function
 */
void loop() {
  if (opState == OPSTATE_UNKNOWN) {
    /* If we get an unknown opState, just power off the device
     * for safety reasons and don't let the loop to run (return). */
    _turnOff();
    return;
  }
  /* Increases all the LCD message alternation index */
  increaseMessageAlternationIndex();

  /* If the network link is up and the netInitialized == false, we should initialize
   * the network addresses and set the netInitialized to true. If the link is down
   * and the netInitialized == true, we need to set the netInitialized to false again.
   *
   * If the network is initialized and the link is up then process ethernet packets.
   */
  if (netInitialized == false && eth_link_state_up() == true) {
    initNetworkAddr();
    netInitialized = true;
    Serial.println(F("Ethernet link is up."));
  } else if (netInitialized == true) {
    if (eth_link_state_up() == false) {
      netInitialized = false;
      Serial.println(F("Ethernet link is down."));
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

  /* Now read the buttons and execute the opState functions once every
   * 150ms TODO: We need to be reading the buttons much faster, but
   *             we should take care of how we handle double-presses due
   *             to long press of buttons.
   */
  if (millis() - lastTimeButtonWasPressed > 150) {
    buttonsPressed = readButtons();

    /* Clear the upOrDownPressCount if neither up or down are pressed */
    if(!(buttonsPressed & BTN_DOWN) && !(buttonsPressed & BTN_UP))
    {
      upOrDownPressCount = 0;
    }

    /* Check the lcdBacklightTimeOut */
    if (buttonsPressed != 0 && buttonsPressed != BTN_FLOAT_SW) {
      /* If a button is pressed, and this button is not only the
       * floating switch that will be constantly pressed once the
       * sous vide is in the water, switch on the backlight if off. */
      lastTimeButtonWasPressed = millis();
      if (!isLcdBacklightOn()) {
        Serial.println(F("Backlight is OFF. Turning on."));
        setLcdBacklight(LCD_ON);
        /* At this point return, since we don't want to execute anything
         * if the LCD was off and we just turned it on.
         */
        return;
      }
    } else {
      /* Switch off the backlight if a button hasn't been pressed for
       * lcdBacklightTimeOut milliseconds. */
      if (millis() - lastTimeButtonWasPressed > lcdBacklightTimeOut)
        setLcdBacklight(LCD_OFF);
    }

    /* Check the menuReturnTimeOut only if the Sous Vide is running,
     * i.e. is not in the opState OPSTATE_OFF_TURN_ON */
    if ((millis() - lastTimeButtonWasPressed > menuReturnTimeOut) &&
        (opState != OPSTATE_OFF_TURN_ON)) {
      /* If the menu return timeout has expired, go to the default opstate */
      opState = OPSTATE_DEFAULT;

      /* Make sure that the temporary_temperature equals to the
       * desired_temperature if we got an expiration in a menu */
      temporary_temperature = desired_temperature;
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
       * TODO: Currently the above description is not working. We
       *       Only turn the RGB led Green, Red or Off. We don't
       *       use the orange state
       */
      pump_operate(false);
      ssr_operate(0);
      if (opState == OPSTATE_OFF_TURN_ON) {
        setRgbLed(RGB_LED_OFF);
        /* If the device is off, and out of water, just print a message
         * in the LCD to "put the device in water"
         */
        printLcdLine(LCD_STR_PUT_DEVICE_IN_WATER[0]);
      } else {
        /* If the device is on (in any other opstate than OPSTATE_OFF_TURN_ON),
         * and out of water, print the complete message of the
         * LCD_STR_PUT_DEVICE_IN_WATER string which something like:
         * "put the device in water, or press ok to turn off"
         */
        setRgbLed(RGB_LED_RED);
        uint8_t total_strings_str = sizeof(LCD_STR_PUT_DEVICE_IN_WATER) / sizeof(char*) / LCD_ROWS;
        uint8_t current_message_index = getMessageAlternationIndex(total_strings_str);
        printLcdLine(LCD_STR_PUT_DEVICE_IN_WATER[current_message_index]);

        /* If we are in this state we have to look for user action.
         * If the user presses the OK button, we have to turn the
         * device off.
         */
        if (buttonsPressed & BTN_OK)
          opState = OPSTATE_OFF_TURN_ON;
      }

      /* If device is our of water, set the prevOpState to unknown.
       * The prevOpState is not really unknown, but we use this variable
       * in order to determine if we will refresh all lines in the LCD,
       * So after the device is out of water and back in, we would want
       * to make the prevOpState different than the current opState in order
       * to force a refresh in the LCD.
       */
      prevOpState = OPSTATE_UNKNOWN;

      return;
    }

    /* If we execute code at this point, the device is in the water
     * And we handle the rest from the different opState functions
     */

    if (opState != OPSTATE_OFF_TURN_ON)
      setRgbLed(RGB_LED_GREEN);

    /* Execute the corresponding opState function based on the current state.
     *
     * Each of the functions that are called in the following switch statement
     * is changing the opState value. So in order to keep track of the previous
     * opState in the variable prevOpState, we cannot do something like
     * prevOpState = opState after the function has been executed, because the
     * value of the opState has already changed to the value of the "next"
     * opState. For this reason, assign the value of the prevOpState right
     * after each function has been executed in each of the "case" statements.
     */
    switch (opState) {
      case OPSTATE_OFF_TURN_ON:
        turnOn(buttonsPressed);
        prevOpState = OPSTATE_OFF_TURN_ON;
        break;
      case OPSTATE_MENU_TURN_OFF:
        turnOff(buttonsPressed);
        prevOpState = OPSTATE_MENU_TURN_OFF;
        break;
      case OPSTATE_MENU_PRESET:
        preset(buttonsPressed);
        prevOpState = OPSTATE_MENU_PRESET;
        break;
      case OPSTATE_MENU_PRESET_CHOOSE:
        preset(buttonsPressed);
        prevOpState = OPSTATE_MENU_PRESET_CHOOSE;
        break;
      case OPSTATE_MENU_TEMP:
        tempMenu(buttonsPressed);
        prevOpState = OPSTATE_MENU_TEMP;
        break;
      case OPSTATE_MENU_TEMP_SETUP:
        tempMenu(buttonsPressed);
        prevOpState = OPSTATE_MENU_TEMP_SETUP;
        break;
      case OPSTATE_MENU_NET_SETTINGS:
        netSettings(buttonsPressed);
        prevOpState = OPSTATE_MENU_NET_SETTINGS;
        break;
      case OPSTATE_MENU_NET_SETTINGS_SHOW:
        netSettings(buttonsPressed);
        prevOpState = OPSTATE_MENU_NET_SETTINGS_SHOW;
        break;
      case OPSTATE_DISPLAY_TEMP:
        display_temperature();
        prevOpState = OPSTATE_DISPLAY_TEMP;
        break;
      default:
        opState = OPSTATE_UNKNOWN;
        break;
    }
  }

  return;

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
