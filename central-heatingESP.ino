/*
--------------------------------------------------------------------------------------------------------------------------
CENTRAL HEATING - control system for central heating
Petr Fory pfory@seznam.cz
GIT - https://github.com/pfory/central-heating
*/

#include "Configuration.h"


#include <Wire.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include "Adafruit_MQTT.h"
#include "Adafruit_MQTT_Client.h"
#include <FS.h>
#include <ArduinoOTA.h>
#include <WiFiManager.h> 
#include <TimeLib.h>
#include <Timezone.h>
#include <WiFiUdp.h>

// Setup a oneWire instance to communicate with any OneWire devices (not just Maxim/Dallas temperature ICs)
OneWire oneWireOUT(ONE_WIRE_BUS_OUT);
OneWire oneWireIN(ONE_WIRE_BUS_IN);
OneWire oneWireUT(ONE_WIRE_BUS_UT);

// Pass our oneWire reference to Dallas Temperature. 
DallasTemperature sensorsOUT(&oneWireOUT);
DallasTemperature sensorsIN(&oneWireIN);
DallasTemperature sensorsUT(&oneWireUT);

DeviceAddress inThermometer, outThermometer;
DeviceAddress utT[15];

const unsigned long   measDelay                  = 10000; //in ms
unsigned long         lastMeas                   = measDelay * -1;
const unsigned long   measTime                   = 750; //in ms
const unsigned long   sendDelay                  = 20000; //in ms
unsigned long         lastSend                   = sendDelay * -1;
bool                  startConversion            = false;
unsigned long         startConversionMillis      = 0;
float                 tempOUT                    = 0.f;
float                 tempIN                     = 0.f;
float                 tempUT[12];             
bool                  relay                      = HIGH;
const unsigned long   pumpProtect                = 864000000;  //1000*60*60*24*10; //in ms = 10 day, max 49 days
const unsigned long   pumpProtectRun             = 300000;     //1000*60*5;     //in ms = 5 min
bool                  firstMeasComplete          = false;
bool                  tempRefresh                = false;
unsigned long         hbDelay                    = 500;
unsigned long         lastHB                     = hbDelay * -1;
uint32_t              heartBeat                  = 0;
float                 temp[15];
float                 tempINKamna, tempOUTKamna  = 0;
int                   pumpStatus                 = 0;

#define SIMTEMP

static const char ntpServerName[] = "tik.cesnet.cz";
//const int timeZone = 2;     // Central European Time
//Central European Time (Frankfurt, Paris)
TimeChangeRule CEST = {"CEST", Last, Sun, Mar, 2, 120};     //Central European Summer Time
TimeChangeRule CET = {"CET", Last, Sun, Oct, 3, 60};       //Central European Standard Time
Timezone CE(CEST, CET);

WiFiUDP EthernetUdp;
unsigned int localPort = 8888;  // local port to listen for UDP packets
time_t getNtpTime();

#define beep
// #ifdef beep
// #include "beep.h"
// Beep peep(BUZZERPIN);
// #endif

#define verbose
#ifdef verbose
  #define DEBUG_PRINT(x)         Serial.print (x)
  #define DEBUG_PRINTDEC(x)      Serial.print (x, DEC)
  #define DEBUG_PRINTLN(x)       Serial.println (x)
  #define DEBUG_PRINTF(x, y)     Serial.printf (x, y)
  #define DEBUG_PRINTHEX(x)   Serial.print (x, HEX)
  #define PORTSPEED 115200
#else
  #define DEBUG_PRINT(x)
  #define DEBUG_PRINTDEC(x)
  #define DEBUG_PRINTLN(x)
  #define DEBUG_PRINTF(x, y)
#endif 

struct StoreStruct {
  // The variables of your settings
  unsigned int    moduleId;  // module id
  bool            relay;     // relay state
  float           tempON;
  float           tempOFFDiff;
  float           tempAlarm;
  //tmElements_t    lastPumpRun;
} storage = {
  // The default module 0
  0,
  0, // off
  70,
  5,
  95
};

#include <LiquidCrystal_I2C.h>
LiquidCrystal_I2C lcd(LCDADDRESS,LCDCOLS,LCDROWS);  // set the LCD
bool backLight = true;

#include <avr/pgmspace.h>
unsigned long crc;
const PROGMEM uint32_t crc_table[16] = {
    0x00000000, 0x1db71064, 0x3b6e20c8, 0x26d930ac,
    0x76dc4190, 0x6b6b51f4, 0x4db26158, 0x5005713c,
    0xedb88320, 0xf00f9344, 0xd6d6a3e8, 0xcb61b38c,
    0x9b64c2b0, 0x86d3d2d4, 0xa00ae278, 0xbdbdf21c
};


#include <Keypad_I2C.h>
#include <Keypad.h>          // GDY120705
#include <Wire.h>


const byte ROWS = 4; //four rows
const byte COLS = 4; //three columns
char keys[ROWS][COLS]                 = {
                                            {'1','2','3','A'},
                                            {'4','5','6','B'},
                                            {'7','8','9','C'},
                                            {'*','0','#','D'}
};
byte rowPins[ROWS] = {7,6,5,4}; //connect to the row pinouts of the keypad
byte colPins[COLS] = {3,2,1,0}; //connect to the column pinouts of the keypad

//Keypad_I2C keypad = Keypad_I2C( makeKeymap(keys), rowPins, colPins, ROWS, COLS, I2CADDR );
Keypad_I2C keypad( makeKeymap(keys), rowPins, colPins, ROWS, COLS, I2CADDR); 
// #endif

byte displayVar=1;
char displayVarSub=' ';

//for LED status
#include <Ticker.h>
Ticker ticker;

void tick()
{
  //toggle state
  int state = digitalRead(BUILTIN_LED);  // get the current state of GPIO1 pin
  digitalWrite(BUILTIN_LED, !state);     // set pin to the opposite state
}

#define AIO_SERVER      "192.168.1.56"
//#define AIO_SERVER      "178.77.238.20"
#define AIO_SERVERPORT  1883
#define AIO_USERNAME    "datel"
#define AIO_KEY         "hanka12"

WiFiClient client;

Adafruit_MQTT_Client mqtt(&client, AIO_SERVER, AIO_SERVERPORT, AIO_USERNAME, AIO_KEY);

/****************************** Feeds ***************************************/
Adafruit_MQTT_Publish version             = Adafruit_MQTT_Publish(&mqtt, MQTTBASE "VersionSWCentral");
Adafruit_MQTT_Publish hb                  = Adafruit_MQTT_Publish(&mqtt, MQTTBASE "HeartBeat");
Adafruit_MQTT_Publish tINKamna            = Adafruit_MQTT_Publish(&mqtt, MQTTBASE "tINKamna");
Adafruit_MQTT_Publish tOUTKamna           = Adafruit_MQTT_Publish(&mqtt, MQTTBASE "tOUTKamna");
Adafruit_MQTT_Publish sPumpKamna          = Adafruit_MQTT_Publish(&mqtt, MQTTBASE "sPumpKamna/status");
Adafruit_MQTT_Publish t0                  = Adafruit_MQTT_Publish(&mqtt, MQTTBASE "t0");
Adafruit_MQTT_Publish t1                  = Adafruit_MQTT_Publish(&mqtt, MQTTBASE "t1");
Adafruit_MQTT_Publish t2                  = Adafruit_MQTT_Publish(&mqtt, MQTTBASE "t2");
Adafruit_MQTT_Publish t3                  = Adafruit_MQTT_Publish(&mqtt, MQTTBASE "t3");
Adafruit_MQTT_Publish t4                  = Adafruit_MQTT_Publish(&mqtt, MQTTBASE "t4");
Adafruit_MQTT_Publish t5                  = Adafruit_MQTT_Publish(&mqtt, MQTTBASE "t5");
Adafruit_MQTT_Publish t6                  = Adafruit_MQTT_Publish(&mqtt, MQTTBASE "t6");
Adafruit_MQTT_Publish t7                  = Adafruit_MQTT_Publish(&mqtt, MQTTBASE "t7");
Adafruit_MQTT_Publish t8                  = Adafruit_MQTT_Publish(&mqtt, MQTTBASE "t8");
Adafruit_MQTT_Publish t9                  = Adafruit_MQTT_Publish(&mqtt, MQTTBASE "t9");
Adafruit_MQTT_Publish t10                 = Adafruit_MQTT_Publish(&mqtt, MQTTBASE "t10");
Adafruit_MQTT_Publish t11                 = Adafruit_MQTT_Publish(&mqtt, MQTTBASE "t11");

Adafruit_MQTT_Subscribe tONSetup          = Adafruit_MQTT_Subscribe(&mqtt, MQTTBASE "tONSetup");
Adafruit_MQTT_Subscribe tDiffSetup        = Adafruit_MQTT_Subscribe(&mqtt, MQTTBASE "tDiffSetup");

IPAddress _ip           = IPAddress(192, 168, 1, 139);
IPAddress _gw           = IPAddress(192, 168, 1, 1);
IPAddress _sn           = IPAddress(255, 255, 255, 0);


void MQTT_connect(void);

extern "C" {
  #include "user_interface.h"
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


/////////////////////////////////////////////   S  E  T  U  P   ////////////////////////////////////
void setup(void) {
#ifdef verbose
  Serial.begin(PORTSPEED);
#endif
  DEBUG_PRINT(F(SW_NAME));
  DEBUG_PRINT(F(" "));
  DEBUG_PRINTLN(F(VERSION));

  pinMode(BUILTIN_LED, OUTPUT);
  // start ticker with 0.5 because we start in AP mode and try to connect
  ticker.attach(0.6, tick);
  
  DEBUG_PRINTLN(ESP.getResetReason());
  if (ESP.getResetReason()=="Software/System restart") {
    heartBeat=1;
  } else if (ESP.getResetReason()=="Power on") {
    heartBeat=2;
  } else if (ESP.getResetReason()=="External System") {
    heartBeat=3;
  } else if (ESP.getResetReason()=="Hardware Watchdog") {
    heartBeat=4;
  } else if (ESP.getResetReason()=="Exception") {
    heartBeat=5;
  } else if (ESP.getResetReason()=="Software Watchdog") {
    heartBeat=6;
  } else if (ESP.getResetReason()=="Deep-Sleep Wake") {
    heartBeat=7;
  }

  //WiFiManager
  //Local intialization. Once its business is done, there is no need to keep it around
  WiFiManager wifiManager;
  //reset settings - for testing
  //wifiManager.resetSettings();
  wifiManager.setConnectTimeout(60); //5min

  //set callback that gets called when connecting to previous WiFi fails, and enters Access Point mode
  wifiManager.setAPCallback(configModeCallback);
  
  //DEBUG_PRINTLN(ESP.getFlashChipRealSize);
  //DEBUG_PRINTLN(ESP.getCpuFreqMHz);
  //WiFi.begin(ssid, password);
  wifiManager.setSTAStaticIPConfig(_ip, _gw, _sn);
  
 
  if (!wifiManager.autoConnect("CentralHeating", "password")) {
    DEBUG_PRINTLN("failed to connect, we should reset as see if it connects");
    delay(3000);
    ESP.reset();
    delay(5000);
  }

  DEBUG_PRINTLN("Setup TIME");
  EthernetUdp.begin(localPort);
  DEBUG_PRINT("Local port: ");
  DEBUG_PRINTLN(EthernetUdp.localPort());
  DEBUG_PRINTLN("waiting for sync");
  setSyncProvider(getNtpTime);
  setSyncInterval(300);
  
  printSystemTime();
#ifdef beep
  //peep.Delay(100,40,1,255);
  tone(BUZZERPIN, 5000, 5);
#endif  
  lcd.init();               // initialize the lcd 
  lcd.backlight();
  lcd.home();
  lcd.print(SW_NAME);
  PRINT_SPACE
  lcd.print(VERSION);
  
  keypad.begin();
  
  loadConfig();
  DEBUG_PRINTLN();
  DEBUG_PRINT(F("Temp ON "));
  DEBUG_PRINTLN(storage.tempON);
  DEBUG_PRINT(F("Temp OFF diff "));
  DEBUG_PRINTLN(storage.tempOFFDiff);
  DEBUG_PRINT(F("Temp alarm "));
  DEBUG_PRINTLN(storage.tempAlarm);
 
  pinMode(RELAYPIN, OUTPUT);
  pinMode(LEDPIN, OUTPUT);
  digitalWrite(RELAYPIN,relay);
  digitalWrite(LEDPIN,!relay);
  delay(1000);
  lcd.clear();
  while (true) {
    sensorsOUT.begin(); 
    sensorsIN.begin(); 
    sensorsUT.begin(); 
#ifdef SIMTEMP
    if (1==0) {
#else
    if (sensorsIN.getDeviceCount()==0 || sensorsOUT.getDeviceCount()==0) {
#endif
#ifdef beep
      //peep.Delay(100,40,1,255);
#endif
      DEBUG_PRINTLN(F("NO temperature sensor(s) DS18B20 found!!!!!!!!!"));
      lcd.setCursor(0, 1);
      lcd.print(F("!NO temp.sensor(s)!!"));
      lcd.setCursor(0, 2);
      lcd.print(F("!!!DS18B20 found!!!!"));
      lcd.setCursor(0, 3);
      lcd.print(F("!!!!!Check wire!!!!!"));
      displayTime();
      //break;
      } else {
      break;
    }
    delay(800);
  }

  sensorsOUT.setResolution(TEMPERATURE_PRECISION);
  sensorsOUT.setWaitForConversion(false);
  sensorsIN.setResolution(TEMPERATURE_PRECISION);
  sensorsIN.setWaitForConversion(false);
  sensorsUT.setResolution(TEMPERATURE_PRECISION);
  sensorsUT.setWaitForConversion(false);

  
  DEBUG_PRINTLN();
  DEBUG_PRINT(F("Sensor(s) "));
  DEBUG_PRINT(sensorsIN.getDeviceCount());
  DEBUG_PRINT(F(" on bus IN - pin "));
  DEBUG_PRINTLN(ONE_WIRE_BUS_IN);
  DEBUG_PRINT(F("Device Address: "));
  sensorsIN.getAddress(inThermometer, 0); 
  printAddress(inThermometer);
  DEBUG_PRINTLN();
  
  
  lcd.setCursor(0,1);
  lcd.print(F("Sen."));
  lcd.print(sensorsIN.getDeviceCount());
  lcd.print(F(" bus IN"));

  DEBUG_PRINT(F("Sensor(s) "));
  DEBUG_PRINT(sensorsOUT.getDeviceCount());
  DEBUG_PRINT(F(" on bus OUT - pin "));
  DEBUG_PRINTLN(ONE_WIRE_BUS_OUT);
  DEBUG_PRINT(F("Device Address: "));
  sensorsOUT.getAddress(outThermometer, 0); 
  printAddress(outThermometer);
  DEBUG_PRINTLN();
   
  lcd.setCursor(0,2);
  lcd.print(F("Sen."));
  lcd.print(sensorsOUT.getDeviceCount());
  lcd.print(F(" bus OUT"));
 
  DEBUG_PRINT(F("Sensor(s) "));
  DEBUG_PRINT(sensorsUT.getDeviceCount());
  DEBUG_PRINT(F(" on bus UT - pin "));
  DEBUG_PRINTLN(ONE_WIRE_BUS_UT);
  DEBUG_PRINTLN(F("Device Address: "));
  for (byte i = 0; i<sensorsUT.getDeviceCount(); i++) {
    sensorsUT.getAddress(utT[i], i); 
    printAddress(utT[i]);
    DEBUG_PRINTLN();
  }
  
  lcd.setCursor(0,3);
  lcd.print(F("Sen."));
  lcd.print(sensorsUT.getDeviceCount());
  lcd.print(F(" bus UT"));

  
  
  DEBUG_PRINT(F("Send interval "));
  DEBUG_PRINT(sendDelay);
  DEBUG_PRINTLN(F(" sec"));
  
  delay(1000);
  lcd.clear();
  
  displayInfo();

  delay(1000);
  lcd.clear();
  
    //OTA
  // Port defaults to 8266
  // ArduinoOTA.setPort(8266);

  // Hostname defaults to esp8266-[ChipID]
  ArduinoOTA.setHostname("Central heating");

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
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed");
  });
  ArduinoOTA.begin();

  ticker.detach();
  //keep LED on
  digitalWrite(BUILTIN_LED, HIGH);

  
  DEBUG_PRINTLN(F("Setup end."));
}

/////////////////////////////////////////////   L  O  O  P   ///////////////////////////////////////
void loop(void) { 
  tempSetup();
  if (millis() - lastMeas >= measDelay) {
    lastMeas = millis();
    DEBUG_PRINTLN(millis());
    startMeas(); 
    startConversion=true;
    startConversionMillis = millis();
  }
  if (startConversion && (millis() - startConversionMillis >= measTime)) {
    startConversion=false;
    getTemp();
    //printTemp();
    tempRefresh = true;
    //poslani teploty do LED displeje
    Wire.beginTransmission(8);
    Wire.write((byte)tempOUT);
    Wire.endTransmission();

    if (tempOUT <= storage.tempON - storage.tempOFFDiff) {
      relay = HIGH;
      //storage.lastPumpRun = tm;
    }
    if ((tempOUT >= storage.tempON) || (tempIN >= storage.tempON)) {
      relay = LOW;
    }

    if (relay == LOW) {
      DEBUG_PRINTLN(F("Relay ON"));
    } else {
      DEBUG_PRINTLN(F("Relay OFF"));
    }

    digitalWrite(RELAYPIN,!relay);
    digitalWrite(LEDPIN,relay);
  }
  if (millis() - lastSend >= sendDelay) {
    sendDataMQTT();
    lastSend = millis();
  }

  if (tempOUT >= storage.tempAlarm) {
#ifdef beep    
    //peep.noDelay(100,40,3,255);
#endif
  }
  /*    if ((tempOUT || tempIN) >= storage.tempAlarm && (tempOUT || tempIN) < storage.tempAlarm+5) {
    peep.noDelay(100,40,3,255);
  } else if ((tempOUT || tempIN) >= storage.tempAlarm+5 && (tempOUT || tempIN) < storage.tempAlarm+10) {
    peep.noDelay(100,40,5,255);
  } else if ((tempOUT || tempIN) >= storage.tempAlarm+10) {
    peep.noDelay(100,40,9,255);
  } */

  display();
#ifdef beep  
  //peep.loop();
#endif  
  keyBoard();
  
  testPumpProtect();
}

/////////////////////////////////////////////   F  U  N  C   ///////////////////////////////////////
void startMeas() {
  // call sensors.requestTemperatures() to issue a global temperature 
  // request to all devices on the bus
  DEBUG_PRINT(F("Requesting temperatures..."));
  sensorsIN.requestTemperatures(); // Send the command to get temperatures
  sensorsOUT.requestTemperatures(); // Send the command to get temperatures
  sensorsUT.requestTemperatures(); // Send the command to get temperatures
  DEBUG_PRINTLN(F("DONE"));
}

void getTemp() {
  //DeviceAddress da;
  tempIN = sensorsIN.getTempCByIndex(0);
  tempOUT = sensorsOUT.getTempCByIndex(0);

  tempUT[0]=sensorsUT.getTempCByIndex(5);   //obyvak vstup
  tempUT[1]=sensorsUT.getTempCByIndex(4);   //obyvak vystup
  tempUT[2]=sensorsUT.getTempCByIndex(3);   //loznice nova vstup
  tempUT[3]=sensorsUT.getTempCByIndex(11);  //loznice nova vystup
  tempUT[4]=sensorsUT.getTempCByIndex(2);   //loznice stara vstup
  tempUT[5]=sensorsUT.getTempCByIndex(9);   //loznice stara vystup 
  tempUT[6]=sensorsUT.getTempCByIndex(1);   //dilna vstup
  tempUT[7]=sensorsUT.getTempCByIndex(0);   //dilna vystup
  tempUT[8]=sensorsUT.getTempCByIndex(8);   //bojler vstup
  tempUT[9]=sensorsUT.getTempCByIndex(6);   //bojler vystup
  tempUT[10]=sensorsUT.getTempCByIndex(7);  //chodba vstup
  tempUT[11]=sensorsUT.getTempCByIndex(10); //chodba vystup
  

  /*
  0 - 28FF60AA9015018E
  1 - 28FFF438901501C0
  2 - 28FF0C90901501ED
  3 - 28FF62DF90150124
  4 - 28FF0AE2901501DE
  5 - 28FF79E2901501FD
  6 - 28FFA568741603F0
  7 - 28FFE5557416044F
  8 - 28FF6D2F7316052D
  9 - 28FFE3D79015013E
  10 - 28FFBB2F00160185
  11 - 28FF07E090150135


  obyvak
  I - 28FF79E2901501FD
  O - 28FF0AE2901501DE

  loznice nova
  I - 28FF62DF90150124
  O - 28FF07E090150135

  loznice stara
  I - 28FF0C90901501ED
  O - 28FFE3D79015013E

  dilna
  I - 28FFF438901501C0
  O - 28FF60AA9015018E

  bojler
  I - 28FF6D2F7316052D
  O - 28FFA568741603F0

  chodba
  I - 28FFE5557416044F
  O - 28FFBB2F00160185
*/

  // for (byte i=0; i<sensorsUT.getDeviceCount(); i++) {

      // tempUT[i]=sensorsUT.getTempCByIndex(i);
    // /*sensorsUT.getAddress(da, i); 
    // printAddress(da);
    // DEBUG_PRINTLN();
    // if (da[7]=="142") {
      // tempUT[4]=sensorsUT.getTempCByIndex(i);
    // } else if (da[7]=="192") {
      // tempUT[5]=sensorsUT.getTempCByIndex(i);
    // } else if(da[7]=="237") {
      // tempUT[6]=sensorsUT.getTempCByIndex(i);
    // } else if (da[7]=="36") {
      // tempUT[2]=sensorsUT.getTempCByIndex(i);
    // } else if (da[7]=="222") {
      // tempUT[1]=sensorsUT.getTempCByIndex(i);
    // } else if (da[7]=="30") {
      // tempUT[0]=sensorsUT.getTempCByIndex(i);
    // } else if (da[7]=="253") {
      // tempUT[7]=sensorsUT.getTempCByIndex(i);
    // } else if (da[7]=="62") {
      // tempUT[8]=sensorsUT.getTempCByIndex(i);
    // } else if (da[7]=="133") {
      // tempUT[3]=sensorsUT.getTempCByIndex(i);
    // } else if (da[7]=="53") {
      // tempUT[9]=sensorsUT.getTempCByIndex(i);
    // }*/
  // }

  firstMeasComplete=true;
}

void tempSetup() {
  MQTT_connect();
  Adafruit_MQTT_Subscribe *subscription;
  while ((subscription = mqtt.readSubscription(5000))) {
    if (subscription == &tONSetup) {
      char *pNew = (char *)tONSetup.lastread;
      uint32_t pPassw=atol(pNew); 
      DEBUG_PRINT(F("Set ON temperature!"));
      storage.tempON = tONSetup;
    }
    if (subscription == &tDiffSetup) {
      char *pNew = (char *)tDiffSetup.lastread;
      uint32_t pPassw=atol(pNew); 
      DEBUG_PRINT(F("Set temperature diference!"));
      storage.tempOFFDiff = tDiffSetup;
    }
  }
}

void printTemp() {
  DEBUG_PRINT(F("Temp IN: "));
  DEBUG_PRINT(tempIN); 
  DEBUG_PRINTLN();
  DEBUG_PRINT(F("Temp OUT: "));
  DEBUG_PRINT(tempOUT); 
  DEBUG_PRINTLN();
  DeviceAddress da;
  
  DEBUG_PRINT(tempUT[0]);
  DEBUG_PRINT(" obyvak vstup - ");
  sensorsUT.getAddress(da, 4); 
  printAddress(da);
  DEBUG_PRINTLN();
  DEBUG_PRINT(tempUT[1]);
  DEBUG_PRINT(" obyvak vystup - ");
  sensorsUT.getAddress(da, 5); 
  printAddress(da);
  DEBUG_PRINTLN();
  DEBUG_PRINT(tempUT[2]);
  DEBUG_PRINT(" loznice nova vstup - ");
  sensorsUT.getAddress(da, 3); 
  printAddress(da);
  DEBUG_PRINTLN();
  DEBUG_PRINT(tempUT[3]);
  DEBUG_PRINT(" loznice nova vystup - ");
  sensorsUT.getAddress(da, 11); 
  printAddress(da);
  DEBUG_PRINTLN();
  DEBUG_PRINT(tempUT[4]);
  DEBUG_PRINT(" loznice stara vstup - ");
  sensorsUT.getAddress(da, 2); 
  printAddress(da);
  DEBUG_PRINTLN();
  DEBUG_PRINT(tempUT[5]);
  DEBUG_PRINT(" loznice stara vystup - ");
  sensorsUT.getAddress(da, 9); 
  printAddress(da);
  DEBUG_PRINTLN();
  DEBUG_PRINT(tempUT[6]);
  DEBUG_PRINT(" dilna vstup - ");
  sensorsUT.getAddress(da, 0); 
  printAddress(da);
  DEBUG_PRINTLN();
  DEBUG_PRINT(tempUT[7]);
  DEBUG_PRINT(" dilna vystup - ");
  sensorsUT.getAddress(da, 1); 
  printAddress(da);
  DEBUG_PRINTLN();
  DEBUG_PRINT(tempUT[8]);
  DEBUG_PRINT(" bojler vstup - ");
  sensorsUT.getAddress(da, 8); 
  printAddress(da);
  DEBUG_PRINTLN();
  DEBUG_PRINT(tempUT[9]);
  DEBUG_PRINT(" bojler vystup - ");
  sensorsUT.getAddress(da, 6); 
  printAddress(da);
  DEBUG_PRINTLN();
  DEBUG_PRINT(tempUT[10]);
  DEBUG_PRINT(" hala vstup - ");
  sensorsUT.getAddress(da, 7); 
  printAddress(da);
  DEBUG_PRINTLN();
  DEBUG_PRINT(tempUT[11]);
  DEBUG_PRINT(" hala vystup - ");
  sensorsUT.getAddress(da, 10); 
  printAddress(da);
  DEBUG_PRINTLN();

 /* for (byte i=0; i<sensorsUT.getDeviceCount(); i++) {
    DEBUG_PRINT(F("Temp UT["));
    DEBUG_PRINT(i);
    DEBUG_PRINT(F("]: "));
    DEBUG_PRINT(tempUT[i]); 
    DEBUG_PRINT(" - ");
    sensorsUT.getAddress(da, i); 
    printAddress(da);
    DEBUG_PRINTLN();
  }*/
}

void sendDataMQTT() {
  MQTT_connect();

  if (! tINKamna.publish(tempINKamna)) {
    DEBUG_PRINTLN("failed");
  } else {
    DEBUG_PRINTLN("OK!");
  }
  if (! tOUTKamna.publish(tempOUTKamna)) {
    DEBUG_PRINTLN("failed");
  } else {
    DEBUG_PRINTLN("OK!");
  }
  if (! sPumpKamna.publish(pumpStatus)) {
    DEBUG_PRINTLN("failed");
  } else {
    DEBUG_PRINTLN("OK!");
  }
  

  if (! t0.publish(temp[0])) {
    DEBUG_PRINTLN("failed");
  } else {
    DEBUG_PRINTLN("OK!");
  }
  if (! t1.publish(temp[1])) {
    DEBUG_PRINTLN("failed");
  } else {
    DEBUG_PRINTLN("OK!");
  }
  if (! t2.publish(temp[2])) {
    DEBUG_PRINTLN("failed");
  } else {
    DEBUG_PRINTLN("OK!");
  }
  if (! t3.publish(temp[3])) {
    DEBUG_PRINTLN("failed");
  } else {
    DEBUG_PRINTLN("OK!");
  }
  if (! t4.publish(temp[4])) {
    DEBUG_PRINTLN("failed");
  } else {
    DEBUG_PRINTLN("OK!");
  }
  if (! t5.publish(temp[5])) {
    DEBUG_PRINTLN("failed");
  } else {
    DEBUG_PRINTLN("OK!");
  }
  if (! t6.publish(temp[6])) {
    DEBUG_PRINTLN("failed");
  } else {
    DEBUG_PRINTLN("OK!");
  }
  if (! t7.publish(temp[7])) {
    DEBUG_PRINTLN("failed");
  } else {
    DEBUG_PRINTLN("OK!");
  }
  if (! t8.publish(temp[8])) {
    DEBUG_PRINTLN("failed");
  } else {
    DEBUG_PRINTLN("OK!");
  }
  if (! t9.publish(temp[9])) {
    DEBUG_PRINTLN("failed");
  } else {
    DEBUG_PRINTLN("OK!");
  }
  if (! t10.publish(temp[10])) {
    DEBUG_PRINTLN("failed");
  } else {
    DEBUG_PRINTLN("OK!");
  }
  if (! t11.publish(temp[11])) {
    DEBUG_PRINTLN("failed");
  } else {
    DEBUG_PRINTLN("OK!");
  }
}


//display
/*
  01234567890123456789
  --------------------
0|xx/xx       15:30:45
1|xx/xx  xx/xx  xx/xx
2|xx/xx  xx/xx  xx/xx
3|xx/xx  xx/xx  xx/xx
  --------------------
  01234567890123456789
*/

void displayTemp() {
  displayTime();
  if (!tempRefresh) {
    return;
  }
  tempRefresh=false;
  lcd.setCursor(0, 0); //col,row
  lcd.print(F("       "));
  lcd.setCursor(0, 0); //col,row
  if (tempIN==TEMP_ERR) {
    displayTempErr();
  }else {
    addSpacesBefore((int)tempIN);
    lcd.print((int)tempIN);
  }
  lcd.setCursor(2, 0); //col,row
  lcd.print(F("/"));
  //addSpaces((int)tempOUT);
  if (tempOUT==TEMP_ERR) {
    displayTempErr();
  }else {
    lcd.print((int)tempOUT);
  }
  
  byte radka=1;
  byte sensor=0;
  byte radiator=1;
  for (byte i=0; i<sensorsUT.getDeviceCount(); i=i+6) {
    displayRadTemp(0, radka, sensor);
    sensor+=2;
    displayRadTemp(7, radka, sensor);
    sensor+=2;
    displayRadTemp(14, radka, sensor);
    sensor+=2;
    radka++;
  }

  lcd.setCursor(8, 0);
  if (relay==HIGH) {
    lcd.print("    ");
  }else{
    lcd.print("CER");
  }
}

void displayRadTemp(byte sl, byte rad, byte s) {
    lcd.setCursor(sl, rad);
    if (tempUT[s]==TEMP_ERR) {
      displayTempErr();
    }else {
      addSpacesBefore((int)tempUT[s]);
      lcd.print((int)tempUT[s]);
    }
    lcd.setCursor(sl+2, rad);
    lcd.print(F("/"));
    s++;
    if (tempUT[s]==TEMP_ERR) {
      displayTempErr();
    }else {
      lcd.print((int)tempUT[s]);    
      addSpacesAfter((int)tempUT[s]);
    }
}

void displayTempErr() {
  lcd.print("ER");
}

void addSpacesBefore(int cislo) {
  //if (cislo<100 && cislo>0) lcd.print(" ");
  if (cislo<10 && cislo>0) lcd.print(" ");
  //if (cislo<=0 && cislo>-10) lcd.print(" ");
}

void addSpacesAfter(int cislo) {
  //if (cislo<100 && cislo>0) lcd.print(" ");
  if (cislo<10 && cislo>0) lcd.print(" ");
  //if (cislo<=0 && cislo>-10) lcd.print(" ");
}


//display time on LCD
void lcd2digits(int number) {
  if (number >= 0 && number < 10) {
    lcd.write('0');
  }
  lcd.print(number);
}

void print2digits(int number) {
  if (number >= 0 && number < 10) {
    Serial.write('0');
  }
  DEBUG_PRINT(number);
}

void displayTime() {
  lcd.setCursor(12, 0); //col,row
  // if (RTC.read(tm)) {
    // lcd2digits(tm.Hour);
    // lcd.write(':');
    // lcd2digits(tm.Minute);
    // lcd.write(':');
    // lcd2digits(tm.Second);
  // } else {
    // lcd.write('        ');
  // }
}

//zabranuje zatuhnuti cerpadla v lete
void testPumpProtect() {
  //if (storage.lastPumpRun
}

// void setTime() {
  // static time_t tLast;
  // time_t t;
  
  // tmElements_t tm;
  // //check for input to set the RTC, minimum length is 12, i.e. yy,m,d,h,m,s
  // if (Serial.available() >= 12) {
      // //note that the tmElements_t Year member is an offset from 1970,
      // //but the RTC wants the last two digits of the calendar year.
      // //use the convenience macros from Time.h to do the conversions.
      // int y = Serial.parseInt();
      // if (y >= 100 && y < 1000)
        // DEBUG_PRINTLN(F("Error: Year must be two digits or four digits!"));
      // else {
        // if (y >= 1000)
          // tm.Year = CalendarYrToTm(y);
        // else    //(y < 100)
          // tm.Year = y2kYearToTm(y);
          // tm.Month = Serial.parseInt();
          // tm.Day = Serial.parseInt();
          // tm.Hour = Serial.parseInt();
          // tm.Minute = Serial.parseInt();
          // tm.Second = Serial.parseInt();
          // t = makeTime(tm);
    // //use the time_t value to ensure correct weekday is set
        // if(RTC.set(t) == 0) { // Success
          // setTime(t);
          // DEBUG_PRINTLN(F("RTC set"));
          // //printDateTime(t);
          // //DEBUG_PRINTLN();
        // }else
          // DEBUG_PRINTLN(F("RTC set failed!"));
          // //dump any extraneous input
          // while (Serial.available() > 0) Serial.read();
      // }
  // }
// }

void keyBoard() {
  char key = keypad.getKey();
  if (key!=NO_KEY) {
    DEBUG_PRINTLN(key);
    lcd.clear();
    if (key=='1') {
      displayVar = 1;
      lcd.clear();
    } else if (key=='2') {
      displayVar = 2;
      lcd.clear();
    } else if (key=='3') {
      displayVar = 3;
      lcd.clear();
    } else if (key=='4') {
      displayVar = 4;
      lcd.clear();
    } else if (key=='5') {
      displayVar = 5;
      lcd.clear();
    } else if (key=='6') {
      displayVar = 60;
      lcd.clear();
    }
    displayVarSub=key;
    
    /*
    Keyboard layout
    -----------
    | 1 2 3 A |
    | 4 5 6 B |
    | 7 8 9 C |
    | * 0 # D |
    -----------
    1 - normal - zobrazeni teplot
    2 - verze FW, teoplota spinaní, diference, teplota alarmu
    3 - nastavení teploty spínání
    A - 
    4 - nastavení teploty diference
    5 - nastavení teploty alarmu
    6 - nastavení hodin
    B - 
    7 - 
    8 - 
    9 - 
    C - 
    * - 
    0 -
    # - 
    D - 
    */
    key = ' ';
  }
}

void display() {
  if (displayVar==1) {
    displayTemp();
  } else if (displayVar==2) {
    displayInfo();
  } else if (displayVar==3) {
    setTempON();
  } else if (displayVar==4) {
    setTempDiff();
  } else if (displayVar==5) {
    setTempAlarm();
  } else if (displayVar>=60 && displayVar<=62) {
    //setClock(displayVar);
  }
}

void displayInfo() {
  lcd.setCursor(0,0);
  lcd.print(F(SW_NAME));
  lcd.print(F(" "));
  lcd.print(F(VERSION));

  lcd.setCursor(0,1);
  lcd.print(F("Temp ON:"));
  lcd.print(storage.tempON);

  lcd.setCursor(0,2);
  lcd.print(F("Temp OFF diff:"));
  lcd.print(storage.tempOFFDiff);
  
  lcd.setCursor(0,3);
  lcd.print(F("Temp alarm:"));
  lcd.print(storage.tempAlarm);
}

void setTempON() {
  showHelpKey();
  lcd.print(F("Temp ON:"));
  if (displayVarSub=='*') {
    storage.tempON++;
  } else if (displayVarSub=='#') {
    storage.tempON--;
  } else if (displayVarSub=='D') {
    saveConfig();
  }
  lcd.print(storage.tempON);
  displayVarSub=' ';
}

void setTempDiff() {
  showHelpKey();
  lcd.print(F("Temp diff:"));
  if (displayVarSub=='*') {
    storage.tempOFFDiff++;
  } else if (displayVarSub=='#') {
    storage.tempOFFDiff--;
  } else if (displayVarSub=='D') {
    saveConfig();
  }
  lcd.print(storage.tempOFFDiff);
  displayVarSub=' ';
}

void setTempAlarm() {
  showHelpKey();
  lcd.print(F("Temp alarm:"));
  if (displayVarSub=='*') {
    storage.tempAlarm++;
  } else if (displayVarSub=='#') {
    storage.tempAlarm--;
  } else if (displayVarSub=='D') {
    saveConfig();
  }
  lcd.print(storage.tempAlarm);
  displayVarSub=' ';
}

void controlRange(uint8_t *pTime, uint8_t min, uint8_t max) {
   if (*pTime<min) {
     *pTime=min;
   }
   if (*pTime>max) {
     *pTime=max;
   }
  
}

// void setClock(byte typ) {
  // if (typ==60) { //hour
    // showHelpKey1();
    // lcd.print(F("Hour:"));
    // if (displayVarSub=='*') {
      // tm.Hour++;
      // controlRange(&tm.Hour, 0, 23);
    // } else if (displayVarSub=='#') {
      // tm.Hour--;
      // controlRange(&tm.Hour, 0, 23);
    // } else if (displayVarSub=='D') {
      // displayVar=61;
    // }
    // displayVarSub=' ';
  // } else if (typ==61) { //min
    // showHelpKey1();
    // lcd.print(F("Min:"));
    // if (displayVarSub=='*') {
      // tm.Minute++;
      // controlRange(&tm.Minute, 0, 59);
    // } else if (displayVarSub=='#') {
      // tm.Minute--;
      // controlRange(&tm.Minute, 0, 59);
    // } else if (displayVarSub=='D') {
      // displayVar=62;
    // }
    // displayVarSub=' ';
  // } else if (typ==62) { //sec
    // showHelpKey();
    // lcd.print(F("Sec:"));
    // if (displayVarSub=='*') {
      // tm.Second++;
      // controlRange(&tm.Second, 0, 59);
    // } else if (displayVarSub=='#') {
      // tm.Second--;
      // controlRange(&tm.Second, 0, 59);
    // } else if (displayVarSub=='D') {
      // RTC.write(tm);
      // displayVar=1;
    // }
    // lcd2digits(tm.Hour);
    // lcd.write(':');
    // lcd2digits(tm.Minute);
    // lcd.write(':');
    // lcd2digits(tm.Second);
    // displayVarSub=' ';
  // }
// }

void loadConfig() {
  
}

void saveConfig() {
  
}

void showHelpKey() {
  lcd.setCursor(0,0);
  lcd.print(F("key * UP "));
  lcd.setCursor(0,1);
  lcd.print(F("key # DOWN "));
  lcd.setCursor(0,2);
  lcd.print(F("key D SAVE "));
  lcd.setCursor(0,3);
}

void showHelpKey1() {
  lcd.setCursor(0,0);
  lcd.print(F("key * UP "));
  lcd.setCursor(0,1);
  lcd.print(F("key # DOWN "));
  lcd.setCursor(0,2);
  lcd.print(F("key D NEXT "));
  lcd.setCursor(0,3);
}

// function to print a device address
void printAddress(DeviceAddress deviceAddress)
{
  for (uint8_t i = 0; i < 8; i++)
  {
    // zero pad the address if necessary
    if (deviceAddress[i] < 16) DEBUG_PRINT("0");
    DEBUG_PRINTHEX(deviceAddress[i]);
  }
}

void printSystemTime(){
  DEBUG_PRINT(day());
  DEBUG_PRINT(".");
  DEBUG_PRINT(month());
  DEBUG_PRINT(".");
  DEBUG_PRINT(year());
  DEBUG_PRINT(" ");
  DEBUG_PRINT(hour());
  printDigits(minute());
  printDigits(second());
}

void printDigits(int digits){
  // utility function for digital clock display: prints preceding
  // colon and leading 0
  DEBUG_PRINT(":");
  if(digits < 10)
    DEBUG_PRINT('0');
  DEBUG_PRINT(digits);
}

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

// Function to connect and reconnect as necessary to the MQTT server.
// Should be called in the loop function and it will take care if connecting.
void MQTT_connect() {
  int8_t ret;

  // Stop if already connected.
  if (mqtt.connected()) {
    return;
  }

  DEBUG_PRINT("Connecting to MQTT... ");

  uint8_t retries = 3;
  while ((ret = mqtt.connect()) != 0) { // connect will return 0 for connected
       DEBUG_PRINTLN(mqtt.connectErrorString(ret));
       DEBUG_PRINTLN("Retrying MQTT connection in 5 seconds...");
       mqtt.disconnect();
       delay(5000);  // wait 5 seconds
       retries--;
       if (retries == 0) {
         // basically die and wait for WDT to reset me
         while (1);
       }
  }
  DEBUG_PRINTLN("MQTT Connected!");
}
