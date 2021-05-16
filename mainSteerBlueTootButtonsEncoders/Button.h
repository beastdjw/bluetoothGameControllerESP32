#ifndef MY_BUTTON_H
#define MY_BUTTON_H

#include <Arduino.h>

class Button {
  public:
    //boolean begin(const byte* const buttonPins, byte numberOfButtons);
    
    Button(byte buttonId);

    void init();
    //void update();
    //byte getState();

    bool changed();
    bool pushed();
    bool down();
    bool up();
    bool released();
    void clearChangeFlag();
    byte getButtonId();
    void button_ISR();
    
  
  private:
    byte numberOfButtons();
    byte buttonId;
    boolean pressHasBeenSent = false;
    boolean releaseHasBeenSent = true;
    byte state;
    bool currentState = false;
    bool changeFlag = false;
    byte lastReading;
    unsigned long lastChangeTime = 0;
    unsigned long debounceDelay = 30;
    

};

#endif
