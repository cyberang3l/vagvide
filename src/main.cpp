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

/* PID variables */
/* Define Variables we'll be connecting to */
double PID_Output;
/* Specify the links and initial tuning parameters */
PID SousPID(&current_temperature, &PID_Output, &desired_temperature, 850, 0.5, 0.1, DIRECT);
//PID SousPID(&current_temperature, &PID_Output, &desired_temperature, 2, 5, 1, DIRECT);

LiquidCrystal_I2C lcd(LCD_I2C_ADDR, LCD_EN_PIN, LCD_RW_PIN,
  LCD_RS_PIN, LCD_D4_PIN, LCD_D5_PIN, LCD_D6_PIN,
  LCD_D7_PIN, LCD_BACKLIGHT_PIN, POSITIVE);

/***f* setup
 *
 * Default Arduino setup function
 */
void setup() {
  // define the degree symbol

  pinMode(PUSH_BTN_MENU_BACK, INPUT);
  pinMode(PUSH_BTN_MENU_OK, INPUT);
  pinMode(PUSH_BTN_MENU_DOWN, INPUT);
  pinMode(PUSH_BTN_MENU_UP, INPUT);
  pinMode(SSR_PIN, OUTPUT);
  pinMode(PUMPRELAY_PIN, OUTPUT);
  pump_operate(false);
  pinMode(FLOAT_SWITCH_PIN, INPUT);
  ssr_operate(0);

  initTempSensors();

  initNetwork();

  //turn the PID on
  SousPID.SetMode(AUTOMATIC);

  lcd.begin(16, 2); //  <<----- My LCD is 16x2, or 20x4
  lcd.createChar(0, degree_symbol); // Store in byte 0 in LCD the degree_symbol (available bytes are 0-7)

  // Switch on the backlight
  lcd.setBacklight(HIGH);
  //lcd.setBacklight(LOW);
  lcd.home (); // go home

  lcd.print("Xinoula Sola PSI!!");
  lcd.setCursor(0, 1);
  lcd.print("Vangelis");
  lcd.setCursor(9, 1);
  lcd.print("C");
  lcd.write(byte(0));
  //lcd.print(char(0)); // Write this symbol
}

void loop() {
  uint8_t float_switch;
  float_switch = digitalRead(FLOAT_SWITCH_PIN);

  // Serial.print("Button OK: ");
  // Serial.println(digitalRead(PUSH_BTN_MENU_OK));
  // Serial.print("Button BACK: ");
  // Serial.println(digitalRead(PUSH_BTN_MENU_BACK));
  // Serial.print("Button UP: ");
  // Serial.println(digitalRead(PUSH_BTN_MENU_UP));
  // Serial.print("Button DOWN: ");
  // Serial.println(digitalRead(PUSH_BTN_MENU_DOWN));
  // delay(500);

  /* If the float_switch is out of the water, then turn off
   * the pump and SSR
   */
  if (!float_switch)
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
  if(float_switch) {
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
  /* Copy received packets to data buffer Ethernet::buffer
   * and return the uint16_t Size of received data (which is needed by ether.packetLoop).
   */
  uint16_t len = ether.packetReceive();
  /* Parse received data and return the uint16_t Offset of TCP payload data
   * in data buffer Ethernet::buffer, or zero if packet processed
   */
  uint16_t pos = ether.packetLoop(len);

  processEthernetPacket(pos);
}
