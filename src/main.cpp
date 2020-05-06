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

// DEFINE THE UUIDs for the custom characteristics
#define CHAR_ADC_UUID  "229a8d41-fde4-44ff-8dad-feecdc379e92"
#define CHAR_SW_UUID   "d8520577-81ed-478c-a3ad-a810d65c064a"
#define CHAR_LED_UUID  "638cc58f-0c58-4f7a-ab38-0df7aff5e1f3"
#define CHAR_STR_UUID  "be6b91f2-86ef-4426-807e-2a2e6e67e29d"

#define ADC_CALIB_PIN_HIGH  35
#define ADC_CALIB_PIN_LOW   34
#define ADC_TEMP_PIN        32
#define ADC_CALIB_VOLTS_HIGH  (3.0f)
#define ADC_CALIB_VOLTS_LOW (0.3f)

// Define the pins for our GPIOs
#define SW_PIN  23
#define LED_PIN 12
#define STS_PIN 13

// Function Prototypes:
float LM61_ADC_reading_to_temp(uint16_t analog_value);
void ADC_get_calibration();
void onFallingSwPinISR(void);  // ISR
void onTimeISR(void); // ISR

// Global Variables
enum e_ESP32_BLE_STS{
  E_STS_SETUP = 0,
  E_STS_STANDBY,
  E_STS_CONNECTED,
  E_STS_NUMBER_OF_STATUS
};
static e_ESP32_BLE_STS globalStatus;

static uint16_t adcHigh, adcLow;
BLECharacteristic *pCharADC, *pCharSW, *pCharLED, *pCharSTR;

// Timer for status LED
hw_timer_t * timer = NULL;
portMUX_TYPE timerMux = portMUX_INITIALIZER_UNLOCKED;

// Function callbacks
class myAdcCallback : public BLECharacteristicCallbacks{
  void onRead(BLECharacteristic* pCharacteristic){
    // Only read when there is a request    
    // read temperature in Q1 format
    uint16_t temp_Q1 = LM61_ADC_reading_to_temp(analogRead(ADC_TEMP_PIN)) * 10;
      
    // Sets the value of CHARACTERISTIC2 to the temperature reading
    pCharacteristic->setValue(temp_Q1);
    
  }  
  void onWrite(BLECharacteristic* pCharacteristic){
    // Nothing here, because this is read-only
  }
};

class myLEDCallback : public BLECharacteristicCallbacks{
  void onRead(BLECharacteristic* pCharacteristic){
    // No need to do here
  }  
  void onWrite(BLECharacteristic* pCharacteristic){
    // write the state to GPIO if there is write request
    
    // fetch value
    std::string data = pCharacteristic->getValue();

    // set the LED based on 1st character of fetched data   
    if ( data == "SET"){
      digitalWrite(LED_PIN, HIGH);
    } else if ( data == "RESET") {
      digitalWrite(LED_PIN, LOW);
    } 
    // do nothing if other values are received

  }
};

class mySTRCallback : public BLECharacteristicCallbacks{
  void onRead(BLECharacteristic* pCharacteristic){
    // No need to do here
  }  
  void onWrite(BLECharacteristic* pCharacteristic){
    // Send the string to serial port
    
    // fetch value
    std::string data = pCharacteristic->getValue();

    // print value
    Serial.println(data.c_str());

  }
};

class peripheralCallback : public BLEServerCallbacks{
  
  void onConnect(BLEServer* pServer){
    // On connect, display msg
    globalStatus = E_STS_CONNECTED;

    timerAlarmDisable(timer);
    digitalWrite(STS_PIN, HIGH);

  }

  void onDisconnect(BLEServer* pServer){
    // On Disconnect, display msg
    globalStatus = E_STS_STANDBY;
    timerAlarmEnable(timer);  
  }

};

void setup() {
  // Set global status
  globalStatus = E_STS_SETUP;

  // Begin serial debug port
  Serial.begin(115200);
  Serial.println("Starting BLE work!");

  // ADC calibrate
  Serial.println("Calibrating ADC...");
  ADC_get_calibration();

  // Setup status LED timer
  // Refer here: https://github.com/espressif/arduino-esp32/blob/master/libraries/ESP32/examples/Timer/RepeatTimer/RepeatTimer.ino
  timer = timerBegin(0, 80, true); // 1 million ticks/sec @ 80MHz
  // Fire Interrupt every 500k ticks, so 250ms
  timerAttachInterrupt(timer, &onTimeISR, true);
  timerAlarmWrite(timer, 250000, true);
  timerAlarmEnable(timer);
  pinMode(STS_PIN, OUTPUT);

  // set pin modes for SW and LED
  pinMode(SW_PIN, INPUT);
  pinMode(LED_PIN, OUTPUT);

  // attach interrupt for SW_PIN
  attachInterrupt(SW_PIN, onFallingSwPinISR, FALLING);

  // it seems that the device doesn't have to be instantiated here
  // Library is calling a static function
  BLEDevice::init("MyESP32BLE");

  // Create bluetooth server (advertiser)
  BLEServer *pServer = BLEDevice::createServer();
  BLEService *pService = pServer->createService(SERVICE_UUID);

  // Creates the three characteristics: ADC, SW, LED, STR
  // inside the service pService
  pCharADC = pService->createCharacteristic(
                                         CHAR_ADC_UUID,
                                         BLECharacteristic::PROPERTY_READ
                                       ); // This will be read-only
  pCharSW = pService->createCharacteristic(
                                         CHAR_SW_UUID,
                                         BLECharacteristic::PROPERTY_READ
                                       ); // This will be read-only
  pCharLED = pService->createCharacteristic(
                                         CHAR_LED_UUID,
                                         BLECharacteristic::PROPERTY_READ |
                                         BLECharacteristic::PROPERTY_WRITE
                                       ); // This is read/write
  pCharSTR = pService->createCharacteristic(
                                         CHAR_STR_UUID,
                                         BLECharacteristic::PROPERTY_READ |
                                         BLECharacteristic::PROPERTY_WRITE
                                       ); // This is read/write

  // bind callbacks
  pServer->setCallbacks(new peripheralCallback());

  pCharADC->setCallbacks(new myAdcCallback());
  pCharLED->setCallbacks(new myLEDCallback());
  pCharSTR->setCallbacks(new mySTRCallback());

  // Sets the initial value for STR
  pCharSTR->setValue("Hello World...");

  // enable notification on SW
  pCharSW->setNotifyProperty(true);
   
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

  globalStatus = E_STS_STANDBY;
}

void loop() {
  // do nothing here
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

/**
 * @brief On falling edge of SwPinISR
 */
void IRAM_ATTR onFallingSwPinISR(void){
  // Send a notification to GATT client with a value "SW_HIGH"
  pCharSW->setValue("SW_HIGH");
  pCharSW->notify();

}

void IRAM_ATTR onTimeISR() {
	portENTER_CRITICAL_ISR(&timerMux);
  // Increment of shared variables per core must be done here!
	portEXIT_CRITICAL_ISR(&timerMux);

  // toggle status LED here
  digitalWrite(STS_PIN, !digitalRead(STS_PIN));
}
