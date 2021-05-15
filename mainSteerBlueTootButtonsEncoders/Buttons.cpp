/*
 *  Arduino Buttons Library
 *  An interrupt-driven, fully-debounced class to manage input from physical buttons on the Arduino platform.
 *
 *  Copyright (C) 2017 Nicholas Parks Young
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301
 *  USA
 */

/**
 * This static-only class implements a system for getting user input from buttons.
 * It internally applies debounce periods and tracks whether a button press or release
 * has been "processed".
 *
 * This is all interrupt driven, so there is no penalty to running code except when the
 * user actually presses a button. This also means, implicitly, you must ensure that
 * any pins used for button inputs are capable of having interrupts attached to them.
 * On the Arduino Due (for example) all digital pins can be used in this way, but on
 * the Arduino Uno, only pins 2 and 3 can have interrupts attached.
 *
 * Website: https://github.com/Alarm-Siren/arduino-buttons
 *
 *  IMPLEMENTATION FILE
 *
 * @author      Nicholas Parks Young
 * @version     2.0.0
 */

#include "Buttons.h"
//#include <initializer_list>

byte ButtonsClass::_numberOfButtons = 0;
byte* ButtonsClass::_buttonPins = nullptr;
volatile ButtonsClass::Button* ButtonsClass::_buttonStatus = nullptr;
boolean ButtonsClass::_begun = false;

/**
 * TO DO
 */
/*boolean ButtonsClass::begin(std::initializer_list<byte> buttonPins)
{
    
  for (auto pin : buttonPins) {
    
  }
}*/

boolean ButtonsClass::begin(const byte* const buttonPins, byte numberOfButtons)
{
  // Abort if the buttonPins array is null
  if (nullptr == buttonPins)
    return false;
  
  // Abort if Buttons has already been started
  if (_begun)
    return false;

  // Setup internal storage buffers, etc.
  _numberOfButtons = numberOfButtons;
  _buttonPins = new byte[numberOfButtons];
  _buttonStatus = new Button[numberOfButtons];
 
  //Make sure that the memory was successfully allocated.
  if (!_buttonPins || !_buttonStatus) {
    return false;
  }

  // Set up the input pins themselves.
  for (byte i = 0; i < numberOfButtons; i++) {
    _buttonPins[i] = buttonPins[i];
    pinMode(buttonPins[i], INPUT_PULLUP);
  }
  
  // Need to wait some time before setting up the ISRs, otherwise you can get spurious
  // changes as the pullup hasn't quite done its magic yet.
  delay(10);

  //Set up the interrupts on the pins.
  for (byte i = 0; i < numberOfButtons; i++) {
    attachInterrupt(digitalPinToInterrupt(buttonPins[i]), &ButtonsClass::button_ISR, CHANGE);
  }

  // All done.
  _begun = true;
  return true;
}

void ButtonsClass::end()
{
  // If the object is already stopped, we don't need to do anything.
  if (!_begun)
    return;
  
  //Disable the interrupts
  for (byte i = 0; i < _numberOfButtons; i++) {
    detachInterrupt(digitalPinToInterrupt(_buttonPins[i]));
  }
  
  //Destroy dynamic memory.
  delete[] _buttonPins;
  delete[] _buttonStatus;
  
  //Object has been stopped.
  _begun = false;
}

void ButtonsClass::button_ISR()
{
  for (byte i = 0; i < _numberOfButtons; i++) {
    const boolean readState = !digitalRead(_buttonPins[i]);
    if (readState != _buttonStatus[i].currentState) {
      if (millis() > _buttonStatus[i].lastChangeTime + DEBOUNCE_DELAY) {
        _buttonStatus[i].currentState = readState;
        _buttonStatus[i].changeFlag = true;

           
      }
      _buttonStatus[i].lastChangeTime = millis();
    }
  }
}

boolean ButtonsClass::clicked(byte buttonId)
{
  return changed(buttonId) && down(buttonId);
}

boolean ButtonsClass::released(byte buttonId)
{
  return changed(buttonId) && !down(buttonId);
}

boolean ButtonsClass::down(byte buttonId)
{
  if (!_begun)
    return false;
  
  return  _buttonStatus[buttonId].currentState;
}

boolean ButtonsClass::up(byte buttonId)
{
  return !down(buttonId);
}

boolean ButtonsClass::changed(byte buttonId)
{
  if (!_begun)
    return false;

//       if(_buttonStatus[0].changeFlag) {
//          Serial.println("moet wel altijd true zijn");
//        }
  return _buttonStatus[buttonId].changeFlag;
}

void ButtonsClass::clearChangeFlag()
{
  if (!_begun)
    return;
  
  for (byte i = 0; i < _numberOfButtons; i++) {
    _buttonStatus[i].changeFlag = false;
  }
}

void ButtonsClass::clearChangeFlag(byte buttonId)
{
  if (!_begun)
    return;

    _buttonStatus[buttonId].changeFlag = false;
}


byte ButtonsClass::numberOfButtons()
{
  if (_begun) {
    return _numberOfButtons;
  } else {
    return 0;
  }
}

ButtonsClass Buttons;
