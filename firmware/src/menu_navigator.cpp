#include <SSD1306AsciiWire.h>
#include <ClickEncoder.h>

#include "menu_navigator.h"
#include "custom_menu.h"
#include "system_state.h"

MenuNavigator::MenuNavigator(ClickEncoder &encoder, SSD1306AsciiWire &oled) : 
    encoder(encoder), oled(oled) {}


void MenuNavigator::processInput() {
  int16_t encoderValue = encoder.getValue();
  if (encoderValue != 0) {
    topMenu.processMoveEvent(encoderValue);
  }
  ClickEncoder::Button button = encoder.getButton();
  if (button == ClickEncoder::Clicked) {
    topMenu.processEnterEvent();
  } else if (button == ClickEncoder::DoubleClicked) {
    topMenu.processEnterEvent();
    topMenu.processEnterEvent();
  }
}

void MenuNavigator::updateOutput() {
  CustomMenuPrintContext context = {oled, SystemState::isFirstLoop()};
  topMenu.print(context);
}
