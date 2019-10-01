#ifndef CONFIGURATION_H
#define CONFIGURATION_H

//SW name & version
#define     VERSION                      "0.93"
#define     SW_NAME                      "Central heat"

<<<<<<< HEAD
#define timers
#define ota
#define verbose

=======
#define ota
#define time
#define verbose
#define flowSensor
#define serverHTTP
>>>>>>> 572125e94cd55d7a0b8368d4d213ffea87caaaf8

#define AUTOCONNECTNAME   HOSTNAMEOTA
#define AUTOCONNECTPWD    "password"

<<<<<<< HEAD
#define ota
#ifdef ota
#include <ArduinoOTA.h>
#define HOSTNAMEOTA   "Central"
#endif

=======
#ifdef ota
#define HOSTNAMEOTA   "central"
#endif

#ifdef verbose
  #define DEBUG_PRINT(x)                     Serial.print (x)
  #define DEBUG_PRINTDEC(x)                  Serial.print (x, DEC)
  #define DEBUG_PRINTLN(x)                   Serial.println (x)
  #define DEBUG_PRINTF(x, y)                 Serial.printf (x, y)
  #define PORTSPEED 115200             
  #define DEBUG_WRITE(x)                     Serial.write (x)
  #define DEBUG_PRINTHEX(x)                  Serial.print (x, HEX)
  #define SERIAL_BEGIN                       Serial.begin(PORTSPEED)
#else
  #define DEBUG_PRINT(x)
  #define DEBUG_PRINTDEC(x)
  #define DEBUG_PRINTLN(x)
  #define DEBUG_PRINTF(x, y)
  #define DEBUG_WRITE(x)
#endif 

#define PRINT_SPACE                          lcd.print(F(" "));


char                  mqtt_server[40]       = "192.168.1.56";
uint16_t              mqtt_port             = 1883;
char                  mqtt_username[40]     = "datel";
char                  mqtt_key[20]          = "hanka12";
char                  mqtt_base[60]         = "/home/Corridor/esp06";
char                  static_ip[16]         = "192.168.1.109";
char                  static_gw[16]         = "192.168.1.1";
char                  static_sn[16]         = "255.255.255.0";



>>>>>>> 572125e94cd55d7a0b8368d4d213ffea87caaaf8
/*
--------------------------------------------------------------------------------------------------------------------------

Version history:
0.9  - prechod na ESP8266
0.67 - 2.11.2017 - obsluha klavesnice, config
0.66 - 2.11.2017 - nastaveni hodin
0.64 - 11.9.2017 - obsluha displeje, build IDE 1.8.3
0.5             - i2c keyboard
0.4 - 23.2.2016 - add RTC, prenos teploty na satelit
0.3 - 16.1.2015

--------------------------------------------------------------------------------------------------------------------------
HW
<<<<<<< HEAD
ESP8266 Wemos D1
=======
ESP8266 - Wemos D1 Mini
>>>>>>> 572125e94cd55d7a0b8368d4d213ffea87caaaf8
I2C display
2 Relays module
DALLAS temperature sensor
keyboard

<<<<<<< HEAD
=======
LOLIN Wemos D1 pinout
>>>>>>> 572125e94cd55d7a0b8368d4d213ffea87caaaf8
------------------------------------------
TX                                           - Tx
RX                                           - Rx
D0                                           - PIR sensor
D1                                           - I2C display SCL 0x20, keypad 0x27
D2                                           - I2C display SDA 0x20, keypad 0x27
D3                                           - relay 1
D4                                           - LED
D5                                           - DALLAS temperature sensors
D6                                           - DALLAS temperature sensors
D7                                           - DALLAS temperature sensors
D8                                           - BUZZER
--------------------------------------------------------------------------------------------------------------------------

<<<<<<< HEAD
#include <ESP8266WiFi.h>          //https://github.com/esp8266/Arduino
#include <WiFiManager.h>          //https://github.com/tzapu/WiFiManager
#include <Ticker.h>
#include "Sender.h"
#include <Wire.h>
#include <OneWire.h>
#include <DallasTemperature.h>


#define verbose
#ifdef verbose
  #define DEBUG_PRINT(x)         Serial.print (x)
  #define DEBUG_PRINTDEC(x)      Serial.print (x, DEC)
  #define DEBUG_PRINTLN(x)       Serial.println (x)
  #define DEBUG_PRINTF(x, y)     Serial.printf (x, y)
  #define DEBUG_PRINTHEX(x)      Serial.print (x, HEX)
  #define PORTSPEED 115200
  #define SERIAL_BEGIN           Serial.begin(PORTSPEED);
#else
  #define DEBUG_PRINT(x)
  #define DEBUG_PRINTDEC(x)
  #define DEBUG_PRINTLN(x)
  #define DEBUG_PRINTF(x, y)
#endif 

char         mqtt_server[40]                = "192.168.1.56";
uint16_t     mqtt_port                      = 1883;
char         mqtt_username[40]              = "datel";
char         mqtt_key[20]                   = "hanka12";
char         mqtt_base[60]                  = "/home/Corridor/esp08"; //pak esp06
char         static_ip[16]                  = "192.168.1.139";
char         static_gw[16]                  = "192.168.1.1";
char         static_sn[16]                  = "255.255.255.0";
char         mqtt_topic_weather[25]         = "/home/Meteo/Temperature";

=======
>>>>>>> 572125e94cd55d7a0b8368d4d213ffea87caaaf8

*/

//keypad i2c address
#define I2CADDR                              0x20

//display
<<<<<<< HEAD
#define LCDADDRESS                          0x27
#define LCDROWS                             4
#define LCDCOLS                             20

#define TEMPERATURE_X                       4
#define TEMPERATURE_Y                       3
#define RELAY_STATUSX                       17
#define RELAY_STATUSY                       3


#define LED1PIN                             D8 //status LED
#define ONE_WIRE_BUS_IN                     D5
#define ONE_WIRE_BUS_OUT                    D0
#define ONE_WIRE_BUS_UT                     D6
#define RELAYPIN                            D3 //relay
#define BUZZERPIN                           D7
#define PIRPIN                              D4
//SDA                                       D2
//SCL                                       D1

#ifndef NUMBER_OF_DEVICES
#define NUMBER_OF_DEVICES                    12
#endif

#define RELAY_ON                             HIGH
#define RELAY_OFF                            LOW

#define READTEMP_DELAY                       1000

//#define IN                                  0
//#define OUT                                 1
//#define RELAY                               100
//#define RELAYTEMP                           101
=======
#define LCDADDRESS                           0x27
#define LCDROWS                              4
#define LCDCOLS                              20

#define PIRPIN                               D0
#define RELAY1PIN                            D3
#define LEDPIN                               D4
                          
//one wire bus
#define ONE_WIRE_BUS_OUT                     D5
#define ONE_WIRE_BUS_IN                      D6
#define ONE_WIRE_BUS_UT                      D7

#define BIZZERPIN                            D8

#ifndef NUMBER_OF_DEVICES
#define NUMBER_OF_DEVICES                    15
#endif
>>>>>>> 572125e94cd55d7a0b8368d4d213ffea87caaaf8

#define TEMPERATURE_PRECISION 12

#define POZ0X                                0
#define POZ0Y                                0
#define POZ1Y                                1
#define TIMEX                                0
#define TIMEY                                3

#define DISPLAY_MAIN                         0
#define DISPLAY_T_DIFF_ON                    2
#define DISPLAY_T_DIFF_OFF                   3
#define DISPLAY_MAX_IO_TEMP                  5

#define RELAY1X                              19
#define RELAY1Y                              0


#define DS_MEASSURE_INTERVAL                 750 //inteval between meassurements
#define DELAY_AFTER_ON                       120000 //1000*60*2; //po tento cas zustane rele sepnute bez ohledu na stav teplotnich cidel
  
#define SAFETY_ON                            86.0 //teplota, pri niz rele vzdy sepne
  
#define SEND_DELAY                           30000  //prodleva mezi poslanim dat v ms
//#define SHOW_INFO_DELAY                      5000  //
#define SENDSTAT_DELAY                       60000 //poslani statistiky kazdou minutu
#define MEAS_DELAY                           5000  //mereni teplot
#define CALC_DELAY                           1000  //

// maximum number of seconds between resets that
// counts as a double reset 
#define DRD_TIMEOUT 2.0

<<<<<<< HEAD
#define SEND_DELAY                           30000  //prodleva mezi poslanim dat v ms
#define SHOW_INFO_DELAY                      5000  //
#define SENDSTAT_DELAY                       60000 //poslani statistiky kazdou minutu
#define MEAS_DELAY                           5000  //mereni teplot

=======
// address to the block in the RTC user memory
// change it if it collides with another usage 
// of the address block
#define DRD_ADDRESS 0x00
>>>>>>> 572125e94cd55d7a0b8368d4d213ffea87caaaf8

#endif
