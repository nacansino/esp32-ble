#include <Arduino.h>

/*
    Based on Neil Kolban example for IDF: https://github.com/nkolban/esp32-snippets/blob/master/cpp_utils/tests/BLE%20Tests/SampleServer.cpp
    Ported to Arduino ESP32 by Evandro Copercini
    updates by chegewara
*/

#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>

// See the following for generating UUIDs:
// https://www.uuidgenerator.net/

#define SERVICE_UUID        "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"
#define CHARACTERISTIC2_UUID "29efd687-7f8c-4a6e-9e50-c61f42d6361d"

#define ADC_CALIB_PIN_HIGH  35
#define ADC_CALIB_PIN_LOW   34
#define ADC_TEMP_PIN        32
#define ADC_CALIB_VOLTS_HIGH  (3.0f)
#define ADC_CALIB_VOLTS_LOW (0.3f)

// Function Prototypes:
float LM61_ADC_reading_to_temp(uint16_t analog_value);
void ADC_get_calibration();

static uint16_t adcHigh, adcLow;
BLECharacteristic *pCharacteristic, *pCharacteristic2;

void setup() {

  // Begin serial debug port
  Serial.begin(115200);
  Serial.println("Starting BLE work!");

  // ADC calibrate
  Serial.println("Calibrating ADC...");
  ADC_get_calibration();

  // it seems that the device doesn't have to be instantiated here
  // Library is calling a static function
  BLEDevice::init("Long name works now");

  // Create bluetooth server (advertiser)
  BLEServer *pServer = BLEDevice::createServer();
  BLEService *pService = pServer->createService(SERVICE_UUID);

  // Create a characteristic with a UUID defined by CHARACTERISTIC_UUID
  // inside the service pService
  pCharacteristic = pService->createCharacteristic(
                                         CHARACTERISTIC_UUID,
                                         BLECharacteristic::PROPERTY_READ |
                                         BLECharacteristic::PROPERTY_WRITE
                                       );

  // Create a characteristic with a UUID defined by CHARACTERISTIC2_UUID
  // inside the service pService
  pCharacteristic2 = pService->createCharacteristic(
                                         CHARACTERISTIC2_UUID,
                                         BLECharacteristic::PROPERTY_READ |
                                         BLECharacteristic::PROPERTY_WRITE
                                       );

  // Sets the value for CHARACTERISTIC
  pCharacteristic->setValue("Hello World says Neil");
  
  // Sets the value and descriptor for CHARACTERISTIC2
  pCharacteristic2->setValue("This is Niel... hehehe");
  pCharacteristic2->setNotifyProperty(true);
 
  // Start the service
  pService->start();
  // BLEAdvertising *pAdvertising = pServer->getAdvertising();  // this still is working for backward compatibility
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(true);
  pAdvertising->setMinPreferred(0x06);  // functions that help with iPhone connections issue
  pAdvertising->setMinPreferred(0x12);
  BLEDevice::startAdvertising();
  Serial.println("Characteristic defined! Now you can read it in your phone!");
}

void loop() {
  // put your main code here, to run repeatedly:
  uint32_t i = 0;
  uint16_t temp_Q1;
  
  // read temperature in Q1 format
  temp_Q1 = LM61_ADC_reading_to_temp(analogRead(ADC_TEMP_PIN)) * 10;
  
  // Sets the value of CHARACTERISTIC2 to the temperature reading
  pCharacteristic2->setValue(temp_Q1);
  i++;

  // delay for 2s
  delay(2000);
}

/**
 * @brief Converts the LM61 ADC readings to temperature
 * @param [in] analog_value The analog value reading
 */
float LM61_ADC_reading_to_temp(uint16_t analog_value){
  const float offset              = 0.6; // volts at 0 degs
  const float temp_to_volts       = 0.01; // 10mV / ℃
  float reading_volts;

  // map according to calibration values (can't use map because we're dealing with floats)
  reading_volts = (float)(analog_value - adcLow) * (ADC_CALIB_VOLTS_HIGH - ADC_CALIB_VOLTS_LOW) / (adcHigh - adcLow) + ADC_CALIB_VOLTS_LOW;
  
  return (reading_volts - offset) / temp_to_volts;

}

/**
 * @brief Reads the absolute high (3V) and low (0.3V) values of the ADC pins
 */
void ADC_get_calibration(){

  adcHigh = analogRead(ADC_CALIB_PIN_HIGH);
  adcLow  = analogRead(ADC_CALIB_PIN_LOW);

}