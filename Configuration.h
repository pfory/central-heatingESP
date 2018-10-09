#ifndef CONFIGURATION_H
#define CONFIGURATION_H

//SW name & version
#define     VERSION                      "0.93"
#define     SW_NAME                      "Central heat"

/*
--------------------------------------------------------------------------------------------------------------------------

Version history:
0.67 - 2.11.2017 - obsluha klavesnice, config
0.66 - 2.11.2017 - nastaveni hodin
0.64 - 11.9.2017 - obsluha displeje, build IDE 1.8.3
0.5             - i2c keyboard
0.4 - 23.2.2016 - add RTC, prenos teploty na satelit
0.3 - 16.1.2015

--------------------------------------------------------------------------------------------------------------------------
HW
Pro Mini 328
I2C display
1 Relays module
DALLAS
keyboard

Pro Mini 328 Layout
------------------------------------------
0               - DALLAS
1               - Tx
2               - DALLAS
3               - Rx
4               - I2C display SDA 0x27, I2C Central heating unit 0x02, keypad 0x20, 
5               - I2C display SCL 0x27, I2C Central heating unit 0x02, keypad 0x20
12              - LED
13              - BUZZER
14
15              - DALLAS
16              - RELAY
--------------------------------------------------------------------------------------------------------------------------
*/

#define MQTTBASE "/home/Corridor/test/"

//pins for softwareserial
#define RX A2
#define TX A3

#define TEMP_ERR -127     

//display
#define LCDADDRESS                          0x27
#define LCDROWS                             4
#define LCDCOLS                             20

//wiring
#define ONE_WIRE_BUS_IN                     0
#define ONE_WIRE_BUS_OUT                    2
#define ONE_WIRE_BUS_UT                     15
#define RELAYPIN                            16
#define LEDPIN                              12
#define BUZZERPIN                           13

//#define IN                                  0
//#define OUT                                 1
//#define RELAY                               100
//#define RELAYTEMP                           101

#define TEMPERATURE_PRECISION 12

//keypad i2c address
#define I2CADDR                             0x20
#define PRINT_SPACE                         lcd.print(F(" "));


#endif
