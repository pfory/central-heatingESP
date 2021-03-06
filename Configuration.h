#ifndef CONFIGURATION_H
#define CONFIGURATION_H

#include <ESP8266WiFi.h>          //https://github.com/esp8266/Arduino
#include <WiFiManager.h>          //https://github.com/tzapu/WiFiManager
#include "Sender.h"
#include <Wire.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include "FS.h"
#include <DoubleResetDetector.h>      //https://github.com/khoih-prog/ESP_DoubleResetDetector
#include <Keypad_I2C.h>
#include <Keypad.h>          // GDY120705
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <Ticker.h>
#include <timer.h>


//SW name & version
#define     VERSION                      "1.20"
#define     SW_NAME                      "Central heat"

#define timers
#define ota
#define verbose
#define time
#ifdef time
#include <TimeLib.h>
#include <Timezone.h>
#endif

#define AUTOCONNECTNAME   HOSTNAMEOTA
#define AUTOCONNECTPWD    "password"

#define ota
#ifdef ota
#include <ArduinoOTA.h>
#define HOSTNAMEOTA   SW_NAME VERSION
#endif

#define PIR

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


// Number of seconds after reset during which a
// subseqent reset will be considered a double reset.
#define DRD_TIMEOUT 2
// RTC Memory Address for the DoubleResetDetector to use
#define DRD_ADDRESS 0

#define CONFIG_PORTAL_TIMEOUT 60 //jak dlouho zustane v rezimu AP nez se cip resetuje
#define CONNECT_TIMEOUT 120 //jak dlouho se ceka na spojeni nez se aktivuje config portal

static const char* const      mqtt_server                    = "192.168.1.56";
static const uint16_t         mqtt_port                      = 1883;
static const char* const      mqtt_username                  = "datel";
static const char* const      mqtt_key                       = "hanka12";
static const char* const      mqtt_base                      = "/home/Corridor/esp06";
//static const char* const      static_ip                      = "192.168.1.159";
//static const char* const      static_gw                      = "192.168.1.1";
//static const char* const      static_sn                      = "255.255.255.0";
static const char* const      mqtt_topic_weather             = "/home/Meteo/Temperature";
static const char* const      mqtt_topic_setTempON           = "tempON/set";
static const char* const      mqtt_topic_setTempOFFDiff      = "tempOFFDiff/set";
static const char* const      mqtt_topic_setTempAlarm        = "tempAlarm/set";
static const char* const      mqtt_topic_relay_type_set      = "relayType/set";       //0 - MANUAL, 1 - AUTO
static const char* const      mqtt_topic_restart             = "restart";
static const char* const      mqtt_topic_netinfo             = "netinfo";
static const char* const      mqtt_topic_sendSO              = "sorder";
static const char* const      mqtt_topic_so0                 = "so0";
static const char* const      mqtt_topic_so1                 = "so1";
static const char* const      mqtt_topic_so2                 = "so2";
static const char* const      mqtt_topic_so3                 = "so3";
static const char* const      mqtt_topic_so4                 = "so4";
static const char* const      mqtt_topic_so5                 = "so5";
static const char* const      mqtt_topic_so6                 = "so6";
static const char* const      mqtt_topic_so7                 = "so7";
static const char* const      mqtt_topic_so8                 = "so8";
static const char* const      mqtt_topic_so9                 = "so9";
static const char* const      mqtt_topic_so10                = "so10";
static const char* const      mqtt_topic_so11                = "so11";
static const char* const      mqtt_topic_so12                = "so12";

uint32_t              connectDelay                = 30000; //30s
uint32_t              lastConnectAttempt          = 0;  

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
#define ONE_WIRE_BUS_IN                     D6 //MISO                       GPIO12
#define ONE_WIRE_BUS_OUT                    D5 //SCK                        GPIO14
#define ONE_WIRE_BUS_UT                     D7 //MOSI                       GPIO13
#define RELAYPIN                            D3 //relay 10k Pull-up          GPIO0
#define BUZZERPIN                           D8 //                           GPIO15
#ifdef PIR
#define PIRPIN                              D0 //10k Pull-down, SS          GPIO16
#endif
//SDA                                       D2 //                           GPIO4
//SCL                                       D1 //                           GPIO5
//BUILTIN_LED                               D4 //10k Pull-up, BUILTIN_LED   GPIO2

#ifndef NUMBER_OF_DEVICES
#define NUMBER_OF_DEVICES                   12
#endif

#define RELAY_ON                            LOW
#define RELAY_OFF                           HIGH

#define RELAY_TYPE_MANUAL_OFF               0
#define RELAY_TYPE_MANUAL_ON                1
#define RELAY_TYPE_AUTO                     2


#define TEMPERATURE_PRECISION 10

#define TEMPINX                             0
#define TEMPINY                             0
#define TEMPOUTX                            3
#define TEMPOUTY                            0
#define TEMPSETONX                         18
#define TEMPSETONY                          0
#define TEMPSETOFFX                        15
#define TEMPSETOFFY                         0
#define RELAY_STATUSX                       6
#define RELAY_STATUSY                       0
#define RUNMINTODAY_X                      15 
#define RUNMINTODAY_Y                       3
#define TEMPERATURE_X                       6
#define TEMPERATURE_Y                       3

#ifdef time
#define TIMEX                               0
#define TIMEY                               3
#endif

// #define DISPLAY_MAIN                         0
// #define DISPLAY_T_DIFF_ON                    2
// #define DISPLAY_T_DIFF_OFF                   3
// #define DISPLAY_MAX_IO_TEMP                  5
 
#define SAFETY_ON                           86.0 //teplota, pri niz rele vzdy sepne
                                     
#define SEND_DELAY                          30000  //prodleva mezi poslanim dat v ms
//#define SHOW_INFO_DELAY                     5000  //
#define SENDSTAT_DELAY                      60000 //poslani statistiky kazdou minutu
#define MEAS_DELAY                          2000  //mereni teplot
                                     
#define TEMP_ERR                            -127

#define CFGFILE                             "/config.txt"

#endif
