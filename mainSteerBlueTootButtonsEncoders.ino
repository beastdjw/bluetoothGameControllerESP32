#include <BleGamepad.h>
#include <BleConnectionStatus.h>
#include "Button.h"
#include "Rotary.h"
#include "RotaryKnob.h"
#include <driver/adc.h>
#include <driver/gpio.h>

//pins, see https://randomnerdtutorials.com/esp32-pinout-reference-gpios/
//GPIO34-39 can only be set as input mode and do not have software pullup or pulldown functions.
#define ONBOARD_LED GPIO_NUM_2
#define BATTERY_LED GPIO_NUM_4
#define BUTTON1 GPIO_NUM_32
#define BUTTON2 GPIO_NUM_33
#define ROTARYENCODER1BUTTON GPIO_NUM_31
#define ROTARYENCODER1A GPIO_NUM_25
#define ROTARYENCODER1B GPIO_NUM_26
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
BleGamepad bleGamepad("WheelButton", "BeastDJW", percentCharged);
Button button1(BUTTON1);
Button button2(BUTTON2);
Button buttons[] = {button1, button2};
int bleButtons[] = {BUTTON_1, BUTTON_2};
int numberOfButtons = *(&buttons + 1) - buttons;

RotaryKnob rotaryKnob1 = RotaryKnob(ROTARYENCODER1A, ROTARYENCODER1B, ROTARYENCODER_PRESSTIME);
RotaryKnob rotaryKnobs[] = {rotaryKnob1};
int bleRotaryKnobPins[] = {BUTTON_3, BUTTON_4};
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
  pinMode(ONBOARD_LED,OUTPUT);
  pinMode(GPIO_NUM_4,OUTPUT);
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
  attachInterrupt(ROTARYENCODER1A, rotaryKnob1ISR, CHANGE);
  attachInterrupt(ROTARYENCODER1B, rotaryKnob1ISR, CHANGE);
  #endif
  
  //the battery dac pin for measuring the voltage (percent battery)
 // adc2_config_channel_atten( ADC2_CHANNEL_7, ADC_ATTEN_DB_11 );

  //start bluetooth
  batteryCheck();
  bleGamepad.begin();
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
    if (buttons[i].isPressed()) {    
      standbyCounter = 0;                           //reset the standbyCounter
      if (!buttons[i].pressHasBeenSent) {    
        int64_t beforeSendingtime = esp_timer_get_time();
        bleGamepad.press(bleButtons[i]);
        int64_t totalSendingTime = esp_timer_get_time() - beforeSendingtime;
       // Serial.printf("%"PRIu64"\n", totalSendingTime);
        //Serial.printf("Sending Time press: % \n", beforeSendingtime - esp_timer_get_time());PriUint64<DEC>(x)
        buttons[i].pressHasBeenSent = true;
        buttons[i].releaseHasBeenSent = false;       
      }
    }
    else {
      if (!buttons[i].releaseHasBeenSent) {
      int64_t beforeSendingtime = esp_timer_get_time();
        bleGamepad.release(bleButtons[i]);
        int64_t totalSendingTime = esp_timer_get_time() - beforeSendingtime;
      //  Serial.printf("%"PRIu64"\n", totalSendingTime);
        buttons[i].releaseHasBeenSent = true; 
        buttons[i].pressHasBeenSent = false;
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
            rotaryKnobs[i].setReleaseNeeded(false);
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
void rotaryKnob1ISR() {
    rotaryKnobs[0].ISR();
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
//    esp_err_t r = adc2_get_raw( ADC2_CHANNEL_7, ADC_WIDTH_12Bit, &read_raw);
//    Serial.printf("Raw input, battery: %d\n", read_raw);
//    float voltage = float(read_raw)* 3.50f / 4096.0f * 2.0f;
//    if (voltage < 2.7) {
//      percentCharged = 0.0f;
//    }
//    else if (voltage > 3.85 ) {
//      percentCharged = 100.0f;
//    }
//    else {
//      percentCharged = ((voltage-2.7f)*100)/1.2f;
//    }
//    Serial.printf("Voltage battery: %.2f \n", voltage );
//    Serial.printf("Percentage battery: %d\n", int(percentCharged));
//    bleGamepad.setBatteryLevel(int(percentCharged));

    read_raw = analogRead(BATTERY_READ_VOLTAGE_PIN);
  //  Serial.printf("read_raw = %d \n", read_raw);
    voltage_value = (read_raw * 3.3 * 2) / float(4095) + 0.2;
 //   Serial.printf("Voltage = %f volt\n", voltage_value);
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
    if (int(percentCharged) < 30) {
      digitalWrite(GPIO_NUM_4,HIGH);
    } 
    else {
      digitalWrite(GPIO_NUM_4,LOW);
    }
    
    bleGamepad.setBatteryLevel(int(percentCharged));
}

void standbyModeCheck() {
  standbyCounter++;
  if (standbyCounter >= MINUTES_TO_STANDBY) {
     countTimerInterrupt = 0; //is this necessary?
     digitalWrite(ONBOARD_LED,LOW);
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
