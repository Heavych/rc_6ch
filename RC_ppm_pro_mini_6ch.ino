// Arduino pro mini
// Atmega 328p (5v, 16 Mhz)
// Arduino as ISP

#include "Arduino.h"
#include <AltSoftSerial.h>
#include <EnableInterrupt.h>
#include "DFRobotDFPlayerMini.h"

#define SERIAL_PORT_SPEED 57600
#define RC_NUM_CHANNELS  6

#define RC_CH1  0
#define RC_CH2  1
#define RC_CH3  2
#define RC_CH4  3
#define RC_CH5  4
#define RC_CH6  5

#define RC_CH1_INPUT  A0
#define RC_CH2_INPUT  A1
#define RC_CH3_INPUT  A2
#define RC_CH4_INPUT  A3
#define RC_CH5_INPUT  A4
#define RC_CH6_INPUT  A5

AltSoftSerial altSerial;
DFRobotDFPlayerMini myDFPlayer;

uint16_t rc_values[RC_NUM_CHANNELS];
uint32_t rc_start[RC_NUM_CHANNELS];
volatile uint16_t rc_shared[RC_NUM_CHANNELS];

void rc_read_values() {
  noInterrupts();
  memcpy(rc_values, (const void *)rc_shared, sizeof(rc_shared));
  interrupts();
}

void calc_input(uint8_t channel, uint8_t input_pin) {
  if (digitalRead(input_pin) == HIGH) {
    rc_start[channel] = micros();
  } else {
    uint16_t rc_compare = (uint16_t)(micros() - rc_start[channel]);
    rc_shared[channel] = rc_compare;
  }
}

void calc_ch1() {
  calc_input(RC_CH1, RC_CH1_INPUT);
}
void calc_ch2() {
  calc_input(RC_CH2, RC_CH2_INPUT);
}
void calc_ch3() {
  calc_input(RC_CH3, RC_CH3_INPUT);
}
void calc_ch4() {
  calc_input(RC_CH4, RC_CH4_INPUT);
}
void calc_ch5() {
  calc_input(RC_CH5, RC_CH5_INPUT);
}
void calc_ch6() {
  calc_input(RC_CH6, RC_CH6_INPUT);
}
// ********************* LIGHTS *********************

const int lights = 5; // (PWM) R100 oM
const int sideLights = 3; // (PWM) Front R1 KoM, Rear R270 oM
const int spotLight = 2; // R100 oM
const int turnLights_l = 4; // Front 470oM | Side  R1 KoM | Rear R220 oM
const int turnLights_r = 7; // Rear R220 oM
const int stopLights = 12; // R110 oM
const int rearLight = 11; // R110 oM

const int motor_in1 =  10;
const int motor_in2 =  6;

/* Buzy
 D8,D9 - AltSoftSerial
*/

/* Free
  D13
  A6
  A7
*/

/* 2GB MicroSD
 01
  001_gaz_idle.wav
 02
  002_gaz_low.wav
 03
  003_gaz_high.wav
 mp3
   0001_gaz_start.wav
   0002_gear__3.wav
   0003_headlight.wav
   0004_gaz_idle.wav
   0005_hong_heavy.wav
 */
 
int ledState = LOW;
byte volSet = 0;
byte turnFlag = 0;
byte winchFlag = 0;

// byte soundFlag = 0;
int winchState = LOW;
const long interval = 400;
const long stopLightsInterval = 3000;
unsigned long previousMillis = 0;
unsigned long stopLightsPreviousMillis = 0;
byte stopLightsFlag = 0;
byte stopLightsTime = 0;
byte rearLightsFlag = 0;
byte mode = 1;

// ********************* LIGHTS END *********************

void setup() {
  digitalWrite(motor_in1, winchState);
  digitalWrite(motor_in2, winchState);
  Serial.begin(SERIAL_PORT_SPEED);
  altSerial.begin(9600);
  myDFPlayer.begin(altSerial);

  //pinMode(ledPin, OUTPUT);
  pinMode(lights, OUTPUT);
  pinMode(sideLights, OUTPUT);
  pinMode(spotLight, OUTPUT);
  pinMode(turnLights_l, OUTPUT);
  pinMode(turnLights_r, OUTPUT);
  pinMode(stopLights, OUTPUT);
  pinMode(rearLight, OUTPUT);

  pinMode(RC_CH1_INPUT, INPUT);
  pinMode(RC_CH2_INPUT, INPUT);
  pinMode(RC_CH3_INPUT, INPUT);
  pinMode(RC_CH4_INPUT, INPUT);
  pinMode(RC_CH5_INPUT, INPUT);
  pinMode(RC_CH6_INPUT, INPUT);

  pinMode(motor_in1, OUTPUT);
  pinMode(motor_in2, OUTPUT);

  enableInterrupt(RC_CH1_INPUT, calc_ch1, CHANGE);
  enableInterrupt(RC_CH2_INPUT, calc_ch2, CHANGE);
  enableInterrupt(RC_CH3_INPUT, calc_ch3, CHANGE);
  enableInterrupt(RC_CH4_INPUT, calc_ch4, CHANGE);
  enableInterrupt(RC_CH5_INPUT, calc_ch5, CHANGE);
  enableInterrupt(RC_CH6_INPUT, calc_ch6, CHANGE);

  //myDFPlayer.setTimeOut(500); //Set serial communictaion time out 500ms

  //----Set volume----
  myDFPlayer.volume(30);  //Set volume value (0~30)
  // myDFPlayer.volumeUp(); //Volume Up
  // myDFPlayer.volumeDown(); //Volume Down

  //----Set different EQ----
  // myDFPlayer.EQ(DFPLAYER_EQ_NORMAL);
  //  myDFPlayer.EQ(DFPLAYER_EQ_POP);
  //  myDFPlayer.EQ(DFPLAYER_EQ_ROCK);
  //  myDFPlayer.EQ(DFPLAYER_EQ_JAZZ);
  //  myDFPlayer.EQ(DFPLAYER_EQ_CLASSIC);
  myDFPlayer.EQ(DFPLAYER_EQ_NORMAL);

  //----Set device we use SD as default----
  //  myDFPlayer.outputDevice(DFPLAYER_DEVICE_U_DISK);
  myDFPlayer.outputDevice(DFPLAYER_DEVICE_SD);
  //  myDFPlayer.outputDevice(DFPLAYER_DEVICE_AUX);
  //  myDFPlayer.outputDevice(DFPLAYER_DEVICE_SLEEP);
  //  myDFPlayer.outputDevice(DFPLAYER_DEVICE_FLASH);

  //----Mp3 control----
  //  myDFPlayer.sleep();     //sleep
  //  myDFPlayer.reset();     //Reset the module
  //  myDFPlayer.enableDAC();  //Enable On-chip DAC
  //  myDFPlayer.disableDAC();  //Disable On-chip DAC
  //  myDFPlayer.outputSetting(true, 15); //output setting, enable the output and set the gain to 15
}

void loop() {
  rc_read_values();
  // printSerialChannels();
  
  // 1000 right - 1500 netral - 2000 left.

  //CHANNEL [2] Stops & Rear Light
  // 1000 forward - 1500 netral - 2000 backward.

  // Forward
  if (rc_values[RC_CH2] <= 1450) {
    delay(50);
    rearLight_off();
    stopLights_off();
    stopLightsFlag = 1;
    stopLightsTime = 1;
  }
  // Break
  if (rc_values[RC_CH2] >= 1650 && stopLightsFlag == 1) {
    delay(50);
    rearLight_off();
    stopLights_on();
    stopLightsFlag = 2;
  }

  // Netral
  if (rc_values[RC_CH2] >= 1455 && rc_values[RC_CH2] <= 1545) {
    if (rc_values[RC_CH6] < 1400) {
      stopLights_off();
    }
    else {
      rearLight_off();
      unsigned long stopLightsCurrentMillis = millis();
      if (stopLightsCurrentMillis - stopLightsPreviousMillis >= stopLightsInterval && stopLightsTime == 1) {
        stopLightsPreviousMillis = stopLightsCurrentMillis;
        stopLightsTime = 0;
      }
      if (stopLightsTime == 1) {
        stopLights_on();
      }
      if (stopLightsTime == 0) {
        stopLights_off();
      }
    }
    if (stopLightsFlag == 2) {
      stopLightsFlag = 3;
    }
  }
  // Backward
  if (rc_values[RC_CH2] >= 1550 && stopLightsFlag == 3) {
    delay(50);
    stopLights_off();
    rearLight_on();
    stopLightsTime = 1;
  }
  //CHANNEL [3] Lights
  if (rc_values[RC_CH3] > 1450 && rc_values[RC_CH3] < 1550) {
    lights_off();
    sideLights_off();
    turnFlag = 0;
  }
  else if (rc_values[RC_CH3] > 1000 && rc_values[RC_CH3] < 1400) {
    sideLights_on();
    turnFlag = 0;
  }
  if (rc_values[RC_CH3] > 1000 && rc_values[RC_CH3] < 1200) {
    sideLights_on();
    //turnLights_all_on();
    if (rc_values[RC_CH5] >= 1470 && rc_values[RC_CH5] <= 1530) {
      turnLightsAll();
      turnFlag = 1;
    }
  }
  if (rc_values[RC_CH3] < 1000) {
    sideLights_off();
    lights_off();
    spotLight_off();
    if (rc_values[RC_CH5] >= 1470 && rc_values[RC_CH5] <= 1530) {
      turnLightsAll();
      turnFlag = 1;
    }
  }

  if (rc_values[RC_CH3] > 1700) {
    lights_on();
    sideLights_on();
    turnFlag = 0;

    if (rc_values[RC_CH3] > 1900) {
      spotLight_on();
      
      turnFlag = 0;
      winchFlag = 1;
    }
    else {
      spotLight_off();
    }
  }
  //CHANNEL [4] Gear
  if (rc_values[RC_CH4] >= 1900) {
    //myDFPlayer.volume(20);
    //myDFPlayer.playMp3Folder(2);
    //delay(120);
  }
  //CHANNEL [5] Winch
  /////////////////////////////////
  if (rc_values[RC_CH5] >= 1800 && mode == 3) {
    turnLights_on(turnLights_r);
    turnLights_off(turnLights_l);
  }
  else if (rc_values[RC_CH5] <= 1000 && mode == 3) {
    turnLights_on(turnLights_l);
    turnLights_off(turnLights_r);
  }
  else if (turnFlag == 0 && mode == 3) {
    turnLights_all_off();
  }
  /////////////////////////////////
  //-------------WINCH-ON-----------//
      if (rc_values[RC_CH5] >= 1470 && rc_values[RC_CH5] <= 1530 && mode == 1) {
    digitalWrite(motor_in1, LOW);
    digitalWrite(motor_in2, LOW);
  }
  //-------------WINCH-ON-----------//
  if ((rc_values[RC_CH5] >= 1600 && rc_values[RC_CH2] >= 1470) && (rc_values[RC_CH2] <= 1530 && winchFlag == 1 && mode == 1)) {
    digitalWrite(motor_in1, HIGH);
    stopLights_off();
  }
  if (rc_values[RC_CH5] >= 1600 && winchFlag == 1 && mode == 1) {
    digitalWrite(motor_in1, HIGH);
  }
  if (rc_values[RC_CH5] <= 1400 && winchFlag == 1 && mode == 1) {
    digitalWrite(motor_in2, HIGH);
  }
  ////////////////////////////////////////
  //-------------SOUND-ON-----------//
  if (rc_values[RC_CH5] >= 1900 && mode == 2) {
     static unsigned long timer = millis();
     if (millis() - timer > 8000) {
        timer = millis();
        myDFPlayer.playMp3Folder(4); // idle
      }
    //lights_off();
  }
  if (rc_values[RC_CH5] >= 1450 && rc_values[RC_CH5] <= 1550 && mode == 2) {
    //myDFPlayer.playMp3Folder(2);
    myDFPlayer.pause();  //pause the mp3
    //lights_on();
  }
  //-------------HORN-ON-----------//
  if (rc_values[RC_CH5] <= 1000 && mode == 2) {
    myDFPlayer.playMp3Folder(5);
    delay(380);
    //lights_on();
  }
  ////////////////////////////////////////
 
  //CHANNEL [6] Mode
  if (rc_values[RC_CH6] >= 1800) { // Winch
     mode = 1;
   }
  if (rc_values[RC_CH6] >= 1450 && rc_values[RC_CH6] <= 1550) { // Turn
     mode = 3; 
   }
  if (rc_values[RC_CH6] <= 1000) { // Horn
     mode = 2;
   } 
}

// ******************** FUNCTION *********************

void lights_on() {
  analogWrite (lights, 255);
}

void lights_off() {
  analogWrite (lights, 0);
}

void lights_hight_on() {
  analogWrite (lights, 255);
}

void stopLights_on() {
  digitalWrite (stopLights, HIGH);
}
void stopLights_off() {
  digitalWrite (stopLights, LOW);
}

void sideLights_on() {
  analogWrite (sideLights, 150);
}

void sideLights_off() {
  analogWrite (sideLights, 0);
}
void spotLight_on() {
  digitalWrite (spotLight, HIGH);
}
void spotLight_off() {
  digitalWrite (spotLight, LOW);
}

void rearLight_on() {
  digitalWrite (rearLight, HIGH);
}

void rearLight_off() {
  digitalWrite (rearLight, LOW);
}

void turnLights_on(int turnSide) {
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= interval) {
    // save the last time you blinked the LED
    previousMillis = currentMillis;

    if (ledState == LOW) {
      ledState = HIGH;
    } else {
      ledState = LOW;
    }
    digitalWrite(turnSide, ledState);
  }
}

void turnLightsAll() {
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= interval) {
    // save the last time you blinked the LED
    previousMillis = currentMillis;

    if (ledState == LOW) {
      ledState = HIGH;
    } else {
      ledState = LOW;
    }
    digitalWrite(turnLights_l, ledState);
    digitalWrite(turnLights_r, ledState);
  }
}

void turnLights_off(int turnSideOff) {
  digitalWrite(turnSideOff, LOW);
}

void turnLights_all_off() {
  digitalWrite(turnLights_r, LOW);
  digitalWrite(turnLights_l, LOW);
}

/* void klakson () {
  myDFPlayer.playMp3Folder(5);
  delay(300);
}*/

/* void printSerialChannels () {
  Serial.print("CH1:"); Serial.print(rc_values[RC_CH1]); Serial.print(" \t");
  Serial.print("CH2:"); Serial.print(rc_values[RC_CH2]); Serial.print(" \t");
  Serial.print("CH3:"); Serial.print(rc_values[RC_CH3]); Serial.print(" \t");
  Serial.print("CH4:"); Serial.print(rc_values[RC_CH4]); Serial.print(" \t");
  Serial.print("CH5:"); Serial.print(rc_values[RC_CH5]); Serial.print(" \t");
  Serial.print("CH6:"); Serial.println(rc_values[RC_CH6]);
  delay(300);
} */
// ******************** FUNCTION END*********************
