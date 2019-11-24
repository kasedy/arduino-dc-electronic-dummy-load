#pragma once

#include "initializer_list.h"

class CustomMenuItem;

struct CustomMenuPrintContext {
  const SSD1306AsciiWire &oled;
  bool fullPaint;

  void printIntPart(int32_t value, const uint8_t digits, const char filler);
  int32_t constrainInt(int32_t value, const uint8_t digits);
  size_t getStringLength(const char *str);
  size_t getStringLength(const __FlashStringHelper *str);
  size_t getStringLength(const char c);

  template<typename T>
  void lazyPrint(T str, bool enforce = false) {
    if (fullPaint || enforce) {
      oled.print(str);
    } else {
      skipPrintChars(getStringLength(str));
    }
  }

  void skipPrintChars(size_t amount);
  void setCol(uint8_t col);
  void printInt(int32_t value, uint8_t firstDigits, uint8_t lastDigits);
  void printInt(bool change, int32_t value, uint8_t firstDigits, uint8_t lastDigits);
};


class CustomMenu {
  CustomMenuItem* const* children = nullptr;
  const uint8_t num_children:4;
  bool isActive:1;
  int8_t focusItem:4;
  bool isOldActive:1;
  int8_t oldFocusItem:4;
  
public:
  enum FocusStatus {
    AcquiredFocus,
    LostFocus,
    Focused,
    NotFocused,
  };
  
  enum ActiveStatus {
    AcquiredActive,
    LostActive,
    Active,
    NotActive,
  };

  CustomMenu(std::initializer_list<CustomMenuItem *> items);

  void print(const CustomMenuPrintContext &printContext);
  void processMoveEvent(int16_t moveValue);
  CustomMenu* processEnterEvent();

private:
  ActiveStatus getActiveStatus(int i);
  FocusStatus getFocusStatus(int i);
  int8_t getNextFocusableItem(int step);
};

extern CustomMenu topMenu;
