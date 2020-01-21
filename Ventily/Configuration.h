#ifndef CONFIGURATION_H
#define CONFIGURATION_H

//SW name & version
#define     VERSION                      "0.01"
#define     SW_NAME                      "Ventily"

#define timers
#define ota
#define verbose


#define AUTOCONNECTNAME   HOSTNAMEOTA
#define AUTOCONNECTPWD    "password"

#define ota
#ifdef ota
#include <ArduinoOTA.h>
#define HOSTNAMEOTA   "Ventily"
#endif


#include <ESP8266WiFi.h>          //https://github.com/esp8266/Arduino
#include <WiFiManager.h>          //https://github.com/tzapu/WiFiManager
#include <Ticker.h>
#include "Sender.h"
// #include <Wire.h>
// #include <OneWire.h>
// #include <DallasTemperature.h>
// #include "FS.h"

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

static const char* const      mqtt_server                    = "192.168.1.56";
static const uint16_t         mqtt_port                      = 1883;
static const char* const      mqtt_username                  = "datel";
static const char* const      mqtt_key                       = "hanka12";
static const char* const      mqtt_base                      = "/home/Corridor/esp08";
static const char* const      static_ip                      = "192.168.1.111";
static const char* const      static_gw                      = "192.168.1.1";
static const char* const      static_sn                      = "255.255.255.0";
static const char* const      mqtt_topic_weather             = "/home/Meteo/Temperature";
static const char* const      mqtt_topic_restart             = "restart";
static const char* const      mqtt_topic_valve1              = "valve1";


//All of the IO pins have interrupt/pwm/I2C/one-wire support except D0.
#define STATUS_LED                          BUILTIN_LED //status LED
//#define ONE_WIRE_BUS_IN                     D6 //MISO                       GPIO12
//#define ONE_WIRE_BUS_OUT                    D5 //SCK                        GPIO14
//#define ONE_WIRE_BUS_UT                     D7 //MOSI                       GPIO13
//#define RELAYPIN                            D3 //relay 10k Pull-up          GPIO0
//#define BUZZERPIN                           D8 //                           GPIO15
//#ifdef PIR
//#define PIRPIN                              D0 //10k Pull-down, SS          GPIO16
//#endif
//SDA                                       D2 //                           GPIO4
//SCL                                       D1 //                           GPIO5
//BUILTIN_LED                               D4 //10k Pull-up, BUILTIN_LED   GPIO2


#define RELAY_ON                            LOW
#define RELAY_OFF                           HIGH


#define SEND_DELAY                          30000  //prodleva mezi poslanim dat v ms
//#define SHOW_INFO_DELAY                     5000  //
#define SENDSTAT_DELAY                      60000 //poslani statistiky kazdou minutu

#endif
