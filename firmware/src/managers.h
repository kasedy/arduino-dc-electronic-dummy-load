#pragma once

#include "constants.h"

namespace AmperagePinManager {
  void setup();
  void tuneDischargeCurrent(bool isRefreshCycle);
};


namespace EmergencyManager { 
  enum EmergencyType {
    Calmness = 0,
    OverHeat = 1 << 0,
    MosfetFault_1 = 1 << 1,
    MosfetFault_2 = 1 << 2,
    MosfetFault_3 = 1 << 3,
    MosfetFault_4 = 1 << 4,
    MosfetFault_5 = 1 << 5,
    MosfetFault_6 = 1 << 6,
    MosfetFaults = MosfetFault_1 | MosfetFault_2 | MosfetFault_3 |
                   MosfetFault_4 | MosfetFault_5 | MosfetFault_6,
    OverVoltage = 1 << 7,
    StopVoltageReached = 1 << 8,
  };

  void setup();
  void updateOnOffState();
  void reportAmperage(int mAmp, int channel);

  uint16_t getMainEmergency();
  const __FlashStringHelper *emergencyToString(uint16_t emergency);
};


namespace FanManager {
  void setup();
  void updateFanSpeed();
};

namespace FanTemperatureReader {
  void setup();
  void makeMeasurement();
  void updateAverageTemperatureValue();
};

namespace SdCardLogger {
  void setup();
  void writeSystemState();

  bool isFault();
  void printFileName(const Print &printer);
  void changeFile();
};

namespace PersistenceStateManager {
  void setup();
  void preserve();
};


namespace GaugeReader {
  void setup();
  void makeMeasurement(); 
};
