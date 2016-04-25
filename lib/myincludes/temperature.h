#ifndef temperature_h
#define temperature_h
#ifdef __cplusplus

#include "common.h"
/* Use OneWire and DallasTemperatures for the temperature sensors */
#include "OneWire.h"
#include "DallasTemperature.h"

#define MIN_TEMPERATURE 10
#define MAX_TEMPERATURE 85

#define TEMP_RESOLUTION_BITS 12 // 9, 10, 11 or 12 bits resolution with
                                // 93.75ms, 187.5ms, 375ms and 750ms temperature
                                // reading time respectively. Read the DS18B20
                                // datasheet for more information.

#define TEMPSENSOR_DESC_STR_LENGTH 19 // The length of the strings in the tempSensorDesc array.

extern byte oneWirePins[];      // Array to store the pins that each sensor is connected to.
extern const byte numSensors;   // Variable to store the number of sensors available.
extern String tempSensorDesc[]; // Array to store the description of each sensor.
extern float *temperature;      // Array to store the measured temperature for each sensor.
extern float avg_temperature;   // A variable to store the average temperature from all sensors.

extern OneWire temp_sensor_oneWire[];   // Array to store the OneWire association per sensor.
extern DallasTemperature temp_sensor[]; // Array to store the DallasTemperature object for each sensor.

extern elapsedMillis timeElapsedSinceLastMeasurement;

extern double desired_temperature; // These are double instead of float, because the PID module
extern double current_temperature; // accepts double values in the PID constructor.

/***f* printTemperature
 *
 * Prints the temperature for a device in the Serial port.
 * Only used for debugging.
 */
void printTemperature(IN float temperature,
                      IN String sensor_name,
                      IN byte pinConnectedTo);

/***f* _requestAllTemperatures
 *
 * The function _requestAllTemperatures will just initiate an
 * asynchronous temperature conversion to all of the temperature sensors.
 *
 * Important! Treat the temperature sensors as opaque objects, and handle
 * them only through the provided functions.
 */
void _requestAllTemperatures();

/***f* readAllTemperatures
 *
 * Read the temperatures from all sensors and store them in the
 * temperature array. When the necessary time for the conversion
 * has elapsed, initiate a new temperature conversion.
 *
 * Moreover, calculate the average from all sensors and store it
 * in the variable avg_temperature and current_temperature.
 */
void readAllTemperatures();

/***f* initTempSensors
 *
 * Initialize all the temperature sensors.
 * Call in the setup() function.
 */
void initTempSensors();

/***f* initDesiredTemperature
 *
 * Initialize the desired_temperature variable
 * Using either a value from EEPROM or a default value.
 */
void initDesiredTemperature();

#endif // endif __cpluscplus
#endif // endif temperature_h
