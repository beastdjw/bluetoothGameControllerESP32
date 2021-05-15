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

#ifndef BUTTONS_CLASS_H
#define BUTTONS_CLASS_H

#include <Arduino.h>

/**
 * This static-only class implements a system for getting user input from buttons.
 * It internally applies debounce periods and tracks whether a button press or release
 * has been "processed" by use of a Change Flag for each button.
 *
 * This is all interrupt driven, so there is no penalty to running code except when the
 * user actually presses a button. This also means, implicitly, you must ensure that
 * any pins used for button inputs are capable of having interrupts attached to them.
 * On the Arduino Due (for example) all digital pins can be used in this way, but on
 * the Arduino Uno, only pins 2 and 3 can have interrupts attached.
 *
 * Website: https://github.com/Alarm-Siren/arduino-buttons
 *
 *  DEFINITION FILE
 *
 * @author      Nicholas Parks Young
 * @version     2.0.0
 */
 
class ButtonsClass final 
{
  public:

    /**
     * Initialize the buttons as attached to the specified pins and attach appropriate interrupts.
     * The index of each button in the buttonPins parameter array is preserved for the buttonId parameter
     * on accessor methods such as clicked, down etc. Hence, if you want to read the status of
     * the button attached to the pin specified in buttonPins[3], you could call clicked(3).
     *
     * @param buttonPins        pointer to an array of bytes, each being the number of a
     *                          pin with a button attached that is to be managed by this object.
     * @param numberOfButtons   Number of buttons and size of the buttonPins array
     * @return                  true on success, false on failure.
     */
    boolean begin(const byte* const buttonPins, byte numberOfButtons);

    /**
     * TO DO
     */
    //static boolean begin(std::initializer_list<byte> buttonPins);
    
    /**
     * Detach interrupts from the pins controlled by this object and free up all associated memory.
     * If the object has not been started with begin(), or begin() failed, calling this will do nothing.
     */
    void end();

    /**
     * Returns a boolean value indicating if the user has "clicked" the button,
     * defined as the button being down and the Change Flag set.
     * 
     *
     * @param buttonId          Index of the button whose status is to be checked.
     * @return                  true if the button has been clicked since the Change Flag
     *                          was last cleared, false otherwise.
     */
    boolean clicked(byte buttonId);
    
    /**
     * Returns a boolean value indicating if the user has "released" the button,
     * defined as the button being up and the Change Flag set.
     *
     * @param buttonId          Index of the button whose status is to be checked.
     * @return                  true if the button has been clicked since the Change Flag
     *                          was last cleared, false otherwise.
     */
    boolean released(byte buttonId);
    
    /**
     * Returns a boolean value indicating if the button is currently "down"/"pressed".
     * This return value is independent of the state of the Change Flag.
     * It is the opposite of the up() method.
     *
     * @param buttonId          Index of the button whose status is to be checked.
     * @return                  true if the button is down.
     */
    boolean down(byte buttonId);

    /**
     * Returns a boolean value indicating if the button is currently "up"/"not pressed".
     * This return value is independent of the state of the Change Flag.
     * It is the opposite of the down() method.
     *
     * @param buttonId          Index of the button whose status is to be checked.
     * @return                  true if the button is up.
     */
    boolean up(byte buttonId);

    /**
     * Returns a boolean value indicating if the button's state has changed since the 
     * Change Flag was last cleared.
     * This return value is independent of whether the button itself is up or down, it
     * need only have changed.
     *
     * @param buttonId          Index of the button whose status is to be checked.
     * @return                  true if the button's state has changed.
     */
    boolean changed(byte buttonId);

    /**
     * This method clears all Change Flags for all buttons.
     * Useful to call when entering or leaving a user-interaction context so that spurious
     * button presses during the "non-interactive" phase do not trigger an unexpected action.
     */
    void clearChangeFlag();
    
    /**
     * Clears the Change Flag on the specified button ID.
     * 
     * @param buttonId          Index of the button whose change flag is to be cleared.
     */
    void clearChangeFlag(byte buttonId);

    /**
     * Returns the number of buttons currently controlled by this class.
     *
     * @return    The number of buttons controlled by this class
     */
    byte numberOfButtons();

    //This class is a singleton so copying it around will have no effect
    //and the default constructor will do as there's nothing to construct.
    ButtonsClass() = default;
    ~ButtonsClass() = default;
    ButtonsClass& operator=(const ButtonsClass&) = default;
    ButtonsClass(const ButtonsClass&) = default;

  private:

    /**
     * Debounce period in milliseconds.
     */
    static const unsigned long DEBOUNCE_DELAY = 50;

    /**
     * This structure encompasses information relating to an individual button.
     */
    struct Button
    {
      /**
       * Stores the most recently measured state of the button.
       * true = pushed, false = not pushed.
       */
      boolean currentState;

      /**
       * This flag indicates is set to true when currentState changes,
       * and (optionally) set false when that state is read.
       */
      boolean changeFlag;

      /**
       * This records the last time that an Interrupt was triggered from this pin.
       * Used as part of the debounce routine.
       */
      unsigned long lastChangeTime;

      /**
       * Constructor for objects of Button.
       */
      Button() :
        currentState(false),
        changeFlag(false),
        lastChangeTime(0)
      { }
    };

    /**
     * This function is called whenever a button interrupt is fired.
     * It reads all the button states and updates their _buttonStatus objects
     * accordingly.
     */
    static void button_ISR();

    /**
     * Stores the number of buttons controlled by this class,
     * which is also the size of the _buttonPins and _buttonStatus arrays.
     */
    static byte _numberOfButtons;

    /**
     * This array stores pin numbers for each button controlled by this class.
     */
    static byte* _buttonPins;

    /**
     * This array stores Button objects for each button controlled by this class,
     * each containing relevant information for the servicing of the ISR.
     * Its volatile because its members may be modified by an ISR, so we need to
     * prevent register caching of member values.
     */
    static volatile Button* _buttonStatus;

    /**
     * Set to true if this class has been initialised, false otherwise.
     */
    static boolean _begun;
};

extern ButtonsClass Buttons;

#endif
