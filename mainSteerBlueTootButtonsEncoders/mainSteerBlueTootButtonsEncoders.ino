#include <BleGamepad.h>
#include <BleConnectionStatus.h>
#include "Button.h"
#include "Rotary.h"
#include "RotaryKnob.h"
#include <driver/adc.h>
#include <driver/gpio.h>
 
//pins, see https://randomnerdtutorials.com/esp32-pinout-reference-gpios/
//GPIO34-39 can only be set as input mode and do not have software pullup or pulldown functions.
#define ONBOARD_LED GPIO_NUM_2 //this one is used by rotaryencoder2button as well
#define BATTERY_LED GPIO_NUM_4
//flippers
#define BUTTON1 GPIO_NUM_32
#define BUTTON2 GPIO_NUM_33
//leftabove
#define ROTARYENCODER2BUTTON GPIO_NUM_2  //this one is used by the onboard led as well
#define BUTTON4 GPIO_NUM_13
#define BUTTON5 GPIO_NUM_15
#define BUTTON6 GPIO_NUM_0
#define ROTARYENCODER2A GPIO_NUM_14
#define ROTARYENCODER2B GPIO_NUM_12
//rightabove
#define BUTTON9 GPIO_NUM_39
#define BUTTON10 GPIO_NUM_35
#define BUTTON11 GPIO_NUM_27
#define ROTARYENCODER1BUTTON GPIO_NUM_36
#define ROTARYENCODER1A GPIO_NUM_25
#define ROTARYENCODER1B GPIO_NUM_26
//leftunder
#define BUTTON15 GPIO_NUM_4
#define BUTTON16 GPIO_NUM_16
#define BUTTON17 GPIO_NUM_17
#define ROTARYENCODER3BUTTON GPIO_NUM_5
#define ROTARYENCODER3A GPIO_NUM_18
#define ROTARYENCODER3B GPIO_NUM_19
//rightunder
#define BUTTON21 GPIO_NUM_21
#define BUTTON22 GPIO_NUM_3  //NEEDS PULLUP external
#define BUTTON23 GPIO_NUM_1
//#define ROTARYENCODER4BUTTON GPIO_NUM_TEWEINIG!!!!!!!!
#define ROTARYENCODER4A GPIO_NUM_23
#define ROTARYENCODER4B GPIO_NUM_22

#define BATTERY_READ_VOLTAGE_PIN GPIO_NUM_34

//times
#define MINUTES_TO_STANDBY 10
#define ROTARYENCODER_PRESSTIME 20

RTC_DATA_ATTR int bootCount = 0; //stays in memory after standby
volatile int countTimerInterrupt;
int standbyCounter = 0;
float percentCharged = 100;
int read_raw;
float voltage_value;
bool pushedSent = false;


//the init for the timerInterrupt
hw_timer_t * timer = NULL;
portMUX_TYPE timerMux = portMUX_INITIALIZER_UNLOCKED;

//inititialization knobs
BleGamepad bleGamepad("Wheel BeastDJW", "BeastDJW", percentCharged);
//flippers
Button button1(BUTTON1);
Button button2(BUTTON2);
//leftabove buttons
Button button3(ROTARYENCODER2BUTTON);
Button button4(BUTTON4);
Button button5(BUTTON5);
Button button6(BUTTON6);
//rightabove buttons
Button button9(BUTTON9);
Button button10(BUTTON10);
Button button11(BUTTON11);
Button button12(ROTARYENCODER1BUTTON);
//lefunder buttons
Button button15(BUTTON15);
Button button16(BUTTON16);
Button button17(BUTTON17);
Button button18(ROTARYENCODER3BUTTON);
//rightunder buttons
Button button21(BUTTON21);
Button button22(BUTTON22);
Button button23(BUTTON23);
//Button button24(ROTARYENCODER4BUTTON);
Button buttons[] = {button1, button2, button3, button4, button5, button6, button9, button10, button11, button12, button15, button16, button17, button18, button21, button22, button23};
int bleButtons[] = {BUTTON_1, BUTTON_2, BUTTON_3, BUTTON_4, BUTTON_5, BUTTON_6, BUTTON_9, BUTTON_10, BUTTON_11, BUTTON_12, BUTTON_15, BUTTON_16, BUTTON_17, BUTTON_18, BUTTON_21, BUTTON_22, BUTTON_23};
int numberOfButtons = *(&buttons + 1) - buttons;

RotaryKnob rotaryKnob1 = RotaryKnob(ROTARYENCODER1A, ROTARYENCODER1B, ROTARYENCODER_PRESSTIME);
RotaryKnob rotaryKnob2 = RotaryKnob(ROTARYENCODER2A, ROTARYENCODER2B, ROTARYENCODER_PRESSTIME);
RotaryKnob rotaryKnob3 = RotaryKnob(ROTARYENCODER3A, ROTARYENCODER3B, ROTARYENCODER_PRESSTIME);
RotaryKnob rotaryKnob4 = RotaryKnob(ROTARYENCODER4A, ROTARYENCODER4B, ROTARYENCODER_PRESSTIME);
RotaryKnob rotaryKnobs[] = {rotaryKnob1, rotaryKnob2, rotaryKnob3, rotaryKnob4};
int bleRotaryKnobPins[] = {BUTTON_13, BUTTON_14, BUTTON_7, BUTTON_8, BUTTON_19, BUTTON_20, BUTTON_24, BUTTON_25};
int numberofRotaryKnobs = *(&rotaryKnobs + 1) - rotaryKnobs;

// Code with critica section for the sleepmode (standby) timer
void IRAM_ATTR onTime() {
  portENTER_CRITICAL_ISR(&timerMux);
  countTimerInterrupt++;
  portEXIT_CRITICAL_ISR(&timerMux);
}

/************************** S E T  U P *****************************************************/
void setup() {
  setCpuFrequencyMhz(80); //20-30% batterylife saver
  Serial.begin(115200);
  delay(1000);
  Serial.println("Initializing...");
  pinMode(ONBOARD_LED,OUTPUT);
  //pinMode(GPIO_NUM_4,OUTPUT);
  pinMode(BATTERY_READ_VOLTAGE_PIN,INPUT);
  
  //Increment boot number and print it every reboot
  ++bootCount;
  Serial.println("Boot number: " + String(bootCount));
  //Print the wakeup reason for ESP32
  print_wakeup_reason();
  
  //Configure pin as ext0 wake up source for HIGH logic level to activate esp32 out of sleep (standby)
  esp_sleep_enable_ext0_wakeup(BUTTON1, LOW);
  
  // Configure the Prescaler at 80 the quarter of the ESP32 is cadence at 80Mhz
  //80000000 / 80 = 1000000 tics / seconde;
  timer = timerBegin(0, 80, true);                
  timerAttachInterrupt(timer, &onTime, true);    
  // Set the timer interrupt every minute
  timerAlarmWrite(timer, 60000000, true);           
  timerAlarmEnable(timer);

  attachInterrupt(buttons[0].getButtonId(), buttonsISR1, CHANGE);
  attachInterrupt(buttons[1].getButtonId(), buttonsISR1, CHANGE);
  attachInterrupt(buttons[2].getButtonId(), buttonsISR2, CHANGE);
  attachInterrupt(buttons[3].getButtonId(), buttonsISR3, CHANGE);
  attachInterrupt(buttons[4].getButtonId(), buttonsISR4, CHANGE);
  attachInterrupt(buttons[5].getButtonId(), buttonsISR5, CHANGE);
  attachInterrupt(buttons[6].getButtonId(), buttonsISR6, CHANGE);
  attachInterrupt(buttons[7].getButtonId(), buttonsISR7, CHANGE);
  attachInterrupt(buttons[8].getButtonId(), buttonsISR8, CHANGE);
  attachInterrupt(buttons[9].getButtonId(), buttonsISR9, CHANGE);
  attachInterrupt(buttons[10].getButtonId(), buttonsISR10, CHANGE);
  attachInterrupt(buttons[11].getButtonId(), buttonsISR11, CHANGE);
  attachInterrupt(buttons[12].getButtonId(), buttonsISR12, CHANGE);
  attachInterrupt(buttons[13].getButtonId(), buttonsISR13, CHANGE);
  attachInterrupt(buttons[14].getButtonId(), buttonsISR14, CHANGE);
  attachInterrupt(buttons[15].getButtonId(), buttonsISR15, CHANGE);
  attachInterrupt(buttons[16].getButtonId(), buttonsISR16, CHANGE);

  //the ISRs for the rotaryencoders
  #ifdef ROTARYENCODER1A
  attachInterrupt(ROTARYENCODER1A, rotaryKnobISR1, CHANGE);
  attachInterrupt(ROTARYENCODER1B, rotaryKnobISR1, CHANGE);
  attachInterrupt(ROTARYENCODER2A, rotaryKnobISR2, CHANGE);
  attachInterrupt(ROTARYENCODER2B, rotaryKnobISR2, CHANGE);
  attachInterrupt(ROTARYENCODER3A, rotaryKnobISR3, CHANGE);
  attachInterrupt(ROTARYENCODER3B, rotaryKnobISR3, CHANGE);
  attachInterrupt(ROTARYENCODER4A, rotaryKnobISR4, CHANGE);
  attachInterrupt(ROTARYENCODER4B, rotaryKnobISR4, CHANGE);
  #endif
  
  //the battery dac pin for measuring the voltage (percent battery)
 // adc2_config_channel_atten( ADC2_CHANNEL_7, ADC_ATTEN_DB_11 );

  //start bluetooth
  batteryCheck();
  bleGamepad.begin(25, 0, false, false, false, false, false, false, false, false, false, false, false, false, false);
}

/************************** E N D  S E T  U P *****************************************************/

/************************** L O O P *****************************************************/
void loop() {
  
  if(bleGamepad.isConnected()) {
    digitalWrite(ONBOARD_LED,HIGH);
    scanButtons();
    scanRotaryEncoders();
  }     
  else {
    digitalWrite(ONBOARD_LED,LOW);
  }
  oneMinuteProcedure(); 
}
/************************** E N D  L O O P ***********************************************/



void scanButtons() {
  for(int i=0; i<numberOfButtons; i++) {
    buttons[i].button_ISR();
    if (buttons[i].pushed()) {
      standbyCounter = 0;                           //reset the standbyCounter
      bleGamepad.press(bleButtons[i]);
      buttons[i].clearChangeFlag();
    }
    else {
      if (buttons[i].released()) {
        bleGamepad.release(bleButtons[i]);
        buttons[i].clearChangeFlag();
      }
    }
  }   
}

void scanRotaryEncoders() {
    for(int i=0; i<numberofRotaryKnobs; i++) {
      if (rotaryKnobs[i].isSendingPressButton()) {       
         if(rotaryKnobs[i].isReleaseNeeded()) {
            bleGamepad.release(bleRotaryKnobPins[i*2]);
            bleGamepad.release(bleRotaryKnobPins[i*2+1]);
            rotaryKnobs[i].setSendingPressButton(false);
            rotaryKnobs[i].releaseDone();
         }
      }
      else if (rotaryKnobs[i].changed()) {
          if (rotaryKnobs[i].isUp()) {
            rotaryKnobs[i].counter--;
            bleGamepad.press(bleRotaryKnobPins[i*2]);
          } 
          else {
            rotaryKnobs[i].counter++;
            bleGamepad.press(bleRotaryKnobPins[i*2+1]);
          }
          rotaryKnobs[i].setSendingPressButton(true);
      }
    }
}

void buttonsISR0() {
   buttons[0].button_ISR();
}
void buttonsISR1() {
   buttons[1].button_ISR();
}
void buttonsISR2() {
   buttons[2].button_ISR();
}
void buttonsISR3() {
   buttons[3].button_ISR();
}
void buttonsISR4() {
   buttons[4].button_ISR();
}
void buttonsISR5() {
   buttons[5].button_ISR();
}
void buttonsISR6() {
   buttons[6].button_ISR();
}
void buttonsISR7() {
   buttons[7].button_ISR();
}
void buttonsISR8() {
   buttons[8].button_ISR();
}
void buttonsISR9() {
   buttons[9].button_ISR();
}
void buttonsISR10() {
   buttons[10].button_ISR();
}
void buttonsISR11() {
   buttons[11].button_ISR();
}
void buttonsISR12() {
   buttons[12].button_ISR();
}
void buttonsISR13() {
   buttons[13].button_ISR();
}
void buttonsISR14() {
   buttons[14].button_ISR();
}
void buttonsISR15() {
   buttons[15].button_ISR();
}
void buttonsISR16() {
   buttons[16].button_ISR();
}

// rotate is called anytime the rotary inputs change state.
void rotaryKnobISR1() {
    rotaryKnobs[0].ISR();
}
void rotaryKnobISR2() {
    rotaryKnobs[1].ISR();
}
void rotaryKnobISR3() {
    rotaryKnobs[2].ISR();
}
void rotaryKnobISR4() {
    rotaryKnobs[3].ISR();
}

void oneMinuteProcedure() {
  if (countTimerInterrupt > 0) {
      portENTER_CRITICAL(&timerMux);
      countTimerInterrupt--;
      portEXIT_CRITICAL(&timerMux);
      batteryCheck();
      standbyModeCheck();
  }
}

void batteryCheck() {

    read_raw = analogRead(BATTERY_READ_VOLTAGE_PIN);
    voltage_value = (read_raw * 3.3 * 2) / float(4095) + 0.2;
    if (voltage_value < 2.7) {
      percentCharged = 0.0f;
    }
    else if (voltage_value > 4.0 ) {
      percentCharged = 100.0f;
    }
    else {
      percentCharged = ((voltage_value-2.7f)*100)/1.3f;
    }
    
    bleGamepad.setBatteryLevel(int(percentCharged));
}

void standbyModeCheck() {
  standbyCounter++;
  if (standbyCounter >= MINUTES_TO_STANDBY) {
     countTimerInterrupt = 0; //is this necessary?
     esp_deep_sleep_start();
  }
}

//Function that prints the reason by which ESP32 has been awaken from sleep
void print_wakeup_reason(){
  esp_sleep_wakeup_cause_t wakeup_reason;
  wakeup_reason = esp_sleep_get_wakeup_cause();
  switch(wakeup_reason)
  {
    case 1  : Serial.println("Wakeup caused by external signal using RTC_IO"); break;
    case 2  : Serial.println("Wakeup caused by external signal using RTC_CNTL"); break;
    case 3  : Serial.println("Wakeup caused by timer"); break;
    case 4  : Serial.println("Wakeup caused by touchpad"); break;
    case 5  : Serial.println("Wakeup caused by ULP program"); break;
    default : Serial.println("Wakeup was not caused by deep sleep"); break;
  }
}
