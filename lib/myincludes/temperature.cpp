#include "temperature.h"

byte oneWirePins[] = {
  TEMP_SENSOR_1_PIN, TEMP_SENSOR_2_PIN, TEMP_SENSOR_3_PIN, TEMP_SENSOR_4_PIN
};

String tempSensorDesc[] = {
  "TemperatureSensor1", "TemperatureSensor2", "TemperatureSensor3", "TemperatureSensor4"
};

const byte numSensors = sizeof(oneWirePins) / sizeof(byte);
float *temperature = (float*)malloc(sizeof(float) * numSensors);
float avg_temperature;

OneWire temp_sensor_oneWire[numSensors];
DallasTemperature temp_sensor[numSensors];

elapsedMillis timeElapsedSinceLastMeasurement;

double desired_temperature;
double current_temperature;

void printTemperature(IN float temperature,
                      IN String sensor_name,
                      IN byte pinConnectedTo) {
#if DEBUG
  Serial.print(F("Temperature for the sensor "));
  Serial.print(sensor_name);
  Serial.print(F(" (Pin "));
  Serial.print(pinConnectedTo);
  Serial.print(F(") is "));
  Serial.println(temperature);
#endif
}

void _requestAllTemperatures() {
  for (int i = 0; i < numSensors; i++)
    temp_sensor[i].requestTemperatures();
  timeElapsedSinceLastMeasurement = 0;
}

void readAllTemperatures() {
  /* If the necessary time for conversion hasn't elapsed yet,
   * return from this function without updating the temperatures
   * already stored in the temperature array. */
  switch (TEMP_RESOLUTION_BITS){
    case 9:
      if (timeElapsedSinceLastMeasurement < 94)
        return;
      break;
    case 10:
      if (timeElapsedSinceLastMeasurement < 188)
        return;
      break;
    case 11:
      if (timeElapsedSinceLastMeasurement < 375)
        return;
      break;
    default:
      Serial.println(F("Unknown resolution..."));
    case 12:
      if (timeElapsedSinceLastMeasurement < 750)
        return;
      break;
  }

  /* Update the temperature array and calculate the average */
  avg_temperature = 0;
  for (int i = 0; i < numSensors; i++) {
    temperature[i] = temp_sensor[i].getTempCByIndex(0);
    avg_temperature += temperature[i];
  }
  avg_temperature /= numSensors;
  /* At the moment I get an average temperature, and the current
   * temperature is equal to the average. However, I still want
   * to keep these two variable, because after the testing I am not
   * sure if these two variables should be "one" */
  current_temperature = avg_temperature;

  /* Initiate a new temperature conversion */
  _requestAllTemperatures();
}

void initTempSensors() {
#if DEBUG
  Serial.begin(115200);
  Serial.println(F("Dallas Temperature IC Control Library Demo"));

  Serial.print(F("============ Ready with "));
  Serial.print(numSensors);
  Serial.println(F(" Sensors ================"));
#endif

  //Start up the library on all defined pins
  DeviceAddress deviceAddress;
  for (byte i = 0; i < numSensors; i++) {
    ;
    temp_sensor_oneWire[i].setPin(oneWirePins[i]);
    temp_sensor[i].setOneWire(&temp_sensor_oneWire[i]);
    temp_sensor[i].begin();
  #if DEBUG
    if (!temp_sensor[i].getAddress(deviceAddress, 0))
      Serial.println(F("Unable to find address for Device 0"));
  #endif

    temp_sensor[i].setResolution(deviceAddress, TEMP_RESOLUTION_BITS);
    temp_sensor[i].setWaitForConversion(false);
  #if DEBUG
    Serial.print(F("Device Resolution on Pin "));
    Serial.print(oneWirePins[i]);
    Serial.print(F(": "));
    Serial.print(temp_sensor[i].getResolution(deviceAddress), DEC);
    Serial.println();
  #endif
  }
}

void initDesiredTemperature() {
  /* TODO: Through the web interface, the user should be able to choose
   *       if the last settings should be remember across device reboots.
   *       If yes, then we should be reading the stored desired_temperature
   *       value from eeprom. Otherwise, we should be setting the minimum
   *       temperature as the desired_temperature */
  desired_temperature = MIN_TEMPERATURE;
}
