#include <BleGamepad.h>
#include <BleConnectionStatus.h>
#include "Buttons.h"
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

//the init for the timerInterrupt
hw_timer_t * timer = NULL;
portMUX_TYPE timerMux = portMUX_INITIALIZER_UNLOCKED;

//inititialization knobs
BleGamepad bleGamepad("Wheel BeastDJW test", "BeastDJW", percentCharged);
byte pins[] = {BUTTON1,BUTTON2,ROTARYENCODER2BUTTON,BUTTON4,BUTTON5,BUTTON6,BUTTON9,BUTTON10,BUTTON11,ROTARYENCODER1BUTTON,BUTTON15,BUTTON16,BUTTON17,ROTARYENCODER3BUTTON,BUTTON21,BUTTON22,BUTTON23};
int numberOfButtons = sizeof pins / sizeof pins[0];

//paddleshifters 1&2
//leftabove buttons (ROTARYENCODER2BUTTON,BUTTON4,BUTTON5,BUTTON6)
//rightabove buttons (BUTTON9,BUTTON10,BUTTON11,ROTARYENCODER1BUTTON)
//lefunder buttons (BUTTON15,BUTTON16,BUTTON17,ROTARYENCODER3BUTTON)
//rightunder buttons (BUTTON21,BUTTON22,BUTTON23)
//Button button24(ROTARYENCODER4BUTTON), a short on gpio pins :-(
//Button buttons[] = {button1, button2, button3, button4, button5, button6, button9, button10, button11, button12, button15, button16, button17, button18, button21, button22, button23};
int bleButtons[] = {BUTTON_1, BUTTON_2, BUTTON_3, BUTTON_4, BUTTON_5, BUTTON_6, BUTTON_9, BUTTON_10, BUTTON_11, BUTTON_12, BUTTON_15, BUTTON_16, BUTTON_17, BUTTON_18, BUTTON_21, BUTTON_22, BUTTON_23};
//int bleButtons[] = {BUTTON_1, BUTTON_2, BUTTON_3, BUTTON_4, BUTTON_5, BUTTON_6, BUTTON_9, BUTTON_10};


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
  countTimerInterrupt++;//
  portEXIT_CRITICAL_ISR(&timerMux);
}

/************************** S E T  U P *****************************************************/
void setup() {
  setCpuFrequencyMhz(80); //20-30% batterylife saver
  Serial.begin(115200);
  delay(1000);
  Serial.println("Initializing...");

  // initialize (static) all buttons without the 2 encoders data signals 
  delay(1000);
  Buttons.begin(pins, numberOfButtons);
  delay(1000);
  
  //pinMode(ONBOARD_LED,INPUT_PULLUP);
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

  //the ISRs for the rotaryencoders
  #ifdef ROTARYENCODER1A
  attachInterrupt(ROTARYENCODER1A, rotaryKnobISR, CHANGE);
  attachInterrupt(ROTARYENCODER1B, rotaryKnobISR, CHANGE);
  attachInterrupt(ROTARYENCODER2A, rotaryKnobISR, CHANGE);
  attachInterrupt(ROTARYENCODER2B, rotaryKnobISR, CHANGE);
  attachInterrupt(ROTARYENCODER3A, rotaryKnobISR, CHANGE);
  attachInterrupt(ROTARYENCODER3B, rotaryKnobISR, CHANGE);
  attachInterrupt(ROTARYENCODER4A, rotaryKnobISR, CHANGE);
  attachInterrupt(ROTARYENCODER4B, rotaryKnobISR, CHANGE);
  #endif
  
  batteryCheck();
  
  bleGamepad.begin(25, 0, false, false, false, false, false, false, false, false, false, false, false, false, false);
}

/************************** E N D  S E T  U P *****************************************************/

/************************** L O O P *****************************************************/
void loop() {
  
  if(bleGamepad.isConnected()) {
    //digitalWrite(ONBOARD_LED,HIGH);
    scanButtons();
    scanRotaryEncoders();
  }     
  else {
    //digitalWrite(ONBOARD_LED,LOW);
  }
  oneMinuteProcedure(); 
}
/************************** E N D  L O O P ***********************************************/

void scanButtons() {
  for(int i=0; i<numberOfButtons; i++) {
    if (Buttons.clicked(i)) {    
      standbyCounter = 0;                           //reset the standbyCounter
      bleGamepad.press(bleButtons[i]);
      Buttons.clearChangeFlag(i);
    }
    else if(Buttons.released(i)) {
      bleGamepad.release(bleButtons[i]);
      Buttons.clearChangeFlag(i);
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

// rotate is called anytime the rotary inputs change state.
void rotaryKnobISR() {
    rotaryKnobs[0].ISR();
    rotaryKnobs[1].ISR();
    rotaryKnobs[2].ISR();
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
 //   Serial.printf("Percentage battery: %d\n", int(percentCharged));
//    if (int(percentCharged) < 40) {
//      digitalWrite(BATTERY_LED,HIGH);
//    } 
//    else {
//      digitalWrite(BATTERY_LED,LOW);
//    }
    
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
