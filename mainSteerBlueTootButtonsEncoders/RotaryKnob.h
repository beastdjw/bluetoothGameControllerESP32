/*
 * RotaryKnob for RaceSteer.
 */

#include "Arduino.h"
#include "Rotary.h"

#ifndef rotaryknob_h
#define rotaryknob_h

class RotaryKnob
{
  private:

    unsigned long pressTime;
    boolean sendingPressButton = false;
    boolean releaseNeeded = true;
    int64_t setTimerPressKeyDuration = 0;
    Rotary rotary;
    

  public:
   //constructor
    RotaryKnob(char, char, int);
    byte pin1;
    byte pin2;
    //methods
    void init();
    boolean changed();
    boolean isUp();
    boolean isSendingPressButton();
    void setSendingPressButton(boolean);
    boolean isReleaseNeeded();
    void setReleaseNeeded(boolean);
    void ISR();
    int counter = 0;
   
};

#endif
