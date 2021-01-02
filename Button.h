#ifndef MY_BUTTON_H
#define MY_BUTTON_H

#include <Arduino.h>

class Button {
  
  private:
    byte pin;
    byte state;
    byte lastReading;
    unsigned long lastDebounceTime = 0;
    unsigned long debounceDelay = 30;
    int countPressed = 0;
 
    
  public:
    Button(byte pin);

    boolean pressHasBeenSent = false;
    boolean releaseHasBeenSent = true;

    void init();
    void update();

    byte getState();
    bool isPressed();
};

#endif
