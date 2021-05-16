#include "Button.h"

//this class is assuming you use pullup resistance

Button::Button(byte buttonId) {
  this->buttonId = buttonId;
  lastReading = HIGH;
  init();
}

void Button::init() {
  pinMode(buttonId, INPUT_PULLUP);
}

bool Button::changed() {
  return changeFlag;
}

bool Button::pushed() {
  return changed() && down();
}

bool Button::released() {
  return changed() && up(); 
}

bool Button::down() {
  return currentState;
}

bool Button::up() {
  return !down();
}

void Button::clearChangeFlag() {
  changeFlag = false;
}

byte Button::getButtonId() {
  return buttonId;
}

void Button::button_ISR() {
  if (esp_timer_get_time() > lastChangeTime + debounceDelay) {
    const boolean readState = !digitalRead(buttonId);
    if (readState != currentState) {
      currentState = readState;
      changeFlag = true;
      lastChangeTime = esp_timer_get_time();
    }
  }
  
  const bool readState = !digitalRead(buttonId);
  
}
