#ifndef CONFIGURATION_H
#define CONFIGURATION_H

#include <OneWire.h>
#include <DallasTemperature.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <ArduinoJson.h>


//SW name & version
#define     VERSION                      "1.71"
#define     SW_NAME                      "Central heat"

#define timers
#define ota
#define verbose
#define cas
#define PIR

/*
--------------------------------------------------------------------------------------------------------------------------

--------------------------------------------------------------------------------------------------------------------------
HW
ESP8266 Wemos D1
I2C display
2 Relays module
DALLAS temperature sensor
*/

static const char* const      mqtt_server                    = "192.168.1.56";
static const uint16_t         mqtt_port                      = 1883;
static const char* const      mqtt_username                  = "datel";
static const char* const      mqtt_key                       = "hanka12";
static const char* const      mqtt_base                      = "/home/Corridor/esp06";
static const char* const      mqtt_topic_tvymenikKamna       = "/home/Corridor/esp07/tBojler2";
static const char* const      mqtt_topic_tvymenikSolar       = "/home/Corridor/esp07/tBojler1";
static const char* const      mqtt_topic_termohlavicePozice  = "/home/zigbee2mqtt/TermohlaviceLoznice/pozice";
static const char* const      mqtt_topic_weather             = "/home/Meteo/Temperature";
static const char* const      mqtt_topic_setTempON           = "tempON/set";
static const char* const      mqtt_topic_setTempOFFDiff      = "tempOFFDiff/set";
static const char* const      mqtt_topic_setTempAlarm        = "tempAlarm/set";
static const char* const      mqtt_topic_relay_type_set      = "relayType/set";       //0 - MANUAL, 1 - AUTO
static const char* const      mqtt_topic_restart             = "restart";
static const char* const      mqtt_topic_netinfo             = "netinfo";
static const char* const      mqtt_config_portal             = "config";
static const char* const      mqtt_config_portal_stop        = "disconfig";


#define MIN_UNIT                            "m"

#define I2CADDR                             0x20

//display
#define LCDADDRESS                          0x27
#define LCDROWS                             4
#define LCDCOLS                             20

//All of the IO pins have interrupt/pwm/I2C/one-wire support except D0.
#define ONE_WIRE_BUS_KAMNA                  D6 //MISO                       GPIO12
#define ONE_WIRE_BUS_ROOM                   D5 //SCK                        GPIO14
//#define ONE_WIRE_BUS_UT                   D7 //MOSI                       GPIO13
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

#define TEMPROOMX                           0
#define TEMPROOMY                           0
#define TEMPINX                             3
#define TEMPINY                             0
#define TEMPOUTX                            6
#define TEMPOUTY                            0
#define TEMPSETONX                         18
#define TEMPSETONY                          0
#define TEMPSETOFFX                        15
#define TEMPSETOFFY                         0
#define RELAY_STATUSX                      10
#define RELAY_STATUSY                       0
#define RUNMINTODAY_X                      15 
#define RUNMINTODAY_Y                       3
#define TEMPERATURE_X                       6
#define TEMPERATURE_Y                       3

#define TEMPVYMENIKKAMNA_X                  4
#define TEMPVYMENIKKAMNA_Y                  1
#define TEMPVYMENIKSOLAR_X                  14
#define TEMPVYMENIKSOLAR_Y                  1
#define POSTERMOHLAVICE_X                   0
#define POSTERMOHLAVICE_Y                   2

#ifdef cas
#define TIMEX                               0
#define TIMEY                               3
#endif

// #define DISPLAY_MAIN                         0
// #define DISPLAY_T_DIFF_ON                    2
// #define DISPLAY_T_DIFF_OFF                   3
// #define DISPLAY_MAX_IO_TEMP                  5
 
#define SAFETY_ON                           86.0 //teplota, pri niz rele vzdy sepne
                                     
#define SEND_DELAY                          15000  //prodleva mezi poslanim dat v ms
//#define SHOW_INFO_DELAY                     5000  //
#define SENDSTAT_DELAY                      60000 //poslani statistiky kazdou minutu
#define MEAS_DELAY                          2000  //mereni teplot
#define CONNECT_DELAY                        5000 //ms
                                     
#define TEMP_ERR                            -127

#define CFGFILE                             "/config.txt"

#include <fce.h>

#endif
