#include "RotaryKnob.h"
#include "Rotary.h"
#include "Arduino.h"

//this class is assuming you use pullup resistance

RotaryKnob::RotaryKnob(char _pin1, char _pin2, int pressTime) : rotary(_pin1,_pin2) {
  this->pressTime = pressTime * 1000;
  this->counter = counter;
  init();
  
}

void RotaryKnob::init() {
  pinMode(pin1, INPUT_PULLUP);
  pinMode(pin2, INPUT_PULLUP);
  
}

boolean RotaryKnob::changed() {
  return ( counter!=0 );
  
}

boolean RotaryKnob::isUp() {
  return ( counter > 0);
  
}

void RotaryKnob::setSendingPressButton(boolean set) {
  sendingPressButton = set;
  setTimerPressKeyDuration = esp_timer_get_time();
  
}

boolean RotaryKnob::isSendingPressButton() {
  return sendingPressButton;
  
}

void RotaryKnob::setReleaseNeeded(boolean set) {
  releaseNeeded = set;
}


boolean RotaryKnob::isReleaseNeeded() {  
  if (setTimerPressKeyDuration != 0) {
      if (esp_timer_get_time() - setTimerPressKeyDuration > pressTime) {
        setTimerPressKeyDuration = 0;
        releaseNeeded = true;
      }
      else {
        releaseNeeded = false;
      }
  return releaseNeeded;
  }
}


//rotate is called anytime the rotary inputs change state.
void RotaryKnob::ISR() {
  unsigned char result = rotary.process();
  if (result == DIR_CW) {
    counter++;
  } else if (result == DIR_CCW) {
    counter--;
  }
  //Serial.printf("ISR: counter = %d \n", counter);
  
}
