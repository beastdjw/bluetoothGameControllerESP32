#include "Button.h"

//this class is assuming you use pullup resistance

Button::Button(byte pin) {
  this->pin = pin;
  lastReading = HIGH;
  init();
}

void Button::init() {
  pinMode(pin, INPUT_PULLUP);
  update();
}

void Button::update() {
    // You can handle the debounce of the button directly
    // in the class, so you don't have to think about it
    // elsewhere in your code
    byte newReading = digitalRead(pin);
    if (newReading != lastReading) {
      lastDebounceTime = millis();
    }
    if (state != lastReading) {
      if (millis() - lastDebounceTime > debounceDelay) {
        // Update the 'state' attribute only if debounce is checked
        state = newReading;
        if (!state) {
          countPressed++;
         // Serial.printf("button pressed: %d\n", countPressed);
        }
      }
    } 
    lastReading = newReading;
}

byte Button::getState() {
  update();
  return state;
}

bool Button::isPressed() {
  return (getState() == LOW);
}
