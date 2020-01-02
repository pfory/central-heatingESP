#include "PCF8574.h"
#include <Wire.h>

PCF8574 PCF_01(0x20);
PCF8574 PCF_02(0x21);

// Rotační enkodér KY-040

// proměnné pro nastavení propojovacích pinů
// int pinCLK = D5;
// int pinDT  = D6;
// int pinSW  = D7;
int pinCLK      = 0;
int pinDT       = 1;
int pinReverse  = 2;
int pinValve    = 3;

// proměnné pro uložení pozice a stavů pro určení směru
// a stavu tlačítka
int poziceEnkod = 0;
int poziceEnkodOld = 0;
int stavPred;
int stavCLK;

#include <timer.h>
auto timer = timer_create_default(); // create a timer with default settings
Timer<> default_timer; // save as above

void setup() {
  Serial.begin(115200);
  Serial.println("Ventil");

  PCF_01.begin();
  PCF_02.begin();


  //stavPred = digitalRead(pinCLK);   
  stavPred = PCF_01.read(pinCLK);
  
  // PCF_01.write(pinReverse,  LOW);
  // PCF_01.write(pinValve,    LOW);
  // delay(1000);  
  // PCF_01.write(pinReverse,  HIGH);
  // PCF_01.write(pinValve,    HIGH);
  // delay(1000);  
  timer.every(800, checkStatus);
}

void loop() {
  timer.tick(); // tick the timer

  if (Serial.available() > 0) {
    // read the incoming byte:
    int incomingByte = Serial.read();

    // say what you got:
    // Serial.print("I received: ");
    // Serial.println(incomingByte, DEC);
    if (incomingByte == 48) {
      //close
      Serial.println("Zavirani ventilu ");
      PCF_01.write(pinReverse,  HIGH);
      PCF_01.write(pinValve,    HIGH);
      poziceEnkod = 0;
    } else if (incomingByte == 49) {
      //open
      Serial.println("Otevirani ventilu ");
      PCF_01.write(pinReverse,  LOW);
      PCF_01.write(pinValve,    HIGH);
      poziceEnkod = 0;
    } else {
      // digitalWrite(D3, LOW);
    }
   }
    
    
  //stavCLK = digitalRead(pinCLK);
  stavCLK = PCF_01.read(pinCLK);
  if (stavCLK != stavPred) {
    //if (digitalRead(pinDT) != stavCLK) {
    if (PCF_01.read(pinDT) != stavCLK) {
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
      PCF_01.write(pinReverse,  HIGH);
      PCF_01.write(pinValve,    LOW);
  } else {
    poziceEnkodOld = poziceEnkod;
    //Serial.println("Vypnuti rele");
  }
  return true;
}
