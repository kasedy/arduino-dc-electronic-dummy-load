#include <Arduino.h>

#include "system_state.h"
#include "average.h"

namespace SystemState {
  struct ElectricPowerValueAccumulator {
    uint64_t chargeAccumulator = 0; /* mA*ms */
    uint64_t powerAccumulator = 0; /* mA * mV * ms */
  
    inline void addMeasurement(uint16_t averageAmperage /* mA */,
                               uint32_t averageVoltage /*mV*/,
                               uint32_t deltaTime /* ms */) {
      chargeAccumulator += (uint64_t) averageAmperage * deltaTime;
      powerAccumulator += (uint64_t) averageAmperage * averageVoltage * deltaTime;
    }
  
    inline float getCharge() { /* mA*h */
      return chargeAccumulator / 3600000.0 ;
    }
  
    inline float getEnergy() { /* mW*h */
      return powerAccumulator / 3600000000.0;
    }
  
    uint16_t reset(float averageCharge = 0, float averageEnergy = 0) {
      uint16_t change = 0;
      uint64_t newChargeAccumulator = averageCharge * 3600000.0;
      if (chargeAccumulator != newChargeAccumulator) {
        chargeAccumulator = newChargeAccumulator;
        change |= AverageCharge;
      }
      uint64_t newPowerAccumulator  = averageEnergy * 3600000000.0;
      if (powerAccumulator != newPowerAccumulator) {
        powerAccumulator = newPowerAccumulator;
        change |= AverageEnergy;
      }
      return change;
    }
  };

  static struct State {
    int16_t averageTemperature;
    uint16_t desiredAmperage;
    uint32_t stopVoltage;
    uint16_t changeFlag;
    bool deviceIsInShutDownMode:1;
    bool deviceStatusIsOn:1;
    State() : averageTemperature(0), 
              desiredAmperage(0), 
              stopVoltage(0), 
              changeFlag(-1), 
              deviceIsInShutDownMode(false),
              deviceStatusIsOn(false) {}
  } state;

  AverageValueAccumulator<uint16_t, uint32_t> amperage;
  AverageValueAccumulator<uint32_t, uint32_t> voltage;
  ElectricPowerValueAccumulator averageCapacity;
  
  void setAmperage(uint16_t value) {
    if (amperage.addMeasurement(value)) {
      state.changeFlag |= SystemParameterChanged::InstantAmperage;
    }
  }
  
  void setVoltage(uint32_t value) {
    if (voltage.addMeasurement(value)) {
      state.changeFlag |= SystemParameterChanged::InstantVoltage;
    }
  }

  void setDesiredAmperage(uint16_t value) {
    if (state.desiredAmperage != value) {
      state.desiredAmperage = value;
      state.changeFlag |= SystemParameterChanged::DesiredAmperage;
    }
  }
  
  uint16_t getDesiredAmperage() {
    return state.desiredAmperage;
  }
  
  void setStopVoltage(uint32_t value) {
    if (state.stopVoltage != value) {
      state.stopVoltage = value;
      state.changeFlag |= SystemParameterChanged::StopVoltage;
    }
  }
  
  uint32_t getStopVoltage() {
    return state.stopVoltage;
  }
  
  void setAverageTemperature(int16_t value) {
    if (state.averageTemperature != value) {
      state.averageTemperature = value;
      state.changeFlag |= SystemParameterChanged::AverageTemperature;
    }
  }
  
  void calculateAverage(uint32_t timePassedMillis) {
    if (amperage.calculateAverageValue()) {
      state.changeFlag |= SystemParameterChanged::AverageAmperage;
    }
    if (voltage.calculateAverageValue()) {
      state.changeFlag |= SystemParameterChanged::AverageVoltage;
    }
    uint16_t averageAmperage = getAverageAmperage();
    uint32_t averageVoltage = getAverageVoltage();
    averageCapacity.addMeasurement(getAverageAmperage(), getAverageVoltage(), timePassedMillis);
    if (timePassedMillis != 0 && averageAmperage != 0) {
      state.changeFlag |= SystemParameterChanged::AverageCharge;
      if (getAverageVoltage() != 0) {
        state.changeFlag |= SystemParameterChanged::AverageEnergy;
      }
    }
  }
  
  uint16_t getInstantAmperage() {
    return amperage.getInstantValue();
  }
  
  uint16_t getAverageAmperage() {
    return amperage.getAverageValue();
  }

  uint32_t getInstantVoltage() {
    return voltage.getInstantValue();
  }
  
  uint32_t getAverageVoltage() {
    return voltage.getAverageValue();
  }
  
  int16_t getAverageTemperature() {
    return state.averageTemperature;
  }

  uint32_t getAveragePower() {
    return getAverageAmperage() * getAverageVoltage() / 1000;
  }
  
  float getAverageCharge() {
    return averageCapacity.getCharge();
  }
  
  float getAverageEnergy() {
    return averageCapacity.getEnergy();
  }

  void resetAverageChargeAndEnergy() {
    state.changeFlag |= averageCapacity.reset();
  }

  void restoreAverageChargeAndEnergy(float averageCharge, float averageEnergy) {
    state.changeFlag |= averageCapacity.reset(averageCharge, averageEnergy);
  }

  void clearChangeFlag() {
    state.changeFlag = SystemParameterChanged::Nothing;
  }

  bool isChanged(SystemParameterChanged systemParameter) {
    return state.changeFlag & systemParameter;
  }

  void setDeviceStatusIsOn(bool value) {
    if (state.deviceIsInShutDownMode && value) {
      return;
    }
    if (state.deviceStatusIsOn != value) {
      state.deviceStatusIsOn = value;
      state.changeFlag |= SystemParameterChanged::DeviceStatusIsOn;
    }
  }

  bool getDeviceStatusIsOn() {
    return state.deviceStatusIsOn;
  }

  void setDeviceIsInShutDownMode(bool value) {
    if (state.deviceIsInShutDownMode != value) {
      state.deviceIsInShutDownMode = value;
      state.changeFlag |= SystemParameterChanged::DeviceIsInShutDownMode;
      if (state.deviceIsInShutDownMode) {
        setDeviceStatusIsOn(false);
      }
    }
  }

  void setChangeFlag(SystemParameterChanged changeType) {
    state.changeFlag |= changeType;
  }

  bool isFirstLoop() {
    return state.changeFlag == -1;
  }
};
