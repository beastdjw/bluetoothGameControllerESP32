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

bool Button::getChangeFlag() {
  return changeFlag;
}

void Button::setState(bool newState) {
    state = newState;
}

void Button::button_ISR() { 
  const bool readState = !digitalRead(buttonId);
  if(readState != currentState) {
    if(esp_timer_get_time() > lastChangeTime + debounceDelay) {
      currentState = readState;
      changeFlag = true;
    }
    lastChangeTime = esp_timer_get_time();
  }
//  if (esp_timer_get_time() > lastChangeTime + debounceDelay) {
//    bool readState = !digitalRead(buttonId);
//    if (readState != currentState) {
//      currentState = readState;
//      changeFlag = true;
//      lastChangeTime = esp_timer_get_time();
//    }
//  }
    
}
