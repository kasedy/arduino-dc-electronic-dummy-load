#pragma once

namespace SystemState {
  void setup();

  void calculateAverage(uint32_t timePassedMillis);
  void clearChangeFlag();
  bool isFirstLoop();
  
  uint16_t getInstantAmperage();
  uint16_t getAverageAmperage();
  void setAmperage(uint16_t value);

  uint32_t getInstantVoltage();
  uint32_t getAverageVoltage();
  void setVoltage(uint32_t value);
  
  uint16_t getDesiredAmperage();
  void setDesiredAmperage(uint16_t value);

  uint32_t getStopVoltage();
  void setStopVoltage(uint32_t value);

  int16_t getAverageTemperature();
  void setAverageTemperature(int16_t value);
  
  uint32_t getAveragePower();
  float getAverageCharge();
  float getAverageEnergy();
  void resetAverageChargeAndEnergy();
  void restoreAverageChargeAndEnergy(float averageCharge, float averageEnergy);

  bool getDeviceStatusIsOn();
  void setDeviceStatusIsOn(bool value);
  void setDeviceIsInShutDownMode(bool value);

  enum SystemParameterChanged {
    Nothing = 0,
    InstantAmperage = 1 << 0,
    AverageAmperage = 1 << 1,
    InstantVoltage = 1 << 2,
    AverageVoltage = 1 << 3,
    AverageTemperature = 1 << 4,
    AverageCharge = 1 << 5, // mAh
    AverageEnergy = 1 << 6, // mWh
    DesiredAmperage = 1 << 7,
    StopVoltage = 1 << 8,
    DeviceStatusIsOn = 1 << 9,
    DeviceIsInShutDownMode = 1 << 10,
    SdCardLogFile = 1 << 11,
    MainEmergency = 1 << 12,
    AveragePower = AverageVoltage | AverageAmperage, // mW
  };

  void setChangeFlag(SystemParameterChanged changeType);
  bool isChanged(SystemParameterChanged systemParameter);

  inline void debugPrint() {
    if (isChanged(InstantAmperage)) {
      Serial.print(F("InstantAmperage: ")); Serial.println(getInstantAmperage());
    }
    if (isChanged(AverageAmperage)) {
      Serial.print(F("AverageAmperage: ")); Serial.println(getAverageAmperage());
    }
    if (isChanged(AverageVoltage)) {
      Serial.print(F("AverageVoltage: ")); Serial.println(getAverageVoltage());
    }
    if (isChanged(AverageTemperature)) {
      Serial.print(F("AverageTemperature: ")); Serial.println(getAverageTemperature());
    }
    if (isChanged(AverageCharge)) {
      Serial.print(F("AverageCharge: ")); Serial.println(getAverageCharge());
    }
    if (isChanged(AverageEnergy)) {
      Serial.print(F("AverageEnergy: ")); Serial.println(getAverageEnergy());
    }
    if (isChanged(DesiredAmperage)) {
      Serial.print(F("DesiredAmperage: ")); Serial.println(getDesiredAmperage());
    }
    if (isChanged(StopVoltage)) {
      Serial.print(F("StopVoltage: ")); Serial.println(getStopVoltage());
    }
  }
};
