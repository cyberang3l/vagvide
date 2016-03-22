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
  Serial.print("Temperature for the sensor ");
  Serial.print(sensor_name);
  Serial.print(" (Pin ");
  Serial.print(pinConnectedTo);
  Serial.print(") is ");
  Serial.println(temperature);
#endif
}

void readAllTemperatures() {
  switch (TEMP_RESOLUTION_BITS){
    case 9:
      if (timeElapsedSinceLastMeasurement < 94)
        delay(94 - timeElapsedSinceLastMeasurement);
      break;
    case 10:
      if (timeElapsedSinceLastMeasurement < 188)
        delay(188 - timeElapsedSinceLastMeasurement);
      break;
    case 11:
      if (timeElapsedSinceLastMeasurement < 375)
        delay(375 - timeElapsedSinceLastMeasurement);
      break;
    default:
      Serial.println("Unknown resolution...");
    case 12:
      if (timeElapsedSinceLastMeasurement < 750)
        delay(750 - timeElapsedSinceLastMeasurement);
      break;
    }
  //Serial.println("Requesting Temperatures Done...");
  avg_temperature = 0;
  for (int i = 0; i < numSensors; i++) {
    temperature[i] = temp_sensor[i].getTempCByIndex(0);
    avg_temperature += temperature[i];
  }
  avg_temperature /= numSensors;
}

void initTempSensors() {
#if DEBUG
  Serial.begin(115200);
  Serial.println("Dallas Temperature IC Control Library Demo");

  Serial.print("============ Ready with ");
  Serial.print(numSensors);
  Serial.println(" Sensors ================");
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
      Serial.println("Unable to find address for Device 0");
  #endif

    temp_sensor[i].setResolution(deviceAddress, TEMP_RESOLUTION_BITS);
    temp_sensor[i].setWaitForConversion(false);
  #if DEBUG
    Serial.print("Device Resolution on Pin ");
    Serial.print(oneWirePins[i]);
    Serial.print(": ");
    Serial.print(temp_sensor[i].getResolution(deviceAddress), DEC);
    Serial.println();
  #endif
  }
}
