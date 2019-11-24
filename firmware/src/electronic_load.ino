
#include <Wire.h>
#include <ClickEncoder.h>
#include <SSD1306Ascii.h>
#include <SSD1306AsciiWire.h>
#include <MemoryFree.h>

#include "constants.h"
#include "menu_navigator.h"
#include "system_state.h"
#include "managers.h"

SSD1306AsciiWire oled;

ClickEncoder encoder(ENCODER_PIN_LEFT,
                     ENCODER_PIN_RIGHT,
                     ENCODER_PIN_BTN,
                     4);

uint32_t lastGaugeUpdateTime = 0;
uint32_t loopProcessedLastTime = 0;

MenuNavigator menuNavigator = MenuNavigator(encoder, oled);

ISR(TIMER1_OVF_vect)
{
  encoder.service();
}

void setup() {
  Serial.begin(9600);
  Serial.println(F("Setup Start!"));

  randomSeed(analogRead(UNUSED_ANALOG_PIN));

  PersistenceStateManager::setup();
  AmperagePinManager::setup();
  EmergencyManager::setup();
  FanManager::setup();
  FanTemperatureReader::setup();
  GaugeReader::setup();
  SdCardLogger::setup();

  setupOledDisplay();

  // Timer1 is 16 bit, up to 65536
  // None-inverted mode for pin 10
  TCCR1A = _BV(COM1B1); 
  // Fast PWM mode 14 with ICR1 as TOP, scale x1
  TCCR1A |= _BV(WGM11);
  TCCR1B = _BV(WGM13) | _BV(WGM12) | _BV(CS10); 
  // Frequency = 16Mg / 1 scale / 1 fast mode / 16000 = 1Kz
  ICR1 = MAX_PWM_DUTY_CYCLE;
  // interrupt is attached
  TIMSK1 = _BV(TOIE1); 

  Serial.print(F("Memory left: "));
  Serial.print(freeMemory());
  Serial.println(F(" bytes"));
  Serial.println(F("Setup end! "));

  loopProcessedLastTime = millis();
}

void loop() {
  unsigned long now = millis();

  menuNavigator.processInput();
  GaugeReader::makeMeasurement();
  FanTemperatureReader::makeMeasurement();

  unsigned long timeSinceLastupdate = now - lastGaugeUpdateTime;
  bool isRefreshCycle = timeSinceLastupdate >= REFRESH_INTERVAL_MS;
  if (isRefreshCycle) {  
    lastGaugeUpdateTime = now;
    SystemState::calculateAverage(timeSinceLastupdate);
    FanTemperatureReader::updateAverageTemperatureValue();
  }

  EmergencyManager::updateOnOffState();
  AmperagePinManager::tuneDischargeCurrent(isRefreshCycle);
  menuNavigator.updateOutput();
  FanManager::updateFanSpeed();

  if (isRefreshCycle) {
    SdCardLogger::writeSystemState();
    PersistenceStateManager::preserve();
  }
  
//  SystemState::debugPrint();

//  Serial.println(now - loopProcessedLastTime);
//  loopProcessedLastTime = now;

  SystemState::clearChangeFlag(); // should go the last
}

void setupOledDisplay() {
  Wire.begin();
  oled.begin(&Adafruit128x64, DISPLAY_I2C_ADDRESS);
  oled.setFont(MENU_FONT);
 
  oled.clear();
  oled.setContrast(255);
  oled.setCursor(0, 4);
  oled.print(F("Electronic load v1.1"));
  delay(2000);
  oled.clear();
}
