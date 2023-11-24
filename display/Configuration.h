#ifndef CONFIGURATION_H
#define CONFIGURATION_H

// #include <ArduinoJson.h> //https://github.com/bblanchon/ArduinoJson
#include <Wire.h>

//SW name & version
#define     VERSION                          "0.32"
#define     SW_NAME                          "DisplayHeat"

#define ota
#define verbose
//#define cas
#define timers
// #define serverHTTP

static const char* const      mqtt_server                    = "192.168.1.56";
static const uint16_t         mqtt_port                      = 1883;
static const char* const      mqtt_username                  = "datel";
static const char* const      mqtt_key                       = "hanka12";
static const char* const      mqtt_base                      = "/home/displayHeat";
static const char* const      mqtt_topic_restart             = "restart";
static const char* const      mqtt_topic_netinfo             = "netinfo";
static const char* const      mqtt_config_portal             = "config";
static const char* const      mqtt_config_portal_stop        = "disconfig";
static const char* const      mqtt_topic_temperature         = "/home/Corridor/esp06/tOUTKamna";
static const char* const      mqtt_topic_cerpadlo            = "/home/Corridor/esp06/sPumpKamna/status";


//All of the IO pins have interrupt/pwm/I2C/one-wire support except D0.
// #define ONE_WIRE_BUS                        D4 //Dallas
#define SDA                                       D2 //
#define SCL                                       D1 //
#define I2C_ADDR 0x60

//#define SEND_DELAY                           5000  //prodleva mezi poslanim dat v ms
#define SENDSTAT_DELAY                       60000  //poslani statistiky kazdou minutu
#define CONNECT_DELAY                        60000   //ms
#define SHOW_DISPLAY                         250   //refresh displeje

#include <fce.h>

#endif
