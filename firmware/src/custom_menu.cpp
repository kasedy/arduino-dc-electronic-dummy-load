#include <SSD1306AsciiWire.h>
#include <ClickEncoder.h>

#include "custom_menu.h"
#include "system_state.h"
#include "managers.h"
#include "helpers.h"


typedef void (*PrintFn)(const CustomMenuPrintContext &, CustomMenu::FocusStatus, CustomMenu::ActiveStatus);
typedef void (*ProcessMoveEventFn)(int16_t);
typedef bool (*ProcessEnterEventFn)(bool);

struct CustomMenuItemShadow {
  PrintFn printFn;
  bool canAcquireFocusVal;
  ProcessMoveEventFn processMoveEventFn;
  ProcessEnterEventFn processEnterEventFn;
};

class CustomMenuItem {
  const CustomMenuItemShadow &shadow;
public:
  CustomMenuItem(const CustomMenuItemShadow &shadow);
  void print(const CustomMenuPrintContext &context, CustomMenu::FocusStatus focusStatus, CustomMenu::ActiveStatus actionStatus);
  void processMoveEvent(int16_t moveValue);
  bool processEnterEvent(bool isActive);
  bool canAcquireFocus();
};


size_t CustomMenuPrintContext::getStringLength(const char *str) {
  return strlen(str);
}

size_t CustomMenuPrintContext::getStringLength(const __FlashStringHelper *str) {
  return strlen_P((PGM_P)str);
}

size_t CustomMenuPrintContext::getStringLength(const char c) {
  return 1;
}

void CustomMenuPrintContext::skipPrintChars(size_t amount) {
  oled.setCol(oled.col() + amount * (oled.fontWidth() + oled.letterSpacing()));
}

void CustomMenuPrintContext::setCol(uint8_t col) {
  oled.setCol(col * (oled.fontWidth() + oled.letterSpacing()));
}

void CustomMenuPrintContext::printInt(int32_t value, uint8_t firstDigits, uint8_t lastDigits) {
  value = constrainInt(value, firstDigits + lastDigits);
  int32_t divider = pow10(lastDigits);
  printIntPart(value / divider, firstDigits, ' ');
  oled.print('.');
  printIntPart(abs(value % divider), lastDigits, '0');
}

void CustomMenuPrintContext::printInt(bool change, int32_t value, uint8_t firstDigits, uint8_t lastDigits) {
  if (change || fullPaint) {
    printInt(value, firstDigits, lastDigits);
  } else {
    skipPrintChars(firstDigits + lastDigits + 1);
  }
}

void CustomMenuPrintContext::printIntPart(int32_t value, const uint8_t digits, const char filler) {
  for (int i = digits - 1; i > 0; --i) {
    if (value < pow10(i) && value > -pow10(i - 1)) {
      oled.print(filler);
    }
  }
  oled.print(value);  
}

  
int32_t CustomMenuPrintContext::constrainInt(int32_t value, const uint8_t digits) {
  int32_t lowLimit = -pow10(digits - 1);
  if (value < lowLimit) {
    value = lowLimit + 1;
  } else {
    int32_t toplimit = pow10(digits);
    if (value > toplimit) {
      value = toplimit - 1;
    }
  }
  return value;  
}


CustomMenu::CustomMenu(std::initializer_list<CustomMenuItem *> items) 
      : num_children(items.size()), 
      isActive(false), focusItem(-1), 
      isOldActive(false), oldFocusItem(-1) {
  size_t memSize = sizeof(CustomMenuItem *) * items.size();
  children = malloc(memSize);
  memcpy(children, items.begin(), memSize);
  processMoveEvent(1);
}

void CustomMenu::print(const CustomMenuPrintContext &printContext) {
  const SSD1306AsciiWire &oled = printContext.oled;
  for (int i = 0; i < num_children; ++i) {
    CustomMenuItem *child = children[i];
    CustomMenu::ActiveStatus activeStatus = getActiveStatus(i);
    CustomMenu::FocusStatus focusStatus = getFocusStatus(i);
    if (focusStatus == CustomMenu::LostFocus) {
      oled.setCursor(0, i);
      oled.print(' ');
    } else if (focusStatus == CustomMenu::AcquiredFocus) {
      oled.setCursor(0, i);
      oled.print('>');
    } else if (child->canAcquireFocus()) {
      oled.setCursor(0, i);
      printContext.skipPrintChars(1);
    } else {
      oled.setCursor(0, i);
    }
    child->print(printContext, focusStatus, activeStatus);
  }
  oldFocusItem = focusItem;
  isOldActive = isActive;
}

CustomMenu::ActiveStatus CustomMenu::getActiveStatus(int i) {
  if (isActive != isOldActive || focusItem != oldFocusItem) {
    if (i == focusItem && isActive) {
      return CustomMenu::AcquiredActive;
    }
    if (i == oldFocusItem && isOldActive) {
      return CustomMenu::LostActive;
    }
  } else if (i == focusItem && isActive) {
    return CustomMenu::Active;
  }
  return CustomMenu::NotActive;
}

CustomMenu::FocusStatus CustomMenu::getFocusStatus(int i) {
  if (isActive != isOldActive || focusItem != oldFocusItem) {
    if (i == focusItem && !isActive) {
      return CustomMenu::AcquiredFocus;
    }
    if (i == oldFocusItem && !isOldActive) {
      return CustomMenu::LostFocus;
    }
  } else if (i == focusItem && !isActive) {
    return CustomMenu::Focused;
  }
  return CustomMenu::NotFocused;
}

void CustomMenu::processMoveEvent(int16_t moveValue) {
  if (isActive) {
    children[focusItem]->processMoveEvent(moveValue);
    return;
  }
  
  int step = sgn(moveValue);
  while (moveValue != 0) {
    int8_t nextItem = getNextFocusableItem(step);
    if (nextItem == -1) {
      return;
    }
    focusItem = nextItem;
    moveValue -= step;
  }
}

int8_t CustomMenu::getNextFocusableItem(int step) {
  int newFocus = focusItem;
  do {
    newFocus += step;
    if (newFocus < 0 || newFocus >= num_children) {
      return -1;
    }
  } while(!children[newFocus]->canAcquireFocus());
  return newFocus;
}

CustomMenu* CustomMenu::processEnterEvent() {
  if (focusItem != -1) {
    isActive = children[focusItem]->processEnterEvent(isActive);
  }
  return this;
}

CustomMenuItem::CustomMenuItem(const CustomMenuItemShadow &shadow) : shadow(shadow) {}

void CustomMenuItem::print(const CustomMenuPrintContext &context, CustomMenu::FocusStatus focusStatus, CustomMenu::ActiveStatus actionStatus) {
  PrintFn printFn = pgm_read_ptr(&shadow.printFn);
  if (printFn) {
    printFn(context, focusStatus, actionStatus);
  }
}

void CustomMenuItem::processMoveEvent(int16_t moveValue) {
  ProcessMoveEventFn processMoveEventFn = pgm_read_ptr(&shadow.processMoveEventFn);
  if (processMoveEventFn) {
    processMoveEventFn(moveValue);
  }
}

bool CustomMenuItem::processEnterEvent(bool isActive) {
  ProcessEnterEventFn processEnterEventFn = pgm_read_ptr(&shadow.processEnterEventFn);
  return processEnterEventFn ? processEnterEventFn(isActive) : false;
}

bool CustomMenuItem::canAcquireFocus() {
  return pgm_read_byte(&shadow.canAcquireFocusVal);
}


namespace DeviceOnOffToggleMenuItem {
  void print(const CustomMenuPrintContext &context, CustomMenu::FocusStatus, CustomMenu::ActiveStatus activeStatus) {
    if (context.fullPaint || SystemState::isChanged(SystemState::DeviceStatusIsOn)) {
      context.lazyPrint(F("Status "));
      context.oled.print(SystemState::getDeviceStatusIsOn() ? F("On ") : F("Off"));
    }
  }

  bool processEnterEvent(bool isActive) {
    SystemState::setDeviceStatusIsOn(!SystemState::getDeviceStatusIsOn());
    return false;
  }  

  const CustomMenuItemShadow shadow PROGMEM = {print, true, nullptr, processEnterEvent};
};

CustomMenuItem deviceOnOffToggleMenuItem(DeviceOnOffToggleMenuItem::shadow);


namespace CapacityInfoMenuItem {
  void print(const CustomMenuPrintContext &context, CustomMenu::FocusStatus, CustomMenu::ActiveStatus activeStatus) {
    context.printInt(SystemState::isChanged(SystemState::AverageCharge), SystemState::getAverageCharge(), 3, 3);
    context.lazyPrint(F(" Ah "));
    context.printInt(SystemState::isChanged(SystemState::AverageCharge), (SystemState::getAverageEnergy() + 50) / 100, 4, 1);
    context.lazyPrint(F(" Wh"));
  }
  
  const CustomMenuItemShadow shadow PROGMEM = {print, false, nullptr, nullptr};
};

CustomMenuItem capacityInfoMenuItem(CapacityInfoMenuItem::shadow);


namespace SdFileLoggerMenuItem {
  void print(const CustomMenuPrintContext &context, CustomMenu::FocusStatus focusStatus, CustomMenu::ActiveStatus activeStatus) {
    if (context.fullPaint 
        || SystemState::isChanged(SystemState::SdCardLogFile)
        || focusStatus == CustomMenu::AcquiredFocus
        || focusStatus == CustomMenu::LostFocus) {
      if (SdCardLogger::isFault()) {
        context.oled.print(F("No SD"));
      } else {
        SdCardLogger::printFileName(context.oled);
      }
      if (focusStatus == CustomMenu::AcquiredFocus || focusStatus == CustomMenu::Focused) {
        context.oled.print(F(" (new)"));
      }
      context.oled.clearToEOL();
    }
  }

  bool processEnterEvent(bool isActive) {
    SdCardLogger::changeFile();
    SystemState::resetAverageChargeAndEnergy();
    return false;
  }

  const CustomMenuItemShadow shadow PROGMEM = {print, true, nullptr, processEnterEvent};
};

CustomMenuItem sdFileLoggerMenuItem(SdFileLoggerMenuItem::shadow);


namespace EmergencyInfoMenuItem {
  void print(const CustomMenuPrintContext &context, CustomMenu::FocusStatus, CustomMenu::ActiveStatus activeStatus) {
    if (context.fullPaint || SystemState::isChanged(SystemState::MainEmergency)) {
      uint16_t emergencyValue = EmergencyManager::getMainEmergency();
      if (emergencyValue == EmergencyManager::Calmness) {
        context.oled.clearToEOL();
      } else {
        context.oled.print(EmergencyManager::emergencyToString(emergencyValue));
      }
    }
  }

  const CustomMenuItemShadow shadow PROGMEM = {print, false, nullptr, nullptr};
};

CustomMenuItem emergencyInfoMenuItem(EmergencyInfoMenuItem::shadow);


namespace TemperatureAndWattInfoMenuItem {
  void print(const CustomMenuPrintContext &context, CustomMenu::FocusStatus, CustomMenu::ActiveStatus activeStatus) {
    context.skipPrintChars(1);
    context.printInt(SystemState::isChanged(SystemState::AverageTemperature), SystemState::getAverageTemperature(), 3, 1);
    context.lazyPrint(F(" *C "));
    context.printInt(SystemState::isChanged(SystemState::AveragePower), SystemState::getAveragePower(), 3, 3);
    context.lazyPrint(F(" W"));
  }

  const CustomMenuItemShadow shadow PROGMEM = {print, false, nullptr, nullptr};
};

CustomMenuItem temperatureAndWattInfoMenuItem(TemperatureAndWattInfoMenuItem::shadow);


namespace EmptySpacerMenuItem {
  const CustomMenuItemShadow shadow PROGMEM = {nullptr, false, nullptr, nullptr};  
};

CustomMenuItem emptySpacerMenuItem(EmptySpacerMenuItem::shadow);


namespace TwoValuesMenuItem {
  struct Object {
    bool isFine:1;
    bool activeCursorChanged:1;
    Object() : isFine(false), activeCursorChanged(false) {}
  };

  void print(Object &obj, const CustomMenuPrintContext &context, CustomMenu::FocusStatus, CustomMenu::ActiveStatus activeStatus, const __FlashStringHelper *name, 
      bool isLeftValueChanged, int32_t leftValue, bool isRightValueChanged, int32_t rightValue, bool customZero) {
    context.lazyPrint(name);
    context.lazyPrint(' ');
    context.printInt(isLeftValueChanged, leftValue, 2, 3);
    context.lazyPrint(F(" /"));
    if (context.fullPaint || obj.activeCursorChanged) {
      if (activeStatus == CustomMenu::LostActive || activeStatus == CustomMenu::NotActive) {
        context.oled.print(' ');
      } else if (activeStatus == CustomMenu::AcquiredActive || activeStatus == CustomMenu::Active) {
        context.oled.print(obj.isFine ? ':' : '>');
      }
    } else {
      context.skipPrintChars(1);
    }
    if (customZero && rightValue == 0) {
      context.lazyPrint(F(" -----"), isRightValueChanged);
    } else {
      context.printInt(isRightValueChanged, rightValue, 2, 3);
    }
    if (context.fullPaint || obj.activeCursorChanged) {
      if (activeStatus == CustomMenu::LostActive || activeStatus == CustomMenu::NotActive) {
        context.oled.print(' ');
      } else if (activeStatus == CustomMenu::AcquiredActive || activeStatus == CustomMenu::Active) {
        context.oled.print(obj.isFine ? ':' : '<');
      }
    } else {
      context.skipPrintChars(1);
    }
    obj.activeCursorChanged = false;
  }

  bool processEnterEvent(Object &obj, bool isActive) {
    obj.activeCursorChanged = true;
    if (!isActive) {
      return true;
    }
    obj.isFine = !obj.isFine;
    return obj.isFine;
  } 
};


namespace AmperageTwoValuesMenuItem {
  TwoValuesMenuItem::Object obj;
  
  void print(const CustomMenuPrintContext &context, CustomMenu::FocusStatus focusStatus, CustomMenu::ActiveStatus activeStatus) {
    bool isLeftValueChanged = SystemState::isChanged(SystemState::AverageAmperage);
    bool isRightValueChanged = SystemState::isChanged(SystemState::DesiredAmperage);
    int32_t leftValue = SystemState::getAverageAmperage();
    int32_t rightValue = SystemState::getDesiredAmperage();
    TwoValuesMenuItem::print(obj, context, focusStatus, activeStatus, F("Amp"), 
        isLeftValueChanged, leftValue, isRightValueChanged, rightValue, false);    
  }

  void processMoveEvent(int16_t moveValue) {
    int16_t multiplier = obj.isFine ? AMPERAGE_CHANGE_FINE_STEP : AMPERAGE_CHANGE_CORSE_STEP;
    int32_t adjustment = moveValue * multiplier;
    int32_t rightValue = SystemState::getDesiredAmperage() + adjustment;
    if (rightValue > 0 && rightValue < MIN_CURRENT_MA) {
      rightValue = adjustment > 0 ? MIN_CURRENT_MA : 0;
    }
    rightValue = constrain(rightValue, 0, MAX_CURRENT_MA);
    SystemState::setDesiredAmperage(rightValue);
  }
  
  bool processEnterEvent(bool isActive) {
    return TwoValuesMenuItem::processEnterEvent(obj, isActive);
  }

  const CustomMenuItemShadow shadow PROGMEM = {print, true, processMoveEvent, processEnterEvent}; 
}

CustomMenuItem amperageTwoValuesMenuItem(AmperageTwoValuesMenuItem::shadow);


namespace VoltageTwoValuesMenuItem {
  TwoValuesMenuItem::Object obj;
  
  void print(const CustomMenuPrintContext &context, CustomMenu::FocusStatus focusStatus, CustomMenu::ActiveStatus activeStatus) {
    bool isLeftValueChanged = SystemState::isChanged(SystemState::AverageVoltage);
    bool isRightValueChanged = SystemState::isChanged(SystemState::StopVoltage);
    int32_t leftValue = SystemState::getAverageVoltage();
    int32_t rightValue = SystemState::getStopVoltage();
    TwoValuesMenuItem::print(obj, context, focusStatus, activeStatus, F("Vlt"), 
        isLeftValueChanged, leftValue, isRightValueChanged, rightValue, true);    
  }

  void processMoveEvent(int16_t moveValue) {
    int32_t rightValue = SystemState::getStopVoltage();
    int32_t multiplier = obj.isFine ? 1 : 100;
    rightValue += moveValue * multiplier;
    rightValue = constrain(rightValue, 0, 32000);
    SystemState::setStopVoltage(rightValue);
  }

  bool processEnterEvent(bool isActive) {
    return TwoValuesMenuItem::processEnterEvent(obj, isActive);
  }
  const CustomMenuItemShadow shadow PROGMEM = {print, true, processMoveEvent, processEnterEvent}; 
};

CustomMenuItem voltageTwoValuesMenuItem(VoltageTwoValuesMenuItem::shadow);


CustomMenu topMenu = CustomMenu({&amperageTwoValuesMenuItem, 
                                 &voltageTwoValuesMenuItem, 
                                 &deviceOnOffToggleMenuItem, 
                                 &sdFileLoggerMenuItem, 
                                 &emptySpacerMenuItem, 
                                 &temperatureAndWattInfoMenuItem, 
                                 &capacityInfoMenuItem, 
                                 &emergencyInfoMenuItem});
