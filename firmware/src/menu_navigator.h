#pragma once

class ClickEncoder;
class SSD1306AsciiWire;


class MenuNavigator {
  const ClickEncoder &encoder;
  SSD1306AsciiWire &oled;
public:
  MenuNavigator(ClickEncoder &encoder, SSD1306AsciiWire &oled);
  void processInput();
  void updateOutput();
};
