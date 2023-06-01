/*
  Lead Acid Battery Charger
  Created by Robert Ribeiro, May 28, 2023.
  All rights reserved.
*/

#include <Adafruit_INA219.h>
#include <Wire.h>

#define DISCHARGE_BATTERY 0

#define VOLTAGE_PIN   A0
#define RELAY_PIN     8

#define DISCHARGED_BATT_LEVEL   11.5
#define FLOATING_BATT_LEVEL     13.8
#define MINIMUM_BATT_LEVEL      6.0

#define MINIMUM_CHG_CURRENT     0.20 //mA

#define VOLTAGE_FACTOR          0.01955036F //(5V/1023)*(R3/R3+R4)

#define CHECK_STATUS_PERIOD     100
#define HYSTERESIS_PERIOD       90000UL

#define BATT_CHARGED        0x00
#define BATT_CHARGING       0x01
#define BATT_DISCONNECTED   0x02
uint8_t battStatus = BATT_DISCONNECTED;

bool flagCtrlCharge = false;

float maxCurrent_mA = 0;
float shuntvoltage = 0;
float busvoltage = 0;
float loadVoltage = 0;
float current_mA = 0;
float power_mW = 0;

float energy = 0;
float capacity = 0;

unsigned long previousCheckBatteryStatus;
unsigned long startHysteresisTime;

Adafruit_INA219 INA219;

void setup() {
  Serial.begin(38400);

  pinMode(VOLTAGE_PIN, INPUT);
  pinMode(RELAY_PIN, OUTPUT);

  INA219.begin();
  
  INA219.setCalibration_32V_2A();    // set measurement range to 32V, 2A  (do not exceed 26V!)
  //INA219.setCalibration_32V_1A();    // set measurement range to 32V, 1A  (do not exceed 26V!)
  //INA219.setCalibration_16V_400mA(); // set measurement range to 16V, 400mA
}

void loop() {

#if DISCHARGE_BATTERY //if you want discharge battery, set DISCHARGE_BATTERY to 1          
  for (uint8_t i = 0; i < 5; i ++) {    //     ___/\/\/\/\____
    digitalWrite(RELAY_PIN, LOW);       //    |       R       |
    delay(HYSTERESIS_PERIOD);           //  +++++ batt        |
    digitalWrite(RELAY_PIN, HIGH);      //   ---              |
    delay(1000);                        //    |_______________|
  }                                     //   discharge schematic

  delay(8000);
#else

  if (millis() - previousCheckBatteryStatus >= CHECK_STATUS_PERIOD) {
    previousCheckBatteryStatus = millis();

    float voltage = analogRead(VOLTAGE_PIN) * VOLTAGE_FACTOR; //voltage measurement with voltage divide circuit
    read_INA219(); //current and voltage measurement using INA219 sensor
 
    switch (battStatus) {
      case BATT_CHARGED: { //voltage is less than 11.5V?
        if ((loadVoltage < DISCHARGED_BATT_LEVEL) && (!flagCtrlCharge)) {
          flagCtrlCharge = true;
          startHysteresisTime = millis();
        } else if ((loadVoltage > DISCHARGED_BATT_LEVEL) && (flagCtrlCharge) && 
                   (millis() - startHysteresisTime >= HYSTERESIS_PERIOD)) {
          flagCtrlCharge = false;
        } else if ((loadVoltage < DISCHARGED_BATT_LEVEL) && (flagCtrlCharge) && 
                   (millis() - startHysteresisTime >= HYSTERESIS_PERIOD)) {
          battStatus = BATT_CHARGING;
        }

        if (loadVoltage < MINIMUM_BATT_LEVEL) { //battery disconnected
          flagCtrlCharge = false;
          battStatus = BATT_DISCONNECTED;
        }

        digitalWrite(RELAY_PIN, HIGH); //relay off
      }
      break;

      case BATT_CHARGING: {
        if ((loadVoltage > FLOATING_BATT_LEVEL) && (flagCtrlCharge)) { //battery voltage above 13.8V?
          flagCtrlCharge = false;
          startHysteresisTime = millis();
        } else if ((loadVoltage < FLOATING_BATT_LEVEL) && (!flagCtrlCharge) && 
                   (millis() - startHysteresisTime >= HYSTERESIS_PERIOD)) {
          flagCtrlCharge = true;
        } else if ((loadVoltage > FLOATING_BATT_LEVEL) && (!flagCtrlCharge) && 
                   (millis() - startHysteresisTime >= HYSTERESIS_PERIOD)) {
          battStatus = BATT_CHARGED;
        }

        if ((current_mA < MINIMUM_CHG_CURRENT) && (flagCtrlCharge)) { //current under 20mA?
          flagCtrlCharge = false;
          startHysteresisTime = millis();
        } else if ((current_mA > MINIMUM_CHG_CURRENT) && (!flagCtrlCharge) && 
                   (millis() - startHysteresisTime >= HYSTERESIS_PERIOD)) {
          flagCtrlCharge = true;
        } else if ((current_mA < MINIMUM_CHG_CURRENT) && (!flagCtrlCharge) && 
                   (millis() - startHysteresisTime >= HYSTERESIS_PERIOD)) {
          battStatus = BATT_CHARGED;
        }

        if (loadVoltage < MINIMUM_BATT_LEVEL) { //battery disconnected
          flagCtrlCharge = false;
          battStatus = BATT_DISCONNECTED;
        }

        digitalWrite(RELAY_PIN, LOW); //relay on
      }
      break;

      case BATT_DISCONNECTED: {
        if ((loadVoltage >= MINIMUM_BATT_LEVEL) && (!flagCtrlCharge)) { //voltage above 6V?
          flagCtrlCharge = true;
          startHysteresisTime = millis();
        } else if ((loadVoltage < MINIMUM_BATT_LEVEL) && (flagCtrlCharge) && 
                   (millis() - startHysteresisTime >= 10000)) {
          flagCtrlCharge = false;
        } else if ((loadVoltage >= MINIMUM_BATT_LEVEL) && (loadVoltage < DISCHARGED_BATT_LEVEL) && (flagCtrlCharge) && 
                   (millis() - startHysteresisTime >= 10000)) {
          battStatus = BATT_CHARGING;
        } else if ((loadVoltage >= MINIMUM_BATT_LEVEL) && (loadVoltage > DISCHARGED_BATT_LEVEL) && (flagCtrlCharge) && 
                   (millis() - startHysteresisTime >= 10000)) {
          battStatus = BATT_CHARGED;
        }

        digitalWrite(RELAY_PIN, HIGH); //relay off
      }
      break;
    }

    //debug parameters
    Serial.print(F("STS: "));
    Serial.print(battStatus);
    Serial.print(F("  V[V]: "));
    Serial.print(voltage);
    Serial.print(F("  VL[V]: "));
    Serial.print(loadVoltage);
    Serial.print(F("  I[A]: "));
    Serial.print(current_mA);
    Serial.print(F("  IMAX[A]: "));
    Serial.println(maxCurrent_mA);
  }
#endif

}

/*****************************************
  read voltage and current 
*****************************************/
void read_INA219(void) {
  float sumI = 0;

  shuntvoltage = INA219.getShuntVoltage_mV();
  busvoltage = INA219.getBusVoltage_V();
  current_mA = INA219.getCurrent_mA();

  // for (uint16_t i = 0; i < 1024; i ++) {
  //   float mA = INA219.getCurrent_mA();

  //   sumI += (mA * mA);
  // }

  // current_mA = sqrt(sumI / 1024.0);

  if (current_mA < 0) current_mA = 0;

  if (current_mA > maxCurrent_mA) {
    maxCurrent_mA = current_mA;
  }

  loadVoltage = busvoltage + (shuntvoltage * 0.001);
  power_mW = (loadVoltage * current_mA);
  capacity += (current_mA * 0.000277) * 0.1;
  energy += (power_mW * 0.000277) * 0.1;
}