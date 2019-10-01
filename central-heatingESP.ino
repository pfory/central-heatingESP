/*
--------------------------------------------------------------------------------------------------------------------------
CENTRAL HEATING - control system for central heating
Petr Fory pfory@seznam.cz
GIT - https://github.com/pfory/central-heating
*/

#include "Configuration.h"

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

//const unsigned long   measTime                   = 750; //in ms
///bool                  startConversion            = false;
//unsigned long         startConversionMillis      = 0;
float                 tempOUT                    = 0.f;
float                 tempIN                     = 0.f;
float                 tempUT[12];             
//bool                  relay                      = HIGH;
const unsigned long   pumpProtect                = 864000000;  //1000*60*60*24*10; //in ms = 10 day, max 49 days
const unsigned long   pumpProtectRun             = 300000;     //1000*60*5;     //in ms = 5 min
bool                  firstMeasComplete          = false;
bool                  tempRefresh                = false;
uint32_t              heartBeat                  = 0;
float                 temp[15];
int                   pumpStatus                 = 0;

byte relayStatus                                 = RELAY_OFF;
byte manualRelay                                 = 2;


#define SIMTEMP

#define time
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
#define TIMEX   0
#define TIMEY   3
#endif

#define beep
// #ifdef beep
// #include "beep.h"
// Beep peep(BUZZERPIN);
// #endif

struct StoreStruct {
  // The variables of your settings
  float           tempON;
  float           tempOFFDiff;
  float           tempAlarm;
} storage = {
  70,
  5,
  95
};

#include <LiquidCrystal_I2C.h>
LiquidCrystal_I2C lcd(LCDADDRESS,LCDCOLS,LCDROWS);  // set the LCD
#define PRINT_SPACE  lcd.print(" ");
volatile bool showDoubleDot                 = false;
#define DISPLAY_MAIN                         0

//navrhar - https://maxpromer.github.io/LCD-Character-Creator/
byte customChar[] = {
  B01110,
  B01010,
  B01110,
  B00000,
  B00000,
  B00000,
  B00000,
  B00000
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

#include <timer.h>
auto timer = timer_create_default(); // create a timer with default settings
Timer<> default_timer; // save as above


void tick()
{
  //toggle state
  int state = digitalRead(BUILTIN_LED);  // get the current state of GPIO1 pin
  digitalWrite(BUILTIN_LED, !state);     // set pin to the opposite state
  digitalWrite(LED1PIN, state);     // set pin to the opposite state
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
  
  if (strcmp(topic, "/home/Corridor/esp07/manualRelay")==0) {
    printMessageToLCD(topic, val);
    DEBUG_PRINT("set manual control relay to ");
    manualRelay = val.toInt();
    if (val.toInt()==1) {
      DEBUG_PRINTLN(F("ON"));
    } else {
      DEBUG_PRINTLN(F("OFF"));
    }
  } else if (strcmp(topic, "/home/Corridor/esp07/restart")==0) {
    printMessageToLCD(topic, val);
    DEBUG_PRINT("RESTART");
    ESP.restart();
  } else if (strcmp(topic, mqtt_topic_weather)==0) {
      DEBUG_PRINT("Temperature from Meteo: ");
      DEBUG_PRINTLN(val.toFloat());
      displayValue(TEMPERATURE_X,TEMPERATURE_Y, (int)round(val.toFloat()), 3, 0);
      lcd.write(byte(0));
      lcd.print("C");
  }

}

void printMessageToLCD(char* t, String v) {
  lcd.clear();
  lcd.print(t);
  lcd.print(": ");
  lcd.print(v);
  delay(2000);
  lcd.clear();
}


WiFiClient espClient;
PubSubClient client(espClient);

/////////////////////////////////////////////   S  E  T  U  P   ////////////////////////////////////
void setup(void) {
  SERIAL_BEGIN;
  DEBUG_PRINT(F(SW_NAME));
  DEBUG_PRINT(F(" "));
  DEBUG_PRINTLN(F(VERSION));
  
  lcd.init();               // initialize the lcd 
  lcd.backlight();
  lcd.home();
  lcd.print(SW_NAME);
  PRINT_SPACE
  lcd.print(VERSION);

  pinMode(ONE_WIRE_BUS_IN, INPUT);
  pinMode(ONE_WIRE_BUS_OUT, INPUT);
  pinMode(ONE_WIRE_BUS_UT, INPUT);

  pinMode(RELAYPIN, OUTPUT);
  pinMode(LED1PIN, OUTPUT);
  pinMode(BUILTIN_LED, OUTPUT);
  pinMode(BUZZERPIN, OUTPUT);
  pinMode(PIRPIN, INPUT);
  digitalWrite(RELAYPIN, LOW);

  rst_info *_reset_info = ESP.getResetInfoPtr();
  uint8_t _reset_reason = _reset_info->reason;
  DEBUG_PRINT("Boot-Mode: ");
  DEBUG_PRINTLN(_reset_reason);
  heartBeat = _reset_reason;
  
  /*
 REASON_DEFAULT_RST             = 0      normal startup by power on 
 REASON_WDT_RST                 = 1      hardware watch dog reset 
 REASON_EXCEPTION_RST           = 2      exception reset, GPIO status won't change 
 REASON_SOFT_WDT_RST            = 3      software watch dog reset, GPIO status won't change 
 REASON_SOFT_RESTART            = 4      software restart ,system_restart , GPIO status won't change 
 REASON_DEEP_SLEEP_AWAKE        = 5      wake up from deep-sleep 
 REASON_EXT_SYS_RST             = 6      external system reset 
  */
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);

  ticker.attach(1, tick);

  //WiFiManager
  //Local intialization. Once its business is done, there is no need to keep it around
  WiFiManager wifiManager;
  //reset settings - for testing
  //wifiManager.resetSettings();
  //wifiManager.setConnectTimeout(60); //5min

  IPAddress _ip,_gw,_sn;
  _ip.fromString(static_ip);
  _gw.fromString(static_gw);
  _sn.fromString(static_sn);

  wifiManager.setSTAStaticIPConfig(_ip, _gw, _sn);
  
  DEBUG_PRINTLN(_ip);
  DEBUG_PRINTLN(_gw);
  DEBUG_PRINTLN(_sn);

  //set callback that gets called when connecting to previous WiFi fails, and enters Access Point mode
  wifiManager.setAPCallback(configModeCallback);
  
  //DEBUG_PRINTLN(ESP.getFlashChipRealSize);
  //DEBUG_PRINTLN(ESP.getCpuFreqMHz);
  //WiFi.begin(ssid, password);
  
  if (!wifiManager.autoConnect(AUTOCONNECTNAME, AUTOCONNECTPWD)) { 
    DEBUG_PRINTLN("failed to connect and hit timeout");
    delay(3000);
    //reset and try again, or maybe put it to deep sleep
    ESP.reset();
    delay(5000);
  } 

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
#endif


#ifdef beep
  //peep.Delay(100,40,1,255);
  tone(BUZZERPIN, 5000, 5);
#endif  
  
  keypad.begin();
  
  loadConfig();
  DEBUG_PRINTLN();
  DEBUG_PRINT(F("Temp ON "));
  DEBUG_PRINTLN(storage.tempON);
  DEBUG_PRINT(F("Temp OFF diff "));
  DEBUG_PRINTLN(storage.tempOFFDiff);
  DEBUG_PRINT(F("Temp alarm "));
  DEBUG_PRINTLN(storage.tempAlarm);
  delay(5000);
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
  
  delay(1000);
  lcd.clear();
  
  displayInfo();

  delay(1000);
  lcd.clear();

  //setup timers
  timer.every(SEND_DELAY,     sendDataHA);
  timer.every(SENDSTAT_DELAY, sendStatisticHA);
  timer.every(MEAS_DELAY,     readTemp);
#ifdef time  
  timer.every(500, displayTime);
#endif
  void * a;
  sendStatisticHA(a);
  

  ticker.detach();
  //keep LED on
  digitalWrite(BUILTIN_LED, HIGH);
  digitalWrite(LED1PIN, LOW);
  
  DEBUG_PRINTLN(F("Setup end."));
}

/////////////////////////////////////////////   L  O  O  P   ///////////////////////////////////////
void loop(void) { 
  timer.tick(); // tick the timer
#ifdef ota
  ArduinoOTA.handle();
#endif

  if (digitalRead(PIRPIN)==1) {
    lcd.backlight();
  } else {
    lcd.noBacklight();
  }
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  relay();

  Wire.beginTransmission(8);
  Wire.write((byte)tempOUT);
  Wire.endTransmission();

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
void relay() {
  if (manualRelay==2) {
    //-----------------------------------zmena 0-1--------------------------------------------
    if (relayStatus == RELAY_OFF && (tempOUT >= storage.tempON || tempIN >= storage.tempON)) {
      relayStatus = RELAY_ON;
      changeRelay(relayStatus);
      sendRelayHA(1);
    //-----------------------------------zmena 1-0--------------------------------------------
    } else if (relayStatus == RELAY_ON && tempOUT <= storage.tempON - storage.tempOFFDiff) { 
      relayStatus = RELAY_OFF;
      changeRelay(relayStatus);
      sendRelayHA(0);
    }
  } else if (manualRelay==1) {
      relayStatus = RELAY_ON;
      changeRelay(relayStatus);
  } else if (manualRelay==0) {
      relayStatus = RELAY_OFF;
      changeRelay(relayStatus);
  }
  dispRelayStatus();
}


void changeRelay(byte status) {
  digitalWrite(RELAYPIN, status);
}

void sendRelayHA(byte akce) {
  digitalWrite(LED1PIN, LOW);
  digitalWrite(BUILTIN_LED, LOW);
  SenderClass sender;
  sender.add("relayChange", akce);
 
  sender.sendMQTT(mqtt_server, mqtt_port, mqtt_username, mqtt_key, mqtt_base);
  digitalWrite(LED1PIN, HIGH);
  digitalWrite(BUILTIN_LED, HIGH);
}


void dispRelayStatus() {
  lcd.setCursor(RELAY_STATUSX,RELAY_STATUSY);
  if (relayStatus==1) {
    lcd.print(" ON");
  } else if (relayStatus==0) {
    lcd.print("OFF");
  } else if (manualRelay==1) {
    lcd.print("MON");
  } else if (manualRelay==0) {
    lcd.print("MOF");
  }
}


bool readTemp(void *) {
  startMeas(); 
  getTemp();
  //printTemp();
  return true;
}


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

// void tempSetup() {
  // MQTT_connect();
  // Adafruit_MQTT_Subscribe *subscription;
  // while ((subscription = mqtt.readSubscription(5000))) {
    // if (subscription == &tONSetup) {
      // char *pNew = (char *)tONSetup.lastread;
      // uint32_t pPassw=atol(pNew); 
      // DEBUG_PRINT(F("Set ON temperature!"));
      // //storage.tempON = tONSetup;
    // }
    // if (subscription == &tDiffSetup) {
      // char *pNew = (char *)tDiffSetup.lastread;
      // uint32_t pPassw=atol(pNew); 
      // DEBUG_PRINT(F("Set temperature diference!"));
      // //storage.tempOFFDiff = tDiffSetup;
    // }
  // }
// }

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

bool sendDataHA(void *) {
  digitalWrite(LED1PIN, HIGH);
  digitalWrite(BUILTIN_LED, LOW);
  SenderClass sender;
  DEBUG_PRINTLN(F(" - I am sending data to HA"));

  sender.add("tINKamna",        tempIN);
  sender.add("tOUTKamna",       tempOUT);
  sender.add("sPumpKamna",      pumpStatus);
  sender.add("t0",              tempUT[0]);
  sender.add("t1",              tempUT[1]);
  sender.add("t2",              tempUT[2]);
  sender.add("t3",              tempUT[3]);
  sender.add("t4",              tempUT[4]);
  sender.add("t5",              tempUT[5]);
  sender.add("t6",              tempUT[6]);
  sender.add("t7",              tempUT[7]);
  sender.add("t8",              tempUT[8]);
  sender.add("t9",              tempUT[9]);
  sender.add("t10",             tempUT[10]);
  sender.add("t11",             tempUT[11]);

  sender.sendMQTT(mqtt_server, mqtt_port, mqtt_username, mqtt_key, mqtt_base);

  digitalWrite(LED1PIN, LOW);
  digitalWrite(BUILTIN_LED, HIGH);
  return true;
}

bool sendStatisticHA(void *) {
  digitalWrite(LED1PIN, HIGH);
  digitalWrite(BUILTIN_LED, LOW);
  //printSystemTime();
  DEBUG_PRINTLN(F(" - I am sending statistic to HA"));

  SenderClass sender;
  sender.add("VersionSWCentral", VERSION);
  sender.add("HeartBeat", heartBeat++);
  sender.add("RSSI", WiFi.RSSI());
  
  DEBUG_PRINTLN(F("Calling MQTT"));
  
  sender.sendMQTT(mqtt_server, mqtt_port, mqtt_username, mqtt_key, mqtt_base);
  digitalWrite(LED1PIN, LOW);
  digitalWrite(BUILTIN_LED, HIGH);
  return true;
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
  
/*  byte radka=1;
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
*/
  lcd.setCursor(8, 0);
  if (relayStatus==HIGH) {
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

bool displayTime(void *) {
  lcd.setCursor(TIMEX, TIMEY); //col,row
  char buffer[6];
  if (showDoubleDot) {
    sprintf(buffer, "%02d:%02d", hour(), minute());
  } else {
    sprintf(buffer, "%02d %02d", hour(), minute());
  }
  lcd.print(buffer);
  return true;
}
#endif


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
      //client.subscribe((String(mqtt_base) + "/" + "manualRelay").c_str());
      //client.subscribe((String(mqtt_base) + "/" + "restart").c_str());
      //client.subscribe(mqtt_topic_weather);
    } else {
      DEBUG_PRINT("failed, rc=");
      DEBUG_PRINT(client.state());
      DEBUG_PRINTLN(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void displayValue(int x, int y, float value, byte cela, byte des) {
  char buffer [18];
  if (des==0) {
    value = round(value);
  }
 
  char format[5];
  char cislo[2];
  itoa (cela, cislo, 10);
  strcpy(format, "%");
  strcat(format, cislo);
  strcat(format, "d\n");

  sprintf (buffer, format, (int)value); // send data to the buffer
  lcd.setCursor(x,y);
  lcd.print(buffer); // display line on buffer

  if (des>0) {
    lcd.print(F("."));
    lcd.print(abs((int)(value*(10*des))%(10*des)));
  }
}