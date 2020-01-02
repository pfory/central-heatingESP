#include "PCF8574.h"
#include <Wire.h>

PCF8574 PCF_01(0x20);
PCF8574 PCF_02(0x21);

// Rotační enkodér KY-040

//PCF-01
int pinCLKValve1        = 0;
int pinDTValve1         = 1;
int pinCLKValve2        = 2;
int pinDTValve2         = 3;
int pinCLKValve3        = 4;
int pinDTValve3         = 5;
int pinCLKValve4        = 6;
int pinDTValve4         = 7;
//PCF-02        
int pinCLKValve5        = 0;
int pinDTValve5         = 1;
int pinPolarityRelay    = 2;
int pinrelayValve1      = 3;
int pinrelayValve2      = 4;
int pinrelayValve3      = 5;
int pinrelayValve4      = 6;
int pinrelayValve5      = 7;


int poziceEnkod         = 0;
int poziceEnkodOld      = 0;
int stavPredValve1      = 0;
int stavPredValve2      = 0;
int stavPredValve3      = 0;
int stavPredValve4      = 0;
int stavPredValve5      = 0;
int stavCLK             = 0;

#include <timer.h>
auto timer = timer_create_default(); // create a timer with default settings
Timer<> default_timer; // save as above

void setup() {
  Serial.begin(115200);
  Serial.println("Ventil");
  
  PCF_01.begin();
  PCF_02.begin();


  stavPredValve1 = PCF_01.read(pinCLKValve1);
  stavPredValve2 = PCF_01.read(pinCLKValve2);
  stavPredValve3 = PCF_01.read(pinCLKValve3);
  stavPredValve4 = PCF_01.read(pinCLKValve4);
  stavPredValve5 = PCF_02.read(pinCLKValve5);
  
  timer.every(800, checkStatus);
}

void loop() {
  timer.tick(); // tick the timer

  if (Serial.available() > 0) {
    int incomingByte = Serial.read();

    Serial.print("I received: ");
    Serial.println(incomingByte, DEC);
    if (incomingByte == 48) {
      //close
      Serial.print("Zavirani ventilu ");
      poziceEnkod = 0;
      stavPred = getStavPred(1);
    } else if (incomingByte == 49) {
      //open
      Serial.print("Otevirani ventilu ");
      poziceEnkod = 0;
      stavPred = getStavPred(1);
    } else {
    }
   }
    
    
  stavCLK = getStavCLK(1);
  if (stavCLK != stavPred) {
    if (PCF_01.read(pinDTValve1) != stavCLK) {
      poziceEnkod++;
    } else {
      poziceEnkod--;
    }
    Serial.print(".");
  }
  stavPred = stavCLK;
}

bool checkStatus(void *) {
  if (poziceEnkod == poziceEnkodOld) {
  } else {
    poziceEnkodOld = poziceEnkod;
  }
  return true;
}

int getStavCLK(int valve) {
  if (valve == 1) { return PCF_01.read(pinCLKValve1); }
  else if (valve == 2) { return PCF_01.read(pinCLKValve2); }
  else if (valve == 3) { return PCF_01.read(pinCLKValve3); }
  else if (valve == 4) { return PCF_01.read(pinCLKValve4); }
  else if (valve == 5) { return PCF_02.read(pinCLKValve5); }
  else { return 0; }
}

int getStavPred(int valve) {
  if (valve == 1) { return stavPredValve1; }
  else if (valve == 2) { return stavPredValve2; }
  else if (valve == 3) { return stavPredValve3; }
  else if (valve == 4) { return stavPredValve4; }
  else if (valve == 5) { return stavPredValve5; }
  else { return 0; }
}
