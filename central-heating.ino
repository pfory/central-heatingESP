/*CENTRAL-HEATING
--------------------------------------------------------------------------------------------------------------------------
CENTRAL - control system for central heating
Petr Fory pfory@seznam.cz
GIT - https://github.com/pfory/central-heating
//Wemos D1 R2 & mini
*/

#include "Configuration.h"


#include <ESP8266WiFi.h>          //https://github.com/esp8266/Arduino
#include <WiFiManager.h>          //https://github.com/tzapu/WiFiManager
#include <FS.h>          //this needs to be first
#include <Ticker.h>
#include <ArduinoJson.h> //https://github.com/bblanchon/ArduinoJson
#include "DoubleResetDetector.h" // https://github.com/datacute/DoubleResetDetector
#include "Sender.h"
#include <Wire.h>
#include <PubSubClient.h>
#include "ESP8266FtpServer.h"

FtpServer ftpSrv;

#ifdef ota
#include <ArduinoOTA.h>
#endif

#ifdef serverHTTP
#include <ESP8266WebServer.h>
ESP8266WebServer server(80);
#endif

#ifdef time
#include <TimeLib.h>
#include <Timezone.h>
WiFiUDP EthernetUdp;
static const char     ntpServerName[]       = "tik.cesnet.cz";
//const int timeZone = 2;     // Central European Time
//Central European Time (Frankfurt, Paris)
TimeChangeRule        CEST                  = {"CEST", Last, Sun, Mar, 2, 120};     //Central European Summer Time
TimeChangeRule        CET                   = {"CET", Last, Sun, Oct, 3, 60};       //Central European Standard Time
Timezone CE(CEST, CET);
unsigned int          localPort             = 8888;  // local port to listen for UDP packets
time_t getNtpTime();
#endif


#define DRD_TIMEOUT       1
// RTC Memory Address for the DoubleResetDetector to use
#define DRD_ADDRESS       0
DoubleResetDetector drd(DRD_TIMEOUT, DRD_ADDRESS);

#define CFGFILE "/config.json"

unsigned long lastRunMin                    = 0;
    
uint32_t heartBeat                          = 0;
   
float tempIN                                = 0; //input medium temperature
float tempOUT                               = 0; //output medium temperature
float tempUT[NUMBER_OF_DEVICES];             
//unsigned long energyADay                    = 0; //energy a day in Ws
unsigned long secOnDay                      = 0; //sec in ON state per day
//unsigned int  power                         = 0; //actual power in W
//float energyDiff                            = 0.f; //difference in Ws
volatile bool showDoubleDot                 = false;
bool firstTempMeasDone                      = false;
unsigned long lastOffOn                     = 0; //zamezuje cyklickemu zapinani a vypinani rele
unsigned long lastOff                       = 0;  //ms posledniho vypnuti rele
bool dispClear                              = false;

//maximal temperatures
float tMaxIn                                = 0; //maximal input temperature (just for statistics)
float tMaxOut                               = 0; //maximal output temperature (just for statistics)

  
//HIGH - relay OFF, LOW - relay ON   
bool relay1                                 = HIGH; 
   
bool manualON                               = false;
bool shouldSaveConfig                       = false; //flag for saving data


ADC_MODE(ADC_VCC); //vcc read

#ifdef flowSensor
volatile int      numberOfPulsesFlow        = 0; // Measures flow sensor pulses
void flow () { // Interrupt function
   numberOfPulsesFlow++;
}
#endif    
    
unsigned int display                        = 0;
unsigned long showInfo                      = 0; //zobrazeni 4 radky na displeji


#include <LiquidCrystal_I2C.h>
LiquidCrystal_I2C lcd(LCDADDRESS,LCDCOLS,LCDROWS);  // set the LCD

#include <Keypad_I2C.h>
#include <Keypad.h>          // GDY120705
#include <Wire.h>

const byte ROWS = 4; //four rows
const byte COLS = 4; //three columns
char keys[ROWS][COLS]                       = {
                                            {'1','2','3','A'},
                                            {'4','5','6','B'},
                                            {'7','8','9','C'},
                                            {'*','0','#','D'}
};
byte rowPins[ROWS]                          = {7,6,5,4}; //connect to the row pinouts of the keypad
byte colPins[COLS]                          = {3,2,1,0}; //connect to the column pinouts of the keypad

//Keypad_I2C keypad = Keypad_I2C( makeKeymap(keys), rowPins, colPins, ROWS, COLS, I2CADDR );
Keypad_I2C keypad( makeKeymap(keys), rowPins, colPins, ROWS, COLS, I2CADDR); 
// #endif

// enum modeDisplay                          {SETUP, INFO};
// modeDisplay displayMode                    = INFO;


//promenne ulozene v pameti (viz CFGFILE "/config.json")
byte            tempON                    = 70; //rozdil vystupni teploty panelu 1 tP1Out nebo panelu 2 tP2Out proti teplote bojleru nebo mistnosti tControl pri kterem dojde ke spusteni cerpadla
byte            tempOFF                   = 60; //rozdil vystupni teploty panelu 2 tP2Out proti teplote bojleru nebo mistnosti tControl pri kterem dojde k vypnuti cerpadla

byte            sensorOrder[NUMBER_OF_DEVICES];

//DALLAS temperature sensors
#include <OneWire.h>
OneWire onewireOUT(ONE_WIRE_BUS_OUT);  // pin for onewire DALLAS bus for temperature OUT sensor
OneWire onewireIN(ONE_WIRE_BUS_IN);   // pin for onewire DALLAS bus for temperature IN sensor
OneWire onewireUT(ONE_WIRE_BUS_UT);   // pin for onewire DALLAS bus for temperature sensors
#include <DallasTemperature.h>
DallasTemperature dsSensorsOUT(&onewireOUT);
DallasTemperature dsSensorsIN(&onewireIN);
DallasTemperature dsSensorsUT(&onewireUT);
DeviceAddress tempDeviceAddress;

DeviceAddress tempDeviceAddresses[NUMBER_OF_DEVICES];
unsigned int numberOfDevices                = 0; // Number of temperature devices found

float sensor[NUMBER_OF_DEVICES];

bool isDebugEnabled()
{
#ifdef verbose
  return true;
#endif // verbose
  return false;
}

//for LED status
#include <Ticker.h>
Ticker ticker;


#include <timer.h>
auto timer = timer_create_default(); // create a timer with default settings
Timer<> default_timer; // save as above


//MQTT callback
void callback(char* topic, byte* payload, unsigned int length) {
  char * pEnd;
  long int valL;
  String val =  String();
  DEBUG_PRINT("Message arrived [");
  DEBUG_PRINT(topic);
  DEBUG_PRINT("] ");
  for (int i=0;i<length;i++) {
    DEBUG_PRINT((char)payload[i]);
    val += (char)payload[i];
  }
  DEBUG_PRINTLN();
/*  if (strcmp(topic, "/home/Corridor/esp07/controlSensorBojler")==0) {
    DEBUG_PRINT("set control sensor to ");
    if (val.toInt()==1) {
      DEBUG_PRINTLN(F("Bojler"));
    } else {
      DEBUG_PRINTLN(F("Room"));
    }
    controlSensorBojler=val.toInt();
    saveConfig();
  } else if (strcmp(topic, "/home/Corridor/esp07/tempOFF")==0) {
    DEBUG_PRINT("set tempOFF to ");
    tempOFF=val.toInt();
    DEBUG_PRINT(tempOFF);
    saveConfig();
  } else if (strcmp(topic, "/home/Corridor/esp07/tempON")==0) {
    DEBUG_PRINT("set tempON to ");
    tempON=val.toInt();
    DEBUG_PRINT(tempON);
    saveConfig();
  } else if (strcmp(topic, "/home/Corridor/esp07/so0")==0) {
    DEBUG_PRINT("set sensor order 0 to ");
    sensorOrder[0]=val.toInt();
    DEBUG_PRINT(val.toInt());
    saveConfig();
  } else if (strcmp(topic, "/home/Corridor/esp07/so1")==0) {
    DEBUG_PRINT("set sensor order 1 to ");
    sensorOrder[1]=val.toInt();
    DEBUG_PRINT(val.toInt());
    saveConfig();
  } else if (strcmp(topic, "/home/Corridor/esp07/so2")==0) {
    DEBUG_PRINT("set sensor order 2 to ");
    sensorOrder[2]=val.toInt();
    DEBUG_PRINT(val.toInt());
    saveConfig();
  } else if (strcmp(topic, "/home/Corridor/esp07/so3")==0) {
    DEBUG_PRINT("set sensor order 3 to ");
    sensorOrder[3]=val.toInt();
    DEBUG_PRINT(val.toInt());
    saveConfig();
  } else if (strcmp(topic, "/home/Corridor/esp07/so4")==0) {
    DEBUG_PRINT("set sensor order 4 to ");
    sensorOrder[4]=val.toInt();
    DEBUG_PRINT(val.toInt());
    saveConfig();
  } else if (strcmp(topic, "/home/Corridor/esp07/so5")==0) {
    DEBUG_PRINT("set sensor order 5 to ");
    sensorOrder[5]=val.toInt();
    DEBUG_PRINT(val.toInt());
    saveConfig();
  } else if (strcmp(topic, "/home/Corridor/esp07/so6")==0) {
    DEBUG_PRINT("set sensor order 6 to ");
    sensorOrder[6]=val.toInt();
    DEBUG_PRINT(val.toInt());
    saveConfig();
  } else if (strcmp(topic, "/home/Corridor/esp07/so7")==0) {
    DEBUG_PRINT("set sensor order 7 to ");
    sensorOrder[7]=val.toInt();
    DEBUG_PRINT(val.toInt());
    saveConfig();
  } else if (strcmp(topic, "/home/Corridor/esp07/so8")==0) {
    DEBUG_PRINT("set sensor order 8 to ");
    sensorOrder[8]=val.toInt();
    DEBUG_PRINT(val.toInt());
    saveConfig();
  } else if (strcmp(topic, "/home/Corridor/esp07/so9")==0) {
    DEBUG_PRINT("set sensor order 9 to ");
    sensorOrder[9]=val.toInt();
    DEBUG_PRINT(val.toInt());
    saveConfig();*/
  if (strcmp(topic, "/home/Corridor/esp06/restart")==1) {
    DEBUG_PRINT("RESTART");
    saveConfig();
    ESP.restart();
  }
}

WiFiClient espClient;
PubSubClient client(espClient);


//----------------------------------------------------- S E T U P -----------------------------------------------------------
void setup() {
  // put your setup code here, to run once:
  SERIAL_BEGIN;
  DEBUG_PRINT(F(SW_NAME));
  DEBUG_PRINT(F(" "));
  DEBUG_PRINTLN(F(VERSION));

  lcd.init();               // initialize the lcd 
  lcd.backlight();
  //lcd.begin();               // initialize the lcd 
  lcd.home();                   
  lcd.print(SW_NAME);  
  PRINT_SPACE
  lcd.print(VERSION);
  
  pinMode(BUILTIN_LED, OUTPUT);
  pinMode(LEDPIN, OUTPUT);

  pinMode(RELAY1PIN, OUTPUT);

  digitalWrite(RELAY1PIN, relay1);

  ticker.attach(1, tick);
  bool _dblreset = drd.detectDoubleReset();
    
  WiFi.printDiag(Serial);
    
  bool validConf = readConfig();
  if (!validConf) {
    DEBUG_PRINTLN(F("ERROR config corrupted"));
  }
  
  rst_info *_reset_info = ESP.getResetInfoPtr();
  uint8_t _reset_reason = _reset_info->reason;
  DEBUG_PRINT("Boot-Mode: ");
  DEBUG_PRINTLN(_reset_reason);
  heartBeat = _reset_reason;

  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);
  
  // drd.stop();

  // if (_dblreset) {
    // WiFi.disconnect(); //  this alone is not enough to stop the autoconnecter
    // WiFi.mode(WIFI_AP);
  // }
  
  //WiFiManager
  //Local intialization. Once its business is done, there is no need to keep it around
  WiFiManager wifiManager;
  //reset settings - for testing
  //wifiManager.resetSettings();
  
  IPAddress _ip,_gw,_sn;
  _ip.fromString(static_ip);
  _gw.fromString(static_gw);
  _sn.fromString(static_sn);

  wifiManager.setSTAStaticIPConfig(_ip, _gw, _sn);
  
  DEBUG_PRINTLN(_ip);
  DEBUG_PRINTLN(_gw);
  DEBUG_PRINTLN(_sn);

  //wifiManager.setConfigPortalTimeout(60); 
  //set callback that gets called when connecting to previous WiFi fails, and enters Access Point mode
  wifiManager.setAPCallback(configModeCallback);
  
    //DEBUG_PRINTLN("Double reset detected. Config mode.");

  WiFiManagerParameter custom_mqtt_server("mqtt_server", "MQTT server", mqtt_server, 40);
  WiFiManagerParameter custom_mqtt_port("mqtt_port", "MQTT server port", String(mqtt_port).c_str(), 5);
  WiFiManagerParameter custom_mqtt_uname("mqtt_uname", "MQTT username", mqtt_username, 40);
  WiFiManagerParameter custom_mqtt_key("mqtt_key", "MQTT password", mqtt_key, 20);
  WiFiManagerParameter custom_mqtt_base("mqtt_base", "MQTT topic end without /", mqtt_base, 60);
  WiFiManagerParameter custom_tempON("tempON", "temperature ON", String(tempON).c_str(), 2);
  WiFiManagerParameter custom_tempOFF("tempOFF", "temperature OFF", String(tempOFF).c_str(), 2);

  wifiManager.addParameter(&custom_mqtt_server);
  wifiManager.addParameter(&custom_mqtt_port);
  wifiManager.addParameter(&custom_mqtt_uname);
  wifiManager.addParameter(&custom_mqtt_key);
  wifiManager.addParameter(&custom_mqtt_base);
  wifiManager.addParameter(&custom_tempON);
  wifiManager.addParameter(&custom_tempOFF);

  //sets timeout until configuration portal gets turned off
  //useful to make it all retry or go to sleep
  //in seconds
  
  wifiManager.setTimeout(30);
  wifiManager.setConnectTimeout(30); 
  //wifiManager.setBreakAfterConfig(true);
  
  //set config save notify callback
  wifiManager.setSaveConfigCallback(saveConfigCallback);
  
  //fetches ssid and pass and tries to connect
  //if it does not connect it starts an access point with the specified name
  //here  "AutoConnectAP"
  //and goes into a blocking loop awaiting configuration
  if (!wifiManager.autoConnect(AUTOCONNECTNAME, AUTOCONNECTPWD)) { 
    DEBUG_PRINTLN("failed to connect and hit timeout");
    delay(3000);
    //reset and try again, or maybe put it to deep sleep
    ESP.reset();
    delay(5000);
  } 
  
  validateInput(custom_mqtt_server.getValue(), mqtt_server);
  mqtt_port = String(custom_mqtt_port.getValue()).toInt();
  validateInput(custom_mqtt_uname.getValue(), mqtt_username);
  validateInput(custom_mqtt_key.getValue(), mqtt_key);
  validateInput(custom_mqtt_base.getValue(), mqtt_base);
  tempON = String(custom_tempON.getValue()).toInt();
  tempOFF = String(custom_tempOFF.getValue()).toInt();
  
  if (shouldSaveConfig) {
    saveConfig();
  }
  
  //if you get here you have connected to the WiFi
  DEBUG_PRINTLN("CONNECTED");
  DEBUG_PRINT("Local ip : ");
  DEBUG_PRINTLN(WiFi.localIP());
  DEBUG_PRINTLN(WiFi.subnetMask());

#ifdef serverHTTP
  server.on ( "/", handleRoot );
  server.begin();
  DEBUG_PRINTLN ( "HTTP server started!!" );
#endif

#ifdef time
  DEBUG_PRINTLN("Setup TIME");
  EthernetUdp.begin(localPort);
  DEBUG_PRINT("Local port: ");
  DEBUG_PRINTLN(EthernetUdp.localPort());
  DEBUG_PRINTLN("waiting for sync");
  setSyncProvider(getNtpTime);
  setSyncInterval(300);
  
  printSystemTime();
#endif

#ifdef ota
  //OTA
  // Port defaults to 8266
  // ArduinoOTA.setPort(8266);

  // Hostname defaults to esp8266-[ChipID]
  ArduinoOTA.setHostname(HOSTNAMEOTA);

  // No authentication by default
  // ArduinoOTA.setPassword("admin");

  // Password can be set with it's md5 value as well
  // MD5(admin) = 21232f297a57a5a743894a0e4a801fc3
  // ArduinoOTA.setPasswordHash("21232f297a57a5a743894a0e4a801fc3");

  ArduinoOTA.onStart([]() {
    // String type;
    // if (ArduinoOTA.getCommand() == U_FLASH)
      // type = "sketch";
    // else // U_SPIFFS
      // type = "filesystem";

    // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
    //DEBUG_PRINTLN("Start updating " + type);
    DEBUG_PRINTLN("Start updating ");
  });
  ArduinoOTA.onEnd([]() {
   DEBUG_PRINTLN("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    DEBUG_PRINTF("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    DEBUG_PRINTF("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) DEBUG_PRINTLN("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) DEBUG_PRINTLN("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) DEBUG_PRINTLN("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) DEBUG_PRINTLN("Receive Failed");
    else if (error == OTA_END_ERROR) DEBUG_PRINTLN("End Failed");
  });
  ArduinoOTA.begin();
#endif


  DEBUG_PRINT(F("tempON:"));  
  DEBUG_PRINTLN(tempON);
  DEBUG_PRINT(F("tempOFF:"));  
  DEBUG_PRINTLN(tempOFF);

  lcd.setCursor(0,1);
  lcd.print(F("tempON:"));  
  lcd.print(tempON);
  lcd.print(F(" tempOFF:"));  
  lcd.print(tempOFF);
  lcd.setCursor(0,2);

  keypad.begin();
 
  dsInit();

  lcd.clear();

  if (numberOfDevices>NUMBER_OF_DEVICES) {
    DEBUG_PRINTLN("ERROR - real number of devices DS18B20 > NUMBER_OF_DEVICES. Change variable NUMBER_OF_DEVICES in configuration file!!!!!!!!");
  }

  if (SPIFFS.begin()) {
   DEBUG_PRINTLN("SPIFFS opened!");
   ftpSrv.begin("esp8266", "esp8266"); // username, password for ftp. Set ports in ESP8266FtpServer.h (default 21, 50009 for PASV)
  }
  
  //setup timers
  if (numberOfDevices>0) {
    timer.every(SEND_DELAY, sendDataHA);
    timer.every(MEAS_DELAY, tempMeas);
    //timer.every(CALC_DELAY, calcPowerAndEnergy);
  }
  
  timer.every(SENDSTAT_DELAY, sendStatisticHA);
  //timer.every(SENDSTAT_DELAY, countMinRun);
  timer.every(CALC_DELAY/2, displayTime);

  DEBUG_PRINTLN(" Ready");
 
  ticker.detach();
  //keep LED on
  digitalWrite(BUILTIN_LED, HIGH);
  digitalWrite(LEDPIN, HIGH);
} //setup


//----------------------------------------------------- L O O P -----------------------------------------------------------
void loop() {
  firstTempMeasDone ? mainControl() : void();
  
  if (digitalRead(PIRPIN)==1) {
    lcd.backlight();
  } else {
    lcd.noBacklight();
  }

  lcdShow();
  
  keyBoard();
  
  timer.tick(); // tick the timer
#ifdef serverHTTP
  server.handleClient();
#endif

#ifdef ota
  ArduinoOTA.handle();
#endif

  if (!client.connected()) {
    reconnect();
  }
  client.loop();
  
  //handle ftp server
  ftpSrv.handleFTP();

  if (minute()==0 && second()==0) {
    if (!dispClear) { 
      lcd.clear();
      dispClear = true;
    }
  } else {
    dispClear = false;
  }
  
  if (hour()==0 && minute()==0) { //first ON in actual day
    tMaxIn=T_MIN;
    tMaxOut=T_MIN;
  }

} //loop


//----------------------------------------------------- F U N C T I O N S -----------------------------------------------------------
// bool calcPowerAndEnergy(void *) {
  // float t1;
  // float t2;
  
  // if (relay1==LOW) {  //pump is ON
    // if (controlSensorBojler==1) {
      // t1 = tBojlerIn;
      // t2 = tBojlerOut;
    // } else {
      // t1 = tP2Out;
      // t2 = tP1In;
    // }
    // if (t1>t2) {
      // secOnDay++;
      // power = getPower(t1, t2); //in W
      // //DEBUG_PRINTLN(F(power);
      // energyDiff += CALC_DELAY*(float)power/1000.f; //in Ws
      // if (energyDiff >= 3600.f) { //Wh
        // energyADay  += (unsigned long)energyDiff;
        // energyDiff = energyDiff - (long)energyDiff;
      // }
    // } else {
      // power=0;
    // }
  // } else {
    // power=0;
  // }
// }


void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    DEBUG_PRINT("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect(mqtt_base, mqtt_username, mqtt_key)) {
      DEBUG_PRINTLN("connected");
      // Once connected, publish an announcement...
      //client.publish("outTopic","hello world");
      // ... and resubscribe
      //client.subscribe(mqtt_base + '/' + 'inTopic');
      client.subscribe((String(mqtt_base) + "/" + "tempON").c_str());
      client.subscribe((String(mqtt_base) + "/" + "tempOFF").c_str());
/*      client.subscribe((String(mqtt_base) + "/" + "so0").c_str());
      client.subscribe((String(mqtt_base) + "/" + "so1").c_str());
      client.subscribe((String(mqtt_base) + "/" + "so2").c_str());
      client.subscribe((String(mqtt_base) + "/" + "so3").c_str());
      client.subscribe((String(mqtt_base) + "/" + "so4").c_str());
      client.subscribe((String(mqtt_base) + "/" + "so5").c_str());
      client.subscribe((String(mqtt_base) + "/" + "so6").c_str());
      client.subscribe((String(mqtt_base) + "/" + "so7").c_str());
      client.subscribe((String(mqtt_base) + "/" + "so8").c_str());
      client.subscribe((String(mqtt_base) + "/" + "so9").c_str());*/
      client.subscribe((String(mqtt_base) + "/" + "restart").c_str());
    } else {
      DEBUG_PRINT("failed, rc=");
      DEBUG_PRINT(client.state());
      DEBUG_PRINTLN(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}


#ifdef serverHTTP
void handleRoot() {
	char temp[600];
  printSystemTime();
  DEBUG_PRINTLN(" Client request");
  digitalWrite(BUILTIN_LED, LOW);
  
	snprintf ( temp, 400,
      "<html>\
        <head>\
          <meta charset='UTF-8'>\
        </head>\
        <body>\
          sensor[0],%s%d.%02d<br />\
          sensor[1],%s%d.%02d<br />\
          sensor[2],%s%d.%02d<br />\
          sensor[3],%s%d.%02d<br />\
          sensor[4],%s%d.%02d<br />\
          sensor[5],%s%d.%02d<br />\
          sensor[6],%s%d.%02d<br />\
          sensor[7],%s%d.%02d<br />\
        </body>\
      </html>",
      sensor[0]<0 && sensor[0]>-1 ? "-":"",
      (int)sensor[0], 
      abs((sensor[0] - (int)sensor[0]) * 100),
      sensor[1]<0 && sensor[1]>-1 ? "-":"",
      (int)sensor[1], 
      abs((sensor[1] - (int)sensor[1]) * 100),
      sensor[2]<0 && sensor[2]>-1 ? "-":"",
      (int)sensor[2], 
      abs((sensor[2] - (int)sensor[2]) * 100),
      sensor[3]<0 && sensor[3]>-1 ? "-":"",
      (int)sensor[3], 
      abs((sensor[3] - (int)sensor[3]) * 100),
      sensor[4]<0 && sensor[4]>-1 ? "-":"",
      (int)sensor[4], 
      abs((sensor[4] - (int)sensor[4]) * 100),
      sensor[5]<0 && sensor[5]>-1 ? "-":"",
      (int)sensor[5], 
      abs((sensor[5] - (int)sensor[5]) * 100),
      sensor[6]<0 && sensor[6]>-1 ? "-":"",
      (int)sensor[6], 
      abs((sensor[6] - (int)sensor[6]) * 100),
      sensor[7]<0 && sensor[7]>-1 ? "-":"",
      (int)sensor[7], 
      abs((sensor[7] - (int)sensor[7]) * 100)
	);
	server.send ( 200, "text/html", temp );
  digitalWrite(BUILTIN_LED, HIGH);
}
#endif

void tick()
{
  //toggle state
  int state = digitalRead(BUILTIN_LED);  // get the current state of GPIO1 pin
  digitalWrite(BUILTIN_LED, !state);     // set pin to the opposite state
  digitalWrite(LEDPIN, !state);          // set pin to the opposite state
}
  
//callback notifying us of the need to save config
void saveConfigCallback () {
  DEBUG_PRINTLN("Should save config");
  shouldSaveConfig = true;
}

//gets called when WiFiManager enters configuration mode
void configModeCallback (WiFiManager *myWiFiManager) {
  DEBUG_PRINTLN("Entered config mode");
  DEBUG_PRINTLN(WiFi.softAPIP());
  //if you used auto generated SSID, print it
  DEBUG_PRINTLN(myWiFiManager->getConfigPortalSSID());
  //entered config mode, make led toggle faster
  ticker.attach(0.2, tick);
}


void validateInput(const char *input, char *output)
{
  String tmp = input;
  tmp.trim();
  tmp.replace(' ', '_');
  tmp.toCharArray(output, tmp.length() + 1);
}

bool saveConfig() {
  DEBUG_PRINTLN(F("Saving config..."));

  // if SPIFFS is not usable
  if (!SPIFFS.begin() || !SPIFFS.exists(CFGFILE) ||
      !SPIFFS.open(CFGFILE, "w"))
  {
    DEBUG_PRINTLN(F("Need to format SPIFFS: "));
    SPIFFS.end();
    SPIFFS.begin();
    DEBUG_PRINTLN(SPIFFS.format());
  }

  DynamicJsonBuffer jsonBuffer;
  JsonObject &json = jsonBuffer.createObject();

  json["MQTT_server"]             = mqtt_server;
  json["MQTT_port"]               = mqtt_port;
  json["MQTT_uname"]              = mqtt_username;
  json["MQTT_pwd"]                = mqtt_key;
  json["MQTT_base"]               = mqtt_base;
  
  json["ip"]                      = WiFi.localIP().toString();
  json["gateway"]                 = WiFi.gatewayIP().toString();
  json["subnet"]                  = WiFi.subnetMask().toString();

  json["tempON"]                  = tempON;
  json["tempOFF"]                 = tempOFF;
  json["sensorOrder[0]"]          = sensorOrder[0];
  json["sensorOrder[1]"]          = sensorOrder[1];
  json["sensorOrder[2]"]          = sensorOrder[2];
  json["sensorOrder[3]"]          = sensorOrder[3];
  json["sensorOrder[4]"]          = sensorOrder[4];
  json["sensorOrder[5]"]          = sensorOrder[5];
  json["sensorOrder[6]"]          = sensorOrder[6];
  json["sensorOrder[7]"]          = sensorOrder[7];
  
  
  File configFile = SPIFFS.open(CFGFILE, "w+");
  if (!configFile) {
    DEBUG_PRINTLN(F("Failed to open config file for writing"));
    SPIFFS.end();
    return false;
  } else {
    if (isDebugEnabled) {
      json.printTo(Serial);
    }
    json.printTo(configFile);
    configFile.close();
    SPIFFS.end();
    DEBUG_PRINTLN(F("\nSaved successfully"));
    return true;
  }
}

bool readConfig() {
  DEBUG_PRINT(F("Mounting FS..."));

  if (SPIFFS.begin()) {
    DEBUG_PRINTLN(F(" mounted!"));
    if (SPIFFS.exists(CFGFILE)) {
      // file exists, reading and loading
      DEBUG_PRINTLN(F("Reading config file"));
      File configFile = SPIFFS.open(CFGFILE, "r");
      if (configFile) {
        DEBUG_PRINTLN(F("Opened config file"));
        size_t size = configFile.size();
        // Allocate a buffer to store contents of the file.
        std::unique_ptr<char[]> buf(new char[size]);

        configFile.readBytes(buf.get(), size);
        DynamicJsonBuffer jsonBuffer;
        JsonObject &json = jsonBuffer.parseObject(buf.get());

        if (json.success()) {
          DEBUG_PRINTLN(F("Parsed json"));
          
          if (json.containsKey("MQTT_server")) {
            strcpy(mqtt_server, json["MQTT_server"]);
            DEBUG_PRINT(F("MQTT server: "));
            DEBUG_PRINTLN(mqtt_server);
          }
          if (json.containsKey("MQTT_port")) {
            mqtt_port = json["MQTT_port"];
            DEBUG_PRINT(F("MQTT port: "));
            DEBUG_PRINTLN(mqtt_port);
          }
          if (json.containsKey("MQTT_uname")) {
            strcpy(mqtt_username, json["MQTT_uname"]);
            DEBUG_PRINT(F("MQTT username: "));
            DEBUG_PRINTLN(mqtt_username);
          }
          if (json.containsKey("MQTT_pwd")) {
            strcpy(mqtt_key, json["MQTT_pwd"]);
            DEBUG_PRINT(F("MQTT password: "));
            DEBUG_PRINTLN(mqtt_key);
          }
          if (json.containsKey("MQTT_base")) {
            strcpy(mqtt_base, json["MQTT_base"]);
            DEBUG_PRINT(F("MQTT base: "));
            DEBUG_PRINTLN(mqtt_base);
          }
          if(json["ip"]) {
            DEBUG_PRINTLN("setting custom ip from config");
            strcpy(static_ip, json["ip"]);
            strcpy(static_gw, json["gateway"]);
            strcpy(static_sn, json["subnet"]);
            DEBUG_PRINTLN(static_ip);
          } else {
            DEBUG_PRINTLN("no custom ip in config");
          }

          if (json.containsKey("tempON")) {
            tempON = json["tempON"];
            DEBUG_PRINT(F("tempON: "));
            DEBUG_PRINTLN(tempON);
          }
          
          if (json.containsKey("tempOFF")) {
            tempOFF = json["tempOFF"];
            DEBUG_PRINT(F("tempOFF: "));
            DEBUG_PRINTLN(tempOFF);
          }
          
          if (json.containsKey("sensorOrder[0]")) {
            sensorOrder[0] = json["sensorOrder[0]"];
            DEBUG_PRINT(F("sensorOrder[0]: "));
            DEBUG_PRINTLN(sensorOrder[0]);
          } 
          if (json.containsKey("sensorOrder[1]")) {
            sensorOrder[1] = json["sensorOrder[1]"];
            DEBUG_PRINT(F("sensorOrder[1]: "));
            DEBUG_PRINTLN(sensorOrder[1]);
          } 
          if (json.containsKey("sensorOrder[2]")) {
            sensorOrder[2] = json["sensorOrder[2]"];
            DEBUG_PRINT(F("sensorOrder[2]: "));
            DEBUG_PRINTLN(sensorOrder[2]);
          } 
          if (json.containsKey("sensorOrder[3]")) {
            sensorOrder[3] = json["sensorOrder[3]"];
            DEBUG_PRINT(F("sensorOrder[3]: "));
            DEBUG_PRINTLN(sensorOrder[3]);
          } 
          if (json.containsKey("sensorOrder[4]")) {
            sensorOrder[4] = json["sensorOrder[4]"];
            DEBUG_PRINT(F("sensorOrder[4]: "));
            DEBUG_PRINTLN(sensorOrder[4]);
          } 
          if (json.containsKey("sensorOrder[5]")) {
            sensorOrder[5] = json["sensorOrder[5]"];
            DEBUG_PRINT(F("sensorOrder[5]: "));
            DEBUG_PRINTLN(sensorOrder[5]);
          } 
          if (json.containsKey("sensorOrder[6]")) {
            sensorOrder[6] = json["sensorOrder[6]"];
            DEBUG_PRINT(F("sensorOrder[6]: "));
            DEBUG_PRINTLN(sensorOrder[6]);
          } 
          if (json.containsKey("sensorOrder[7]")) {
            sensorOrder[6] = json["sensorOrder[7]"];
            DEBUG_PRINT(F("sensorOrder[7]: "));
            DEBUG_PRINTLN(sensorOrder[7]);
          } 
          
          DEBUG_PRINTLN(F("Parsed config:"));
          if (isDebugEnabled) {
            json.printTo(Serial);
            DEBUG_PRINTLN();
          }
          return true;
        }
        else {
          DEBUG_PRINTLN(F("ERROR: failed to load json config"));
          return false;
        }
      }
      DEBUG_PRINTLN(F("ERROR: unable to open config file"));
    } else {
      DEBUG_PRINTLN(F("ERROR: config file not exist"));
    }
  } else {
    DEBUG_PRINTLN(F(" ERROR: failed to mount FS!"));
  }
  return false;
}

bool sendDataHA(void *) {
  digitalWrite(BUILTIN_LED, LOW);
  printSystemTime();
  DEBUG_PRINTLN(F(" - I am sending data to HA"));
  
  SenderClass sender;
  sender.add("tINKamna", tempIN);
  sender.add("tOUTKamna", tempOUT);
  sender.add("sPumpKamna/status", relay1==LOW ? 1 : 0);
  /*sender.add("t0", t0);
  sender.add("t1", t1);
  sender.add("t2", t2);
  sender.add("t3", t3);
  sender.add("t4", t4);
  sender.add("t5", t5);
  sender.add("t6", t6);
  sender.add("t7", t7);
  sender.add("t8", t8);
  sender.add("t9", t9);
  sender.add("t10", t10);
  sender.add("t11", t11);*/
  DEBUG_PRINTLN(F("Calling MQTT"));

  sender.sendMQTT(mqtt_server, mqtt_port, mqtt_username, mqtt_key, mqtt_base);
  digitalWrite(BUILTIN_LED, HIGH);
  interrupts();
  return true;
}

bool sendStatisticHA(void *) {
  digitalWrite(BUILTIN_LED, LOW);
  printSystemTime();
  DEBUG_PRINTLN(F(" - I am sending statistic to HA"));

  SenderClass sender;
  sender.add("VersionSWCentral", VERSION);
  sender.add("Napeti",  ESP.getVcc());
  sender.add("HeartBeat", heartBeat++);
  sender.add("RSSI", WiFi.RSSI());
  sender.add("tempON", tempON);
  sender.add("tempOFF", tempOFF);
 
  DEBUG_PRINTLN(F("Calling MQTT"));
  
  sender.sendMQTT(mqtt_server, mqtt_port, mqtt_username, mqtt_key, mqtt_base);
  digitalWrite(BUILTIN_LED, HIGH);
  return true;
}


#ifdef time
/*-------- NTP code ----------*/

const int NTP_PACKET_SIZE = 48; // NTP time is in the first 48 bytes of message
byte packetBuffer[NTP_PACKET_SIZE]; //buffer to hold incoming & outgoing packets

time_t getNtpTime()
{
  //IPAddress ntpServerIP; // NTP server's ip address
  IPAddress ntpServerIP = IPAddress(195, 113, 144, 201);

  while (EthernetUdp.parsePacket() > 0) ; // discard any previously received packets
  DEBUG_PRINTLN("Transmit NTP Request");
  // get a random server from the pool
  //WiFi.hostByName(ntpServerName, ntpServerIP);
  DEBUG_PRINT(ntpServerName);
  DEBUG_PRINT(": ");
  DEBUG_PRINTLN(ntpServerIP);
  sendNTPpacket(ntpServerIP);
  uint32_t beginWait = millis();
  while (millis() - beginWait < 1500) {
    int size = EthernetUdp.parsePacket();
    if (size >= NTP_PACKET_SIZE) {
      DEBUG_PRINTLN("Receive NTP Response");
      EthernetUdp.read(packetBuffer, NTP_PACKET_SIZE);  // read packet into the buffer
      unsigned long highWord = word(packetBuffer[40], packetBuffer[41]);
      unsigned long lowWord = word(packetBuffer[42], packetBuffer[43]);
      // combine the four bytes (two words) into a long integer
      // this is NTP time (seconds since Jan 1 1900):
      unsigned long secsSince1900 = highWord << 16 | lowWord;
      DEBUG_PRINT("Seconds since Jan 1 1900 = " );
      DEBUG_PRINTLN(secsSince1900);

      // now convert NTP time into everyday time:
      DEBUG_PRINT("Unix time = ");
      // Unix time starts on Jan 1 1970. In seconds, that's 2208988800:
      const unsigned long seventyYears = 2208988800UL;
      // subtract seventy years:
      unsigned long epoch = secsSince1900 - seventyYears;
      // print Unix time:
      DEBUG_PRINTLN(epoch);
	  
      TimeChangeRule *tcr;
      time_t utc;
      utc = epoch;
      
      return CE.toLocal(utc, &tcr);
      //return secsSince1900 - 2208988800UL + timeZone * SECS_PER_HOUR;
    }
  }
  DEBUG_PRINTLN("No NTP Response :-(");
  return 0; // return 0 if unable to get the time
}

// send an NTP request to the time server at the given address
void sendNTPpacket(IPAddress &address)
{
  // set all bytes in the buffer to 0
  memset(packetBuffer, 0, NTP_PACKET_SIZE);
  // Initialize values needed to form NTP request
  // (see URL above for details on the packets)
  packetBuffer[0] = 0b11100011;   // LI, Version, Mode
  packetBuffer[1] = 0;     // Stratum, or type of clock
  packetBuffer[2] = 6;     // Polling Interval
  packetBuffer[3] = 0xEC;  // Peer Clock Precision
  // 8 bytes of zero for Root Delay & Root Dispersion
  packetBuffer[12] = 49;
  packetBuffer[13] = 0x4E;
  packetBuffer[14] = 49;
  packetBuffer[15] = 52;
  // all NTP fields have been given values, now
  // you can send a packet requesting a timestamp:
  EthernetUdp.beginPacket(address, 123); //NTP requests are to port 123
  EthernetUdp.write(packetBuffer, NTP_PACKET_SIZE);
  EthernetUdp.endPacket();
}

void printSystemTime(){
  DEBUG_PRINT(day());
  DEBUG_PRINT(".");
  DEBUG_PRINT(month());
  DEBUG_PRINT(".");
  DEBUG_PRINT(year());
  DEBUG_PRINT(" ");
  print2digits(hour());
  DEBUG_PRINT(":");
  print2digits(minute());
  DEBUG_PRINT(":");
  print2digits(second());
}

bool displayTime(void *) {
  lcd.setCursor(TIMEX, TIMEY); //col,row
  lcd2digits(hour());
  if (showDoubleDot) {
    showDoubleDot = false;
    lcd.write(':');
  } else {
    showDoubleDot = true;
    lcd.write(' ');
  }
  lcd2digits(minute());
/*  lcd.write(':');
  lcd2digits(second());*/
  //zobrazeni hlasky o zmene uhlu kolektoru
}
#endif

//display time on LCD
void lcd2digits(int number) {
  if (number >= 0 && number < 10) {
    lcd.write('0');
  }
  lcd.print(number);
}

void print2digits(int number) {
  if (number >= 0 && number < 10) {
    DEBUG_WRITE('0');
  }
  DEBUG_PRINT(number);
}



bool tempMeas(void *) {
  dsSensorsOUT.requestTemperatures(); 
  dsSensorsIN.requestTemperatures(); 
  dsSensorsUT.requestTemperatures(); 

  tempIN = dsSensorsIN.getTempCByIndex(0);
  tempOUT = dsSensorsOUT.getTempCByIndex(0);
  
  if (tempIN>tMaxIn)           tMaxIn      = tempIN;
  if (tempOUT>tMaxOut)         tMaxOut     = tempOUT;


  tempUT[0]=dsSensorsUT.getTempCByIndex(5);   //obyvak vstup
  tempUT[1]=dsSensorsUT.getTempCByIndex(4);   //obyvak vystup
  tempUT[2]=dsSensorsUT.getTempCByIndex(3);   //loznice nova vstup
  tempUT[3]=dsSensorsUT.getTempCByIndex(11);  //loznice nova vystup
  tempUT[4]=dsSensorsUT.getTempCByIndex(2);   //loznice stara vstup
  tempUT[5]=dsSensorsUT.getTempCByIndex(9);   //loznice stara vystup 
  tempUT[6]=dsSensorsUT.getTempCByIndex(1);   //dilna vstup
  tempUT[7]=dsSensorsUT.getTempCByIndex(0);   //dilna vystup
  tempUT[8]=dsSensorsUT.getTempCByIndex(8);   //bojler vstup
  tempUT[9]=dsSensorsUT.getTempCByIndex(6);   //bojler vystup
  tempUT[10]=dsSensorsUT.getTempCByIndex(7);  //chodba vstup
  tempUT[11]=dsSensorsUT.getTempCByIndex(10); //chodba vystup
  
  //obcas se vyskytne chyba a vsechna cidla prestanou merit
  //zkusim restartovat sbernici
  bool reset=true;
  for (byte i=0; i<numberOfDevices; i++) {
    if (sensor[i]!=0.0) {
      reset=false;
    }
  }
  if (reset) {
    dsInit();
  }
  firstTempMeasDone = true;
  return true;
}


//---------------------------------------------M A I N  C O N T R O L ------------------------------------------------
void mainControl() {
  //safety function
  if ((tempOUT >= SAFETY_ON) || (tempIN >= SAFETY_ON)) {
    relay1=LOW; //relay ON
    DEBUG_PRINTLN(F("SAFETY CONTROL!!!!"));
  } else if (manualON) {
    DEBUG_PRINTLN(F("MANUAL CONTROL!!!!"));
  } else {
    //pump is ON - relay ON = LOW
    if (relay1==LOW) { 
      if ((tempOUT < tempOFF) && (millis() - DELAY_AFTER_ON >= lastOffOn)) { //switch pump ON->OFF
        DEBUG_PRINT(F("millis()="));
        DEBUG_PRINT(millis());
        DEBUG_PRINT(F(" delayAfterON="));
        DEBUG_PRINT(DELAY_AFTER_ON);
        DEBUG_PRINT(F(" lastOffOn="));
        DEBUG_PRINT(lastOffOn);
        DEBUG_PRINT(F(" tempOut="));
        DEBUG_PRINT(tempOUT);
        relay1=HIGH; //relay OFF = HIGH
        lastOff=millis();
      }
    } else { //pump is OFF - relay OFF = HIGH
      if (tempOUT >= tempON) { //switch pump OFF->ON
        relay1=LOW; //relay ON = LOW
        lastOffOn = millis();
      }
    }
  }
  digitalWrite(RELAY1PIN, relay1);
}

//---------------------------------------------D I S P L A Y ------------------------------------------------
void lcdShow() {
  if (display>=100) { 
    lcd.setCursor(POZ0X,POZ0Y);
  }
  // if (millis() > SHOW_INFO_DELAY + showInfo) {
    // showInfo = millis();
    // lcd.setCursor(0,3);
    // for (byte i=0;i<14;i++) {
      // PRINT_SPACE;
    // }
  // }
  
  if (display==DISPLAY_MAIN) {
    //    012345678901234567890
    //00  10  9  0  -0 -9.1  T    
    //01   555W   12.3kWh 124m                    
    //02   2.1l/m  55 45 48  0
    //03                123456
    // displayTemp(TEMP1X,TEMP1Y, tP1In, false);
    // displayTemp(TEMP2X,TEMP2Y, tP1Out, false);
    // displayTemp(TEMP3X,TEMP3Y, tP2In, false);
    // displayTemp(TEMP4X,TEMP4Y, tP2Out, false);
    // displayTemp(TEMP5X,TEMP5Y, tControl, true);
    // displayTemp(TEMP6X,TEMP6Y, tBojlerIn, false);
    // displayTemp(TEMP7X,TEMP7Y, tBojlerOut, false);
    // displayTemp(TEMP8X,TEMP8Y, tBojler, false);
    // lcd.setCursor(POWERX,POWERY);
    //} else {
      //zobrazeni okamziteho vykonu ve W
      //zobrazeni celkoveho vykonu za den v kWh
      //zobrazeni poctu minut behu cerpadla za aktualni den
      //0123456789012345
      // 636 0.1234 720T
      // unsigned int p=(int)power;
      // if (p<10000) PRINT_SPACE
      // if (p<1000) PRINT_SPACE
      // if (p<100) PRINT_SPACE
      // if (p<10) PRINT_SPACE
      // if (power<=65534) {
        // lcd.print(p);
        // lcd.print(F("W"));
      // }
      
    // lcd.setCursor(ENERGYX,ENERGYY);
      // lcd.print(enegyWsTokWh(energyADay)); //Ws -> kWh (show it in kWh)
      // lcd.print(F("kWh"));
    // lcd.setCursor(FLOWX,FLOWY);
      // lcd.print(lMin);
      // lcd.print(F("l/m"));
    //}
    displayRelayStatus();
    // lcd.setCursor(MINRUNX, MINRUNY);
    // if (lastRunMin<100000) PRINT_SPACE
    // if (lastRunMin<10000) PRINT_SPACE
    // if (lastRunMin<1000) PRINT_SPACE
    // if (lastRunMin<100) PRINT_SPACE
    // if (lastRunMin<10) PRINT_SPACE
    // lcd.print(lastRunMin);
    // lcd.setCursor(CONTROLSENSORX, CONTROLSENSORY);
    // controlSensorBojler==1 ? lcd.print(F("B")) : lcd.print(F("R"));
    // lcd.setCursor(SUNANGLEX,SUNANGLEY);
    // lcd.print(sunAngle[month()-1]);
    
    
  } else if (display==DISPLAY_T_DIFF_ON) {
    displayInfoValue('tempON', tempON, 'C');
  } else if (display==DISPLAY_T_DIFF_OFF) { 
    displayInfoValue('tempOFF', tempOFF, 'C');
  } else if (display==DISPLAY_MAX_IO_TEMP) {
    lcd.setCursor(POZ0X,POZ0Y);
    lcd.print(F("Max IN:"));
    lcd.print(tMaxIn);
    lcd.print(F("     "));
    lcd.setCursor(0,1);
    lcd.print(F("Max OUT:"));
    lcd.print(tMaxOut);
    lcd.print(F("     "));
  // } else if (display==DISPLAY_MAX_POWER_TODAY) { 
    // //displayInfoValue('Max power today', maxPower, 'W');
  }
}

void displayTemp(int x, int y, float value, bool des) {
  /*
  value     des=true   des=false
            0123       0123
  89.3      89.3       89
  10.0      10.0       10
   9.9       9.9        9
   1.1       1.1        1
   0.9       0.9        0
   0.1       0.0        0
   0.0       0.0        0
  -0.1      -0.1       -0
  -0.9      -0.9       -0
  -1.0      -1.0       -1
  -9.9      -9.9       -9
 -10.0      -10        -10
 -25.2      -25        -25  
   */
  lcd.setCursor(x,y);
  
  //DEBUG_PRINTLN(F(value);
  if (!des) {
    value = round(value);
  }
  
  if (value<10.f && value>=0.f) {
    //DEBUG_PRINT(F("_"));
    lcd.print(F(" "));
  } else if (value<0.f && value>-10.f) {
    //DEBUG_PRINT(F("_"));
    lcd.print(F("-"));
  } else if (value<-10.f) {
    des = false;
    //DEBUG_PRINT("-"));
  }
  
  if (value>=100.f) {
    value=value-100.f;
  }
 
  lcd.print(abs((int)value));
  if (des) {
    lcd.print(F("."));
    lcd.print(abs((int)(value*10)%10));
  }
  lcd.print(F(" "));
}


//---------------------------------------------K E Y B O A R D ------------------------------------------------
void keyBoard() {
  char key = keypad.getKey();
  if (key!=NO_KEY){
    lcd.clear();
    //DEBUG_PRINTLN(key);
    /*
    Keyboard layout
    -----------
    | 1 2 3 A |
    | 4 5 6 B |
    | 7 8 9 C |
    | * 0 # D |
    -----------
   
    INFO
    1 - 
    2 - tempON
    3 - tempOFF
    4 - 
    5 - Max IN OUT temp
    6 - 
    B - clear display
    7 - 
    8 - 
    9 - 
    C - RESTART
    * -
    0 - main display
    D - manual/auto
    */
    
    if (key=='1') { 
    }
    else if (key=='2') { 
      display=DISPLAY_T_DIFF_ON;
    }
    else if (key=='3') { 
      display=DISPLAY_T_DIFF_OFF;
    }
    else if (key=='5') { 
      display=DISPLAY_MAX_IO_TEMP;
    }
    else if (key=='B') {
      lcd.clear();
    }
    else if (key=='9') { 
    }
    if (key=='C') {
      saveConfig();
      ESP.restart();
    }
    if (key=='*') {
    }
    if (key=='0') { 
      display=DISPLAY_MAIN;
    }
    if (key=='D') {
      manualON = !manualON;
      if (manualON) {
        relay1=LOW;
      } else {
        relay1=HIGH;
      }
    }
    key = ' ';
  }
}

void dsInit(void) {
  dsSensorsOUT.begin();
  dsSensorsIN.begin();
  dsSensorsUT.begin();
  numberOfDevices = dsSensorsUT.getDeviceCount();

  lcd.setCursor(0,3);
  lcd.print(numberOfDevices);
  DEBUG_PRINT(numberOfDevices);
  
  if (numberOfDevices==1) {
    DEBUG_PRINTLN(" sensor found");
    lcd.print(F(" sensor found"));
  } else {
    DEBUG_PRINTLN(" sensor(s) found");
    lcd.print(F(" sensors found"));
  }

  // Loop through each device, print out address
  for (byte i=0;i<numberOfDevices; i++) {
      // Search the wire for address
    if (dsSensorsUT.getAddress(tempDeviceAddress, i)) {
      memcpy(tempDeviceAddresses[i],tempDeviceAddress,8);
    }
  }
  dsSensorsOUT.setResolution(TEMPERATURE_PRECISION);
  dsSensorsOUT.setWaitForConversion(false);
  dsSensorsIN.setResolution(TEMPERATURE_PRECISION);
  dsSensorsIN.setWaitForConversion(false);
  dsSensorsUT.setResolution(TEMPERATURE_PRECISION);
  dsSensorsUT.setWaitForConversion(false);
}

// float enegyWsTokWh(float e) {
  // return e/3600.f/1000.f;
// }

//show relay's status in column 15
void displayRelayStatus(void) {
  lcd.setCursor(RELAY1X,RELAY1Y);
  if (manualON) {
    lcd.print(F("M"));
  } else {
    if (relay1==LOW)
      lcd.print(F("T"));
    else
      lcd.print(F("N"));
  }
  if (manualON) {
    //DEBUG_PRINTLN(F("Manual"));
  } else {
  }
/*  lcd.setCursor(RELAY2X,RELAY2Y);
  if (relay2==LOW)
    lcd.print(F("T"));
  else
    lcd.print(F("N"));
*/
}

void displayInfoValue(char text1, float value, char text2) {
  lcd.setCursor(POZ0X,POZ0Y);
  lcd.print(text1);
  lcd.setCursor(POZ0X,POZ1Y);
  lcd.print(value);
  lcd.print(" ");
  lcd.print(text2);
  lcd.print("          ");
}

bool countMinRun(void *) {
  lastRunMin += 1;
  return true;
}

float enegyWsTokWh(float e) {
  return e/3600.f/1000.f;
}

// unsigned int getPower(float t1, float t2) {
  // /*
  // Q	0,00006	m3/s
  // K	4184000	
  // t vstup	56,4	°C
  // t vystup	63,7	
  // P = Q x K x (t1 - t2)	1832,592	W
  // */
  // return (lMin / 1000.0 / 60.0) * 4184000.0 * (t1 - t2);
  // //return (float)energyKoef*(tBojlerOut-tBojlerIn); //in W
// }

