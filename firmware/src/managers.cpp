#include <Arduino.h>
#include <EEPROM.h>
#include <SPI.h>
#include <SdFat.h>
#include <Adafruit_INA219.h>

#include "managers.h"
#include "system_state.h"
#include "average.h"
#include "helpers.h"


namespace AmperagePinManager {
  static int16_t amperagePwmDutyCycle = 0;

  void setup() {
    pinMode(CONTROL_CURRENT_PIN, OUTPUT);
    digitalWrite(CONTROL_CURRENT_PIN, LOW);
  }

  static int16_t getStep(int16_t amperageDifference) {
    return amperageDifference / 5 + sgn(amperageDifference);
  }

  static void setPwmDutyCycle(int16_t newAmperagePwmDutyCycle) {
    if (amperagePwmDutyCycle == newAmperagePwmDutyCycle) {
      return;
    }
    newAmperagePwmDutyCycle = constrain(newAmperagePwmDutyCycle, 0, MAX_PWM_DUTY_CYCLE);
    if (amperagePwmDutyCycle != newAmperagePwmDutyCycle) {
      amperagePwmDutyCycle = newAmperagePwmDutyCycle;
      OCR1B = amperagePwmDutyCycle;
    }
  }
  
  void tuneDischargeCurrent(bool isRefreshCycle) {
#if ENABLE_AMPERAGE_CALIBRATION
    setPwmDutyCycle(SystemState::getDesiredAmperage());
    return;
# endif
    if (!SystemState::getDeviceStatusIsOn() || SystemState::getAverageVoltage() == 0 || SystemState::getDesiredAmperage() < MIN_CURRENT_MA) {
      setPwmDutyCycle(0);
    } else if (isRefreshCycle) {
      int16_t pwmTopLimit = SystemState::getDesiredAmperage() + max(SystemState::getDesiredAmperage() / 10, 50);
      int16_t step = getStep(SystemState::getDesiredAmperage() - SystemState::getAverageAmperage());
      setPwmDutyCycle(min(amperagePwmDutyCycle + step, pwmTopLimit));
    }
  }
};


namespace EmergencyManager {
  static uint16_t emergency = EmergencyType::Calmness;

  void setEmergency(uint16_t newValue) {
    if (newValue == emergency) {
      return;
    }

    uint16_t oldMainEmergency = getMainEmergency();
    emergency = newValue;
    if (oldMainEmergency != getMainEmergency()) {
      SystemState::setChangeFlag(SystemState::MainEmergency);
    }
  }

  void setup() {
    pinMode(AMPERAGE_ON_OFF_PIN, OUTPUT);
    digitalWrite(AMPERAGE_ON_OFF_PIN, LOW);
  }

  void checkOverHeat() {
    if (!SystemState::isChanged(SystemState::AverageTemperature)) {
      return;
    }
    int16_t averageTemperature = SystemState::getAverageTemperature() / 10;
    if (averageTemperature >= EMERGENCY_TEMPERATURE) {
      setEmergency(emergency | EmergencyType::OverHeat);
      SystemState::setDeviceIsInShutDownMode(true);
    } else if (averageTemperature < FAN_FULL_SPEED_TEMPERATURE) {
      setEmergency(emergency & ~EmergencyType::OverHeat);
      SystemState::setDeviceIsInShutDownMode(false);
    }
  }

  void checkOverVoltage() {
    if (!SystemState::isChanged(SystemState::AverageVoltage)) {
      return;
    }
    uint32_t mV = SystemState::getAverageVoltage();
    if (mV >= EMERGENCY_VOLTAGE) {
      setEmergency(emergency | EmergencyType::OverVoltage);
      SystemState::setDeviceStatusIsOn(false);
    } else {
      setEmergency(emergency & ~EmergencyType::OverVoltage);
    }
  }

  void reportAmperage(int mAmp, int channel) {
    if (mAmp >= EMERGENCY_AMPERAGE) {
      setEmergency(emergency | (MosfetFault_1 << channel));
      SystemState::setDeviceStatusIsOn(false);
    }
  }

  void checkStopVoltageReached() {
    if (SystemState::getStopVoltage() > SystemState::getAverageVoltage()) {
      setEmergency(emergency | EmergencyType::StopVoltageReached);
      SystemState::setDeviceStatusIsOn(false);
    } else if (SystemState::isChanged(SystemState::StopVoltage)
        || (SystemState::isChanged(SystemState::DeviceStatusIsOn) 
            && SystemState::getDeviceStatusIsOn())) {
      setEmergency(emergency & ~EmergencyType::StopVoltageReached);
    }
  }

  void cleanUnnecessaryEmergencyFlags() {
    if (emergency & EmergencyType::MosfetFaults
        && SystemState::isChanged(SystemState::DeviceStatusIsOn)
        && SystemState::getDeviceStatusIsOn()) {
      setEmergency(emergency & ~(EmergencyType::MosfetFaults));
    }
  }

  void updateOnOffState() {
    checkOverHeat();
    checkOverVoltage();
    checkStopVoltageReached();
    cleanUnnecessaryEmergencyFlags();
    bool isOn = SystemState::getDeviceStatusIsOn() && SystemState::getDesiredAmperage() >= MIN_CURRENT_MA;
    digitalWrite(AMPERAGE_ON_OFF_PIN, isOn ? HIGH : LOW);
  }

  uint16_t getMainEmergency() {
    for (uint16_t mask = 1; mask != (1 << 16); mask = (mask << 1)) {
      if (emergency & mask) {
        return mask;
      }
    }
    return EmergencyType::Calmness;
  }

  const __FlashStringHelper *emergencyToString(uint16_t emergency) {
    if (emergency == Calmness) {
      return F("Calmness");
    }
    if (emergency & OverHeat) {
      return F("Over Heat");
    }
    if (emergency & MosfetFault_1) {
      return F("Mosfet Fault #1");
    }
    if (emergency & MosfetFault_2) {
      return F("Mosfet Fault #2");
    }
    if (emergency & MosfetFault_3) {
      return F("Mosfet Fault #3");
    }
    if (emergency & MosfetFault_4) {
      return F("Mosfet Fault #4");
    }
    if (emergency & MosfetFault_5) {
      return F("Mosfet Fault #5");
    }
    if (emergency & MosfetFault_6) {
      return F("Mosfet Fault #6");
    }
    if (emergency & OverVoltage) {
      return F("Over Voltage");
    }
    if (emergency & StopVoltageReached) {
      return F("Stop Voltage Reached");
    }
    return F("Emergency");
  }
};

namespace FanManager {
  void updateFanSpeed() {
    if (!SystemState::isChanged(SystemState::AverageTemperature)) {
      return;
    }

    int16_t averageTemperature = SystemState::getAverageTemperature() / 10;
    if (averageTemperature >= FAN_START_TEMPERATURE) {
      digitalWrite(FAN_ON_OFF_PIN, HIGH);
    } else if (averageTemperature <= FAN_STOP_TEMPERATURE) {
      digitalWrite(FAN_ON_OFF_PIN, LOW);
    }
    int16_t fanDutyCycle = map(averageTemperature, FAN_START_TEMPERATURE, FAN_FULL_SPEED_TEMPERATURE, 0, 79);
    OCR2B = constrain(fanDutyCycle, 0, 79);
  }

  void setup()  {
    pinMode(FAN_ON_OFF_PIN, OUTPUT);

    pinMode(FAN_PWM_PIN, OUTPUT); // Only works with Pin 3
    // Fast PWM Mode, Prescaler = /8
    // PWM on Pin 3, Pin 11 disabled
    // 16Mhz / 8 / (79 + 1) = 25Khz
    TCCR2A = _BV(COM2B1) | _BV(WGM21) | _BV(WGM20);
    TCCR2B = _BV(WGM22) | _BV(CS21);
    // Set TOP and initialize duty cycle to zero(0)
    OCR2A = 79;   // TOP - DO NOT CHANGE, SETS PWM PULSE RATE
    OCR2B = 1;    // duty cycle for Pin 3 (0-79) generates 1 500nS pulse even when 0
  }
};

namespace FanTemperatureReader {
  static AverageValueCalculator<int, uint32_t> thermistorPinValue;

  void setup() {
    pinMode(THERMISTOR_PIN, INPUT);
  }

  void makeMeasurement() {
    thermistorPinValue.addMeasurement(analogRead(THERMISTOR_PIN));
  }

  void updateAverageTemperatureValue() {
    float temperature = thermistorPinValue.calculateAverageValue();

    // convert the value to resistance
    temperature = 1023 / temperature - 1;
    temperature = THERMISTOR_SERIES_RESISTOR / temperature;

    temperature = temperature / THERMISTOR_NOMINAL;     // (R/Ro)
    temperature = log(temperature);                  // ln(R/Ro)
    temperature /= B_COEFFICIENT;                   // 1/B * ln(R/Ro)
    temperature += 1.0 / (TEMPERATURE_NOMINAL + 273.15); // + (1/To)
    temperature = 1.0 / temperature;                 // Invert
    temperature -= 273.15;                         // convert to C

    SystemState::setAverageTemperature(constrain(round(temperature * 10), -999, 2000)); // 4 digits
  }
};


namespace SdCardLogger {
  static struct {
      bool isFault:1;
      uint16_t lastOffWrite:2;
      uint16_t logNumber:13;
  } state = {false, 1, 0};
   
  static SdFat sd;

  static const PROGMEM char log[3] = "log";
  static const PROGMEM char dotCsv[4] = ".csv";

  static inline void setFault() {
    state.isFault = true;
    SystemState::setChangeFlag(SystemState::SdCardLogFile);
  }

  bool isFault() {
    return state.isFault;
  }

  // buffer should be 12 bytes long
  static const char *constructFileName(char *buffer) {
    memcpy_P(buffer, log, 3);
    int number = state.logNumber;
    for (int i = 6; i >= 3; --i) {
      buffer[i] = '0' + number % 10;
      number = number / 10;
    }
    memcpy_P(buffer + 7, dotCsv, 4);
    buffer[11] = '\0';
    return buffer;
  }

  static int atoi(const char *buffer, size_t length) {
    int val = 0;
    for (char *chr = buffer; chr < buffer + length; ++chr) {
      if (*chr == 0) {
        break;
      }
      val = val * 10 + (*chr - '0');
    }
    return val;
  }
  
  static void setup() {
    pinMode(SD_CARD_SC_PIN, OUTPUT);
    digitalWrite(SD_CARD_SC_PIN, HIGH);
    
    if (!sd.begin(SD_CARD_SC_PIN, SD_SCK_MHZ(50))) {
      setFault();
      return;
    }

    SdFile file;
    while (file.openNext(sd.vwd(), O_READ)) {
      const char fileName[13];
      if (!file.getName(fileName, sizeof(fileName))) {
        goto skip;
      }

      String sdCardFileName(fileName);
      if (strlen(fileName) != 11 ||
          strncmp_P(&fileName[0], log, 3) != 0 ||
          strncmp_P(&fileName[7], dotCsv, 4) != 0) {
        goto skip;
      }
      
      for (int i = 3; i < 7; ++i) {
        if (fileName[i] < '0' || fileName[i] > '9') {
          goto skip;
        }
      }

      state.logNumber = max(state.logNumber, atoi(&fileName[3], 4));
skip:
      file.close();
    }
  }
  
  void writeSystemState() {
    if (isFault()) {
      return;
    }

    if (!SystemState::getDeviceStatusIsOn() 
        || SystemState::getAverageVoltage() == 0
        || SystemState::getAverageAmperage() == 0) {
      if (state.lastOffWrite == 0) {
        return;
      }
      --state.lastOffWrite;
    } else {
      state.lastOffWrite = 3;
    }

    SdFile logFile;
    const char buffer[12];
    if (logFile.open(constructFileName(buffer), O_CREAT | O_WRITE | O_AT_END)) {
      size_t printSize = 0;
      printSize += logFile.print(SystemState::getAverageAmperage());
      printSize += logFile.print(',');
      printSize += logFile.print(SystemState::getAverageVoltage());
      printSize += logFile.print(',');
      printSize += logFile.print(SystemState::getAverageCharge(), 0);
      printSize += logFile.print(',');
      printSize += logFile.print(SystemState::getAverageEnergy(), 0);
      printSize += logFile.println();
      logFile.close();
      if (printSize == 0) {
        setFault();
      }
    } else {
      setFault();
    }
  }

  void changeFile() {
    if (isFault()) {
      return;
    }
    if (++state.logNumber > 9999) {
      state.logNumber = 0;
    }
    SystemState::setChangeFlag(SystemState::SdCardLogFile);
    const char buffer[12];
    SdFile logFile;
    if (!logFile.open(constructFileName(buffer), O_CREAT | O_WRITE | O_TRUNC)) {
      setFault();
      return;
    }
    if (logFile.println(F("Amperage(mA),Voltage(mV),Charge(mAh),Energy(mWh)")) == 0) {
      setFault();
    }
    logFile.close();
  }

  void printFileName(const Print &printer) {
    const char buffer[12];
    printer.print(constructFileName(buffer));
  }
};


namespace PersistenceStateManager {
  static uint8_t cycleCount;
  
  struct PersistenceState {
    float averageCharge;
    float averageEnergy;
  };

  uint16_t getOffsetAddress(uint16_t shift = 0) {
    uint16_t offsetAddress;
    uint16_t offsetStorageAddress = EEPROM.length() - sizeof(offsetAddress);
    EEPROM.get(offsetStorageAddress, offsetAddress);
    offsetAddress += shift;
    if (offsetAddress >= offsetStorageAddress) {
      offsetAddress = 0;
    }
    return offsetAddress;
  }

  void setup() {
    PersistenceState persistenceState;
    EEPROM.get(getOffsetAddress(), persistenceState);
    SystemState::restoreAverageChargeAndEnergy(persistenceState.averageCharge, persistenceState.averageEnergy);
  }

  uint16_t shiftAddress() {
    uint16_t offsetAddress = getOffsetAddress(sizeof(PersistenceState));
    EEPROM.put(EEPROM.length() - sizeof(offsetAddress), offsetAddress);
    return offsetAddress;
  }

  void preserve() {
    if (++cycleCount < SD_CARD_DUMP_INTERVAL_CYCLES) {
      return;
    }
    cycleCount = 0;
    bool needShift = random(1000) == 0;
    PersistenceState persistenceState = {
      SystemState::getAverageCharge(),
      SystemState::getAverageEnergy(),
    };

    EEPROM.put(needShift ? shiftAddress() : getOffsetAddress(), persistenceState);
  }
};



namespace GaugeReader {
  Adafruit_INA219 ina219 = Adafruit_INA219(INA219_I2C_ADDRESS);

  void setup() {
    ina219.begin();
  }

  void makeMeasurement() {
    float amperage = ina219.getShuntVoltage_mV() * 60;
    if (amperage < NOICE_AMPERAGE) {
      amperage = 0;
    } else if (amperage < 3778) {
      amperage += amperage * 0.009;
    } else if (amperage < 6091) {
      amperage += 34;
    } else {
      amperage += -0,011 * amperage + 101;
    }
    
    int32_t voltage = ina219.getBusVoltage_V() * 1000;
    if (voltage < NOISE_VOLTAGE) {
      voltage = 0;
    } else {
      voltage += (-27 * voltage + 430000) / 10000;
      voltage += amperage * 24 / 1000;
    }
    
    
    SystemState::setAmperage(amperage);
    SystemState::setVoltage(voltage);
  }
};
