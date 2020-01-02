#include "PCF8574.h"
#include <Wire.h>

PCF8574 PCF_01(0x20);
PCF8574 PCF_02(0x21);

// Rotační enkodér KY-040

// proměnné pro nastavení propojovacích pinů
// int pinCLK = D5;
// int pinDT  = D6;
// int pinSW  = D7;
int pinCLK = 0;
int pinDT  = 1;


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
  // nastavení propojovacích pinů jako vstupních
  //pinMode(pinCLK, INPUT);
  //pinMode(pinDT, INPUT);
  
  // pinMode(D1, OUTPUT);
  // pinMode(D2, OUTPUT);
  // pinMode(D3, OUTPUT);
  // digitalWrite(D1, HIGH);
  // digitalWrite(D2, HIGH);
  // digitalWrite(D3, HIGH);
  
  PCF_01.begin();
  PCF_02.begin();

  // Serial.println(PCF_01.read(0));
  // Serial.println(PCF_01.read(1));
  // Serial.println(PCF_01.read(2));
  // Serial.println(PCF_01.read(3));
  // Serial.println(PCF_01.read(4));

  //stavPred = digitalRead(pinCLK);   
  stavPred = PCF_01.read(pinCLK);
  // Serial.println(PCF_01.read(pinCLK));
  // Serial.println(PCF_01.read(pinDT));
  
  timer.every(800, checkStatus);
}

void loop() {
  timer.tick(); // tick the timer

  // Serial.println(PCF_01.read(pinCLK));
  // Serial.println(PCF_01.read(pinDT));

  if (Serial.available() > 0) {
    // read the incoming byte:
    int incomingByte = Serial.read();

    // say what you got:
    Serial.print("I received: ");
    Serial.println(incomingByte, DEC);
    if (incomingByte == 48) {
      //close
      Serial.print("Zavirani ventilu ");
      // digitalWrite(D3, HIGH);
      // digitalWrite(D1, HIGH);
      // digitalWrite(D2, HIGH);
      poziceEnkod = 0;
    } else if (incomingByte == 49) {
      //open
      Serial.print("Otevirani ventilu ");
      // digitalWrite(D3, HIGH);
      // digitalWrite(D1, LOW);
      // digitalWrite(D2, LOW);
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
  //Serial.println(poziceEnkod);
  //Serial.println(poziceEnkodOld);
  if (poziceEnkod == poziceEnkodOld) {
    // digitalWrite(D3, LOW);
    // digitalWrite(D1, HIGH);
    // digitalWrite(D2, HIGH);
  } else {
    poziceEnkodOld = poziceEnkod;
    //Serial.println("Vypnuti rele");
  }
  return true;
}
