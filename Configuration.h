#ifndef CONFIGURATION_H
#define CONFIGURATION_H

//SW name & version
#define     VERSION                      "0.96  "
#define     SW_NAME                      "Central heat"

#define timers
#define ota
#define verbose


#define AUTOCONNECTNAME   HOSTNAMEOTA
#define AUTOCONNECTPWD    "password"

#define ota
#ifdef ota
#include <ArduinoOTA.h>
#define HOSTNAMEOTA   "Central"
#endif

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
ESP8266 Wemos D1
I2C display
2 Relays module
DALLAS temperature sensor
keyboard
*/

#include <ESP8266WiFi.h>          //https://github.com/esp8266/Arduino
#include <WiFiManager.h>          //https://github.com/tzapu/WiFiManager
#include <Ticker.h>
#include "Sender.h"
#include <Wire.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include "FS.h"

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
char         mqtt_topic_setTempON[29]       = "/home/Corridor/esp08/tempON";
char         mqtt_topic_setTempOFFDiff[34]  = "/home/Corridor/esp08/tempOFFDiff";
char         mqtt_topic_setTempAlarm[32]    = "/home/Corridor/esp08/tempAlarm";

#define MIN_UNIT                            "m"

//keypad i2c address
#define I2CADDR                             0x20

//display
#define LCDADDRESS                          0x27
#define LCDROWS                             4
#define LCDCOLS                             20

#define TEMPERATURE_X                       4
#define TEMPERATURE_Y                       3
#define RELAY_STATUSX                       17
#define RELAY_STATUSY                       3

//All of the IO pins have interrupt/pwm/I2C/one-wire support except D0.
#define STATUS_LED                          BUILTIN_LED //status LED
#define ONE_WIRE_BUS_IN                     D7 //MOSI                       GPIO13
#define ONE_WIRE_BUS_OUT                    D5 //SCK                        GPIO14
#define ONE_WIRE_BUS_UT                     D6 //MISO                       GPIO12
#define RELAYPIN                            D3 //relay 10k Pull-up          GPIO0
#define BUZZERPIN                           D8 //                           GPIO16
#define PIRPIN                              D0 //10k Pull-down, SS          GPIO15
//SDA                                       D2 //                           GPIO4
//SCL                                       D1 //                           GPIO5
//BUILTIN_LED                               D4 //10k Pull-up, BUILTIN_LED   GPIO2

#ifndef NUMBER_OF_DEVICES
#define NUMBER_OF_DEVICES                   12
#endif

#define RELAY_ON                            LOW
#define RELAY_OFF                           HIGH

char*  topicRelay                           = "/home/Corridor/esp08/manualRelay";
char*  topicRestart                         = "/home/Corridor/esp08/restart";

#define TEMPERATURE_PRECISION 12

#define TEMPINOUTX                          0
#define TEMPINOUTY                          0
#define TEMPSETX                            15
#define TEMPSETY                            0
#define RELAY_STATUSX                       6
#define RELAY_STATUSY                       0
#define RUNMINTODAY_X                      15 
#define RUNMINTODAY_Y                       3
#define TEMPERATURE_X                       6
#define TEMPERATURE_Y                       3

// #define DISPLAY_MAIN                         0
// #define DISPLAY_T_DIFF_ON                    2
// #define DISPLAY_T_DIFF_OFF                   3
// #define DISPLAY_MAX_IO_TEMP                  5
 
#define SAFETY_ON                           86.0 //teplota, pri niz rele vzdy sepne
                                     
#define SEND_DELAY                          30000  //prodleva mezi poslanim dat v ms
//#define SHOW_INFO_DELAY                     5000  //
#define SENDSTAT_DELAY                      60000 //poslani statistiky kazdou minutu
#define MEAS_DELAY                          15000  //mereni teplot
                                     
                                     
#define SEND_DELAY                          30000  //prodleva mezi poslanim dat v ms
#define SHOW_INFO_DELAY                     5000  //
#define SENDSTAT_DELAY                      60000 //poslani statistiky kazdou minutu
#define MEAS_DELAY                          5000  //mereni teplot
                                     
#define TEMP_ERR                            -127

#define CFGFILE                             "/config.txt"

#endif
