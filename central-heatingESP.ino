/*
--------------------------------------------------------------------------------------------------------------------------
CENTRAL HEATING - control system for central heating
Petr Fory pfory@seznam.cz
GIT - https://github.com/pfory/central-heating
*/

#include "Configuration.h"

byte            sensorOrder[NUMBER_OF_DEVICES];

// Setup a oneWire instance to communicate with any OneWire devices (not just Maxim/Dallas temperature ICs)
OneWire oneWireOUT(ONE_WIRE_BUS_OUT);
OneWire oneWireIN(ONE_WIRE_BUS_IN);
OneWire oneWireUT(ONE_WIRE_BUS_UT);

// Pass our oneWire reference to Dallas Temperature. 
DallasTemperature sensorsOUT(&oneWireOUT);
DallasTemperature sensorsIN(&oneWireIN);
DallasTemperature sensorsUT(&oneWireUT);

DeviceAddress inThermometer, outThermometer;
DeviceAddress utT[NUMBER_OF_DEVICES];
DeviceAddress tempDeviceAddress;

bool firstTempMeasDone                      = false;

float sensor[NUMBER_OF_DEVICES];


float                 tempOUT                     = 0.f;
float                 tempIN                      = 0.f;
float                 tempUT[12];              
bool                  tempRefresh                 = false;
uint32_t              heartBeat                   = 0;
float                 temp[15];              

byte                  relayStatus                 = RELAY_ON;

uint32_t              runSecToday                 = 0;

bool                  todayClear                  = false;
bool                  dispClear                   = false;

//#define SIMTEMP

#ifdef time
WiFiUDP EthernetUdp;
static const char     ntpServerName[]       = "tik.cesnet.cz";
TimeChangeRule        CEST                  = {"CEST", Last, Sun, Mar, 2, 120};     //Central European Summer Time
TimeChangeRule        CET                   = {"CET", Last, Sun, Oct, 3, 60};       //Central European Standard Time
Timezone CE(CEST, CET);
unsigned int          localPort             = 8888;  // local port to listen for UDP packets
time_t getNtpTime();
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
  int             relayType;
} storage = {
  50,
  2,
  90,
  RELAY_TYPE_AUTO
};

LiquidCrystal_I2C lcd(LCDADDRESS,LCDCOLS,LCDROWS);  // set the LCD
#define PRINT_SPACE  lcd.print(" ");
volatile bool showDoubleDot                 = false;
//#define DISPLAY_MAIN                         0

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


DoubleResetDetector drd(DRD_TIMEOUT, DRD_ADDRESS);

ADC_MODE(ADC_VCC); //vcc read

const byte ROWS = 4; //four rows
const byte COLS = 4; //three columns
char keys[ROWS][COLS]                 = {
                                            {'1','2','3','A'},
                                            {'4','5','6','B'},
                                            {'7','8','9','C'},
                                            {'*','0','#','D'}
};
byte rowPins[ROWS]                    = {7,6,5,4}; //connect to the row pinouts of the keypad
byte colPins[COLS]                    = {3,2,1,0}; //connect to the column pinouts of the keypad

//Keypad_I2C keypad = Keypad_I2C( makeKeymap(keys), rowPins, colPins, ROWS, COLS, I2CADDR );
Keypad_I2C keypad( makeKeymap(keys), rowPins, colPins, ROWS, COLS, I2CADDR); 
// #endif

byte displayVar       = 1;
char displayVarSub    = ' ';

//for LED status
Ticker ticker;

auto timer = timer_create_default(); // create a timer with default settings
Timer<> default_timer; // save as above


void tick() {
  //toggle state
  int state = digitalRead(BUILTIN_LED);  // get the current state of GPIO1 pin
  digitalWrite(BUILTIN_LED, !state);     // set pin to the opposite state
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
  
  if (strcmp(topic, (String(mqtt_base) + "/" + String(mqtt_topic_relay_type_set)).c_str())==0) {
    printMessageToLCD(topic, val);
    DEBUG_PRINT("set relay to ");
    storage.relayType = val.toInt();
    if (val.toInt()==0) {
      DEBUG_PRINTLN(F("MANUAL OFF"));
      relayStatus = RELAY_OFF;
      changeRelay(relayStatus);
    } else if (val.toInt()==1) {
      DEBUG_PRINTLN(F("MANUAL ON"));
      relayStatus = RELAY_ON;
      changeRelay(relayStatus);
    } else if (val.toInt()==2) {
      DEBUG_PRINTLN(F("AUTO"));
    }
    saveConfig();
    void * a;
    sendStatisticMQTT(a);
  } else if (strcmp(topic, (String(mqtt_base) + "/" + String(mqtt_topic_restart)).c_str())==0) {
    printMessageToLCD(topic, val);
    DEBUG_PRINT("RESTART");
    ESP.restart();
  } else if (strcmp(topic, (String(mqtt_base) + "/" + String(mqtt_topic_netinfo)).c_str())==0) {
    printMessageToLCD(topic, val);
    DEBUG_PRINT("NET INFO");
    sendNetInfoMQTT();
  } else if (strcmp(topic, mqtt_topic_weather)==0) {
    DEBUG_PRINT("Temperature from Meteo: ");
    DEBUG_PRINTLN(val.toFloat());
    displayValue(TEMPERATURE_X,TEMPERATURE_Y, (int)round(val.toFloat()), 3, 0);
    lcd.write(byte(0));
    lcd.print("C");
  } else if (strcmp(topic, (String(mqtt_base) + "/" + String(mqtt_topic_setTempON)).c_str())==0) {
    printMessageToLCD(topic, val);
    DEBUG_PRINT("Set temperature for ON: ");
    DEBUG_PRINTLN(val.toInt());
    storage.tempON = val.toInt();
    saveConfig();
    void * a;
    sendStatisticMQTT(a);
  } else if (strcmp(topic, (String(mqtt_base) + "/" + String(mqtt_topic_setTempOFFDiff)).c_str())==0) {
    printMessageToLCD(topic, val);
    DEBUG_PRINT("Set temperature diff for OFF: ");
    DEBUG_PRINTLN(val.toInt());
    storage.tempOFFDiff = val.toInt();
    saveConfig();
    void * a;
    sendStatisticMQTT(a);
  } else if (strcmp(topic, (String(mqtt_base) + "/" + String(mqtt_topic_setTempAlarm)).c_str())==0) {
    printMessageToLCD(topic, val);
    DEBUG_PRINT("Set alerm temperature: ");
    DEBUG_PRINTLN(val.toInt());
    storage.tempAlarm = val.toInt();
    saveConfig();
  } else if (strcmp(topic, (String(mqtt_base) + "/" + String(mqtt_topic_sendSO)).c_str())==0) {
    printMessageToLCD(topic, val);
    DEBUG_PRINT("send sensor order");
    void * a;
    sendSOMQTT(a);
  } else if (strcmp(topic, (String(mqtt_base) + "/" + String(mqtt_topic_so0)).c_str())==0) {
    printMessageToLCD(topic, val);
    DEBUG_PRINT("set sensor order 0 to ");
    sensorOrder[0]=val.toInt();
    DEBUG_PRINT(val.toInt());
    saveConfig();  
  } else if (strcmp(topic, (String(mqtt_base) + "/" + String(mqtt_topic_so1)).c_str())==0) {
    printMessageToLCD(topic, val);
    DEBUG_PRINT("set sensor order 1 to ");
    sensorOrder[1]=val.toInt();
    DEBUG_PRINT(val.toInt());
    saveConfig();  
  } else if (strcmp(topic, (String(mqtt_base) + "/" + String(mqtt_topic_so2)).c_str())==0) {
    printMessageToLCD(topic, val);
    DEBUG_PRINT("set sensor order 2 to ");
    sensorOrder[2]=val.toInt();
    DEBUG_PRINT(val.toInt());
    saveConfig();  
  } else if (strcmp(topic, (String(mqtt_base) + "/" + String(mqtt_topic_so3)).c_str())==0) {
    printMessageToLCD(topic, val);
    DEBUG_PRINT("set sensor order 3 to ");
    sensorOrder[3]=val.toInt();
    DEBUG_PRINT(val.toInt());
    saveConfig();  
  } else if (strcmp(topic, (String(mqtt_base) + "/" + String(mqtt_topic_so4)).c_str())==0) {
    printMessageToLCD(topic, val);
    DEBUG_PRINT("set sensor order 4 to ");
    sensorOrder[4]=val.toInt();
    DEBUG_PRINT(val.toInt());
    saveConfig();  
  } else if (strcmp(topic, (String(mqtt_base) + "/" + String(mqtt_topic_so5)).c_str())==0) {
    printMessageToLCD(topic, val);
    DEBUG_PRINT("set sensor order 5 to ");
    sensorOrder[5]=val.toInt();
    DEBUG_PRINT(val.toInt());
    saveConfig();  
  } else if (strcmp(topic, (String(mqtt_base) + "/" + String(mqtt_topic_so6)).c_str())==0) {
    printMessageToLCD(topic, val);
    DEBUG_PRINT("set sensor order 6 to ");
    sensorOrder[6]=val.toInt();
    DEBUG_PRINT(val.toInt());
    saveConfig();  
  } else if (strcmp(topic, (String(mqtt_base) + "/" + String(mqtt_topic_so7)).c_str())==0) {
    printMessageToLCD(topic, val);
    DEBUG_PRINT("set sensor order 7 to ");
    sensorOrder[7]=val.toInt();
    DEBUG_PRINT(val.toInt());
    saveConfig();  
  } else if (strcmp(topic, (String(mqtt_base) + "/" + String(mqtt_topic_so8)).c_str())==0) {
    printMessageToLCD(topic, val);
    DEBUG_PRINT("set sensor order 8 to ");
    sensorOrder[8]=val.toInt();
    DEBUG_PRINT(val.toInt());
    saveConfig();  
  } else if (strcmp(topic, (String(mqtt_base) + "/" + String(mqtt_topic_so9)).c_str())==0) {
    printMessageToLCD(topic, val);
    DEBUG_PRINT("set sensor order 9 to ");
    sensorOrder[9]=val.toInt();
    DEBUG_PRINT(val.toInt());
    saveConfig();  
  } else if (strcmp(topic, (String(mqtt_base) + "/" + String(mqtt_topic_so10)).c_str())==0) {
    printMessageToLCD(topic, val);
    DEBUG_PRINT("set sensor order 10 to ");
    sensorOrder[10]=val.toInt();
    DEBUG_PRINT(val.toInt());
    saveConfig();  
  } else if (strcmp(topic, (String(mqtt_base) + "/" + String(mqtt_topic_so11)).c_str())==0) {
    printMessageToLCD(topic, val);
    DEBUG_PRINT("set sensor order 11 to ");
    sensorOrder[11]=val.toInt();
    DEBUG_PRINT(val.toInt());
    saveConfig();  
  } else if (strcmp(topic, (String(mqtt_base) + "/" + String(mqtt_topic_so12)).c_str())==0) {
    printMessageToLCD(topic, val);
    DEBUG_PRINT("set sensor order 12 to ");
    sensorOrder[12]=val.toInt();
    DEBUG_PRINT(val.toInt());
    saveConfig();  
  }
}

bool isDebugEnabled() {
#ifdef verbose
  return true;
#endif // verbose
  return false;
}

void printMessageToLCD(char* t, String v) {
  lcd.clear();
  lcd.print(t);
  lcd.print(": ");
  lcd.print(v);
  //delay(2000);
  lcd.clear();
}


WiFiClient espClient;
PubSubClient client(espClient);

WiFiManager wifiManager;

/////////////////////////////////////////////   S  E  T  U  P   ////////////////////////////////////
void setup(void) {
  SERIAL_BEGIN;
  DEBUG_PRINT(F(SW_NAME));
  DEBUG_PRINT(F(" "));
  DEBUG_PRINTLN(F(VERSION));

  pinMode(RELAYPIN, OUTPUT);
  digitalWrite(RELAYPIN, relayStatus);
  delay(5000);

  pinMode(BUILTIN_LED, OUTPUT);
  ticker.attach(1, tick);

  WiFi.mode(WIFI_STA); // explicitly set mode, esp defaults to STA+AP

  //set callback that gets called when connecting to previous WiFi fails, and enters Access Point mode
  wifiManager.setAPCallback(configModeCallback);
  wifiManager.setConfigPortalTimeout(CONFIG_PORTAL_TIMEOUT);
  wifiManager.setConnectTimeout(CONNECT_TIMEOUT);

  lcd.init();               // initialize the lcd 
  lcd.backlight();
  lcd.home();
  lcd.print(SW_NAME);
  PRINT_SPACE
  lcd.print(VERSION);
  lcd.createChar(0, customChar);


  if (drd.detectDoubleReset()) {
    DEBUG_PRINTLN("Double reset detected, starting config portal...");
    ticker.attach(0.2, tick);
    if (!wifiManager.startConfigPortal(HOSTNAMEOTA)) {
      DEBUG_PRINTLN("failed to connect and hit timeout");
      delay(3000);
      //reset and try again, or maybe put it to deep sleep
      ESP.reset();
      delay(5000);
    }
  }

  pinMode(ONE_WIRE_BUS_IN, INPUT);
  pinMode(ONE_WIRE_BUS_OUT, INPUT);
  pinMode(ONE_WIRE_BUS_UT, INPUT);

  pinMode(BUZZERPIN, OUTPUT);
#ifdef PIR
  pinMode(PIRPIN, INPUT);
#endif

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

  WiFi.printDiag(Serial);

  //filesystem
  SPIFFS.begin();
  FSInfo fs_info;
  SPIFFS.info(fs_info);
  printf("SPIFFS: %lu of %lu bytes used.\n",
         fs_info.usedBytes, fs_info.totalBytes);
         
  if (!wifiManager.autoConnect(AUTOCONNECTNAME, AUTOCONNECTPWD)) { 
    DEBUG_PRINTLN("failed to connect and hit timeout");
    delay(3000);
    //reset and try again, or maybe put it to deep sleep
    ESP.reset();
    delay(5000);
  } 

  sendNetInfoMQTT();

#ifdef time
  DEBUG_PRINTLN("Setup TIME");
  lcd.setCursor(0,1);
  lcd.print("Setup time...");
  EthernetUdp.begin(localPort);
  DEBUG_PRINT("Local port: ");
  DEBUG_PRINTLN(EthernetUdp.localPort());
  DEBUG_PRINTLN("waiting for sync");
  setSyncProvider(getNtpTime);
  setSyncInterval(300);
  
  printSystemTime();
#endif

#ifdef ota
  ArduinoOTA.setHostname("Central heating");

  ArduinoOTA.onStart([]() {
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
  //tone(BUZZERPIN, 5000, 5);
#endif  
  
  keypad.begin();
  
  readConfig();
  
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
    dsInit();
    
// #ifdef SIMTEMP
    // if (1==0) {
// #else
    if (sensorsIN.getDeviceCount()==0 || sensorsOUT.getDeviceCount()==0) {
// #endif
#ifdef beep
    //peep.Delay(100,40,1,255);
#endif
      DEBUG_PRINTLN(F("NO temperature sensor(s) DS18B20 found!!!!!!!!!"));
      DEBUG_PRINTLN(sensorsIN.getDeviceCount());
      DEBUG_PRINTLN(sensorsOUT.getDeviceCount());
      lcd.setCursor(0, 1);
      lcd.print(F("!NO temp.sensor(s)!!"));
      lcd.setCursor(0, 2);
      lcd.print(F("!!!DS18B20 found!!!!"));
      lcd.setCursor(0, 3);
      lcd.print(F("!!!!!Check wire!!!!!"));
    } else {
      break;
    }
    delay(2000);
  }

  DEBUG_PRINTLN();
  DEBUG_PRINT(F("Sensor(s) "));
  DEBUG_PRINT(sensorsIN.getDeviceCount());
  DEBUG_PRINT(F(" on bus IN - pin "));
  DEBUG_PRINTLN(ONE_WIRE_BUS_IN);
  DEBUG_PRINT(F("Device Address: "));
  printAddress(inThermometer);
  DEBUG_PRINTLN();
  
  lcd.setCursor(0,0);
  lcd.print(F(SW_NAME));
  lcd.print(F(" "));
  lcd.print(F(VERSION));
  
  lcd.setCursor(0,1);
  lcd.print(F("Sen."));
  lcd.print(sensorsIN.getDeviceCount());
  lcd.print(F(" bus IN"));

  DEBUG_PRINT(F("Sensor(s) "));
  DEBUG_PRINT(sensorsOUT.getDeviceCount());
  DEBUG_PRINT(F(" on bus OUT - pin "));
  DEBUG_PRINTLN(ONE_WIRE_BUS_OUT);
  DEBUG_PRINT(F("Device Address: "));
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
  
  delay(3000);
  lcd.clear();
  
  displayInfo();

  delay(3000);
  lcd.clear();

  //setup timers
  timer.every(SEND_DELAY,     sendDataMQTT);
  timer.every(SENDSTAT_DELAY, sendStatisticMQTT);
  timer.every(MEAS_DELAY,     tempMeas);
  timer.every(1000,           calcStat);
#ifdef time  
  timer.every(500,            displayTime);
#endif
  void * a;
  sendStatisticMQTT(a);
  

  ticker.detach();
  //keep LED on
  digitalWrite(BUILTIN_LED, HIGH);

  drd.stop();

  DEBUG_PRINTLN(F("Setup end."));
}

/////////////////////////////////////////////   L  O  O  P   ///////////////////////////////////////
void loop(void) { 
  firstTempMeasDone ? relay() : void();

  timer.tick(); // tick the timer
#ifdef ota
  ArduinoOTA.handle();
#endif

  reconnect();
  client.loop();

  display();

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

#ifdef beep  
  //peep.loop();
#endif  
  keyBoard();
  
#ifdef PIR
  if (digitalRead(PIRPIN)==1) {
    lcd.backlight();
  } else {
    lcd.noBacklight();
  }  
#endif

#ifdef time
  nulStat();
  displayClear();
#endif
  
  testPumpProtect();
}

//---------------------------------------------M A I N  C O N T R O L ------------------------------------------------
void relay() {
  if (tempIN<-100 || tempOUT<-100) {
    relayStatus = RELAY_ON;
    changeRelay(relayStatus);
  } else if (relayStatus == RELAY_OFF && (tempOUT >= SAFETY_ON || tempIN >= SAFETY_ON)) {
    relayStatus = RELAY_ON;
    changeRelay(relayStatus);
  } else if (storage.relayType==RELAY_TYPE_AUTO) {
    //-----------------------------------zmena 0-1--------------------------------------------
    if (relayStatus == RELAY_OFF && (tempOUT >= storage.tempON || tempIN >= storage.tempON)) {
      relayStatus = RELAY_ON;
      changeRelay(relayStatus);
      //lastMillis = millis();
    //-----------------------------------zmena 1-0--------------------------------------------
    } else if (relayStatus == RELAY_ON && tempOUT <= storage.tempON - storage.tempOFFDiff) { 
      relayStatus = RELAY_OFF;
      changeRelay(relayStatus);
    }
  }
  dispRelayStatus();
}

#ifdef time
void nulStat() {
 //nulovani statistik o pulnoci
  if (hour()==0 && !todayClear) {
    todayClear =true;
    runSecToday = 0;
  } else if (hour()>0) {
    todayClear = false;
  }
}

void displayClear() {
  if (minute()==0 && second()==0) {
    if (!dispClear) { 
      lcd.clear();
      dispClear = true;
    }
  } else {
    dispClear = false;
  }
}
#endif

void changeRelay(byte status) {
  digitalWrite(RELAYPIN, status);
  sendRelayMQTT(status);
}

void sendRelayMQTT(byte akce) {
  digitalWrite(BUILTIN_LED, LOW);
  SenderClass sender;
  sender.add("relayChange", akce);
 
  sender.sendMQTT(mqtt_server, mqtt_port, mqtt_username, mqtt_key, mqtt_base);
  
  digitalWrite(BUILTIN_LED, HIGH);
}


void dispRelayStatus() {
  lcd.setCursor(RELAY_STATUSX,RELAY_STATUSY);
  if (relayStatus==RELAY_ON) {
    lcd.print(" ON");
  } else if (relayStatus==RELAY_OFF) {
    lcd.print("OFF");
  } else if (storage.relayType==RELAY_TYPE_MANUAL_ON) {
    lcd.print("MON");
  } else if (storage.relayType==RELAY_TYPE_MANUAL_OFF) {
    lcd.print("MOF");
  }
}


bool tempMeas(void *) {
  tempRefresh = true;
  DEBUG_PRINT(F("Requesting temperatures..."));
  sensorsIN.requestTemperatures(); // Send the command to get temperatures
  sensorsOUT.requestTemperatures(); // Send the command to get temperatures
  sensorsUT.requestTemperatures(); // Send the command to get temperatures
  DEBUG_PRINTLN(F("DONE"));
  
  for (byte i=0;i<sensorsUT.getDeviceCount(); i++) {
    float tempTemp=(float)TEMP_ERR;
    for (byte j=0;j<10;j++) { //try to read temperature ten times
      tempTemp = sensorsUT.getTempC(utT[i]);
      if (tempTemp>=-55) {
        break;
      }
    }

    // DEBUG_PRINTLN(tempTemp);
    tempUT[0] = sensor[sensorOrder[0]];
  }
  
  float tempTemp=(float)TEMP_ERR;
  for (byte j=0;j<10;j++) { //try to read temperature ten times
    tempTemp = sensorsIN.getTempC(inThermometer);
    if (tempTemp>=-55) {
      break;
    }
  }
  tempIN = tempTemp;
  
  tempTemp=(float)TEMP_ERR;
  for (byte j=0;j<10;j++) { //try to read temperature ten times
    tempTemp = sensorsOUT.getTempC(outThermometer);
    if (tempTemp>=-55) {
      break;
    }
  }
  tempOUT = tempTemp;
  
  //printTemp();

  DEBUG_PRINTLN(tempIN);
  DEBUG_PRINTLN(tempOUT);

/*  tempUT[0]=sensorsUT.getTempCByIndex(5);   //obyvak vstup
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
*/

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

  //obcas se vyskytne chyba a vsechna cidla prestanou merit
  //zkusim restartovat sbernici
  bool reset=false;
  for (byte i=0; i<sensorsUT.getDeviceCount(); i++) {
    //if (sensor[i]==0.0 || sensor[i]<-100.0) {
    if (tempUT[i]<-100.0) {
      reset=true;
    }
  }
  if (tempIN<-100 || tempOUT<-100) {
    reset= true;
  }
  
  if (reset) {
    dsInit();
  } else {
    firstTempMeasDone = true;
  }
  
  return true;
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

bool sendDataMQTT(void *) {
  digitalWrite(BUILTIN_LED, LOW);
  SenderClass sender;
  DEBUG_PRINTLN(F("Data"));

  sender.add("tINKamna",            tempIN);
  sender.add("tOUTKamna",           tempOUT);
  sender.add("sPumpKamna/status",   relayStatus==RELAY_ON ? 1 : 0);
  sender.add("t0",                  tempUT[0]);
  sender.add("t1",                  tempUT[1]);
  sender.add("t2",                  tempUT[2]);
  sender.add("t3",                  tempUT[3]);
  sender.add("t4",                  tempUT[4]);
  sender.add("t5",                  tempUT[5]);
  sender.add("t6",                  tempUT[6]);
  sender.add("t7",                  tempUT[7]);
  sender.add("t8",                  tempUT[8]);
  sender.add("t9",                  tempUT[9]);
  sender.add("t10",                 tempUT[10]);
  sender.add("t11",                 tempUT[11]);

  sender.sendMQTT(mqtt_server, mqtt_port, mqtt_username, mqtt_key, mqtt_base);

  digitalWrite(BUILTIN_LED, HIGH);
  return true;
}

bool sendStatisticMQTT(void *) {
  digitalWrite(BUILTIN_LED, LOW);
  //printSystemTime();
  DEBUG_PRINTLN(F("Statistic"));

  SenderClass sender;
  sender.add("VersionSWCentral",              VERSION);
  sender.add("Napeti",  ESP.getVcc());
  sender.add("HeartBeat",                     heartBeat++);
  if (heartBeat % 10 == 0) sender.add("RSSI", WiFi.RSSI());
  sender.add(String("tempON"),                storage.tempON);
  sender.add(String("tempOFFDiff"),           storage.tempOFFDiff);
  sender.add(String("relayType"),             storage.relayType);
  
  DEBUG_PRINTLN(F("Calling MQTT"));
  
  sender.sendMQTT(mqtt_server, mqtt_port, mqtt_username, mqtt_key, mqtt_base);
  digitalWrite(BUILTIN_LED, HIGH);
  return true;
}


void sendNetInfoMQTT() {
  digitalWrite(BUILTIN_LED, LOW);
  //printSystemTime();
  DEBUG_PRINTLN(F("Net info"));

  SenderClass sender;
  sender.add("IP",              WiFi.localIP().toString().c_str());
  sender.add("MAC",             WiFi.macAddress());
  
  DEBUG_PRINTLN(F("Calling MQTT"));
  
  sender.sendMQTT(mqtt_server, mqtt_port, mqtt_username, mqtt_key, mqtt_base);
  digitalWrite(BUILTIN_LED, HIGH);
  return;
}



//---------------------------------------------D I S P L A Y ------------------------------------------------
/*
  01234567890123456789
  --------------------
0|56/66 CER      40/45
1|                    
2|                    
3|19:20 15°C     1440m
  --------------------
  01234567890123456789  
  
  
*/
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

void displayTemp() {
  if (!tempRefresh) {
    return;
  }
  tempRefresh=false;
  if (tempIN==TEMP_ERR) {
    displayTempErr(TEMPINX,TEMPINY);
  }else {
    displayValue(TEMPINX,TEMPINY, (int)tempIN, 2, 0);
  }
  lcd.print(F("/"));
  if (tempOUT==TEMP_ERR) {
    displayTempErr(TEMPOUTX,TEMPOUTY);
  }else {
    displayValue(TEMPOUTX,TEMPOUTY, (int)tempOUT, 2, 0);
  }
  
  displayValue(TEMPSETOFFX,TEMPSETOFFY, (int)(storage.tempOFFDiff), 2, 0);
  lcd.print(F("/"));
  displayValue(TEMPSETONX,TEMPSETONY, (int)(storage.tempON), 2, 0);
  
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
  
  //statistics of pump run
  displayValue(RUNMINTODAY_X,RUNMINTODAY_Y, (int)(runSecToday / 60), 4, 0);
  lcd.print(MIN_UNIT);
}

// void displayRadTemp(byte sl, byte rad, byte s) {
    // lcd.setCursor(sl, rad);
    // if (tempUT[s]==TEMP_ERR) {
      // displayTempErr();
    // }else {
      // addSpacesBefore((int)tempUT[s]);
      // lcd.print((int)tempUT[s]);
    // }
    // lcd.setCursor(sl+2, rad);
    // lcd.print(F("/"));
    // s++;
    // if (tempUT[s]==TEMP_ERR) {
      // displayTempErr();
    // }else {
      // lcd.print((int)tempUT[s]);    
      // addSpacesAfter((int)tempUT[s]);
    // }
// }


void displayTempErr(int x, int y) {
  lcd.setCursor(x,y);
  lcd.print("--");
}

// void addSpacesBefore(int cislo) {
  // //if (cislo<100 && cislo>0) lcd.print(" ");
  // if (cislo<10 && cislo>0) lcd.print(" ");
  // //if (cislo<=0 && cislo>-10) lcd.print(" ");
// }

// void addSpacesAfter(int cislo) {
  // //if (cislo<100 && cislo>0) lcd.print(" ");
  // if (cislo<10 && cislo>0) lcd.print(" ");
  // //if (cislo<=0 && cislo>-10) lcd.print(" ");
// }


//zabranuje zatuhnuti cerpadla v lete
void testPumpProtect() {
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

// void controlRange(uint8_t *pTime, uint8_t min, uint8_t max) {
   // if (*pTime<min) {
     // *pTime=min;
   // }
   // if (*pTime>max) {
     // *pTime=max;
   // }
  
// }


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

void printSystemTime(){
  char buffer[20];
  sprintf(buffer, "%02d.%02d.%4d %02d:%02d:%02d", day(), month(), year(), hour(), minute(), second());
  DEBUG_PRINT(buffer);
}

bool displayTime(void *) {
  lcd.setCursor(TIMEX, TIMEY); //col,row
  char buffer[6];
  Wire.beginTransmission(8);
  Wire.write((byte)tempOUT);
  Wire.endTransmission();
  if (showDoubleDot) {
    sprintf(buffer, "%02d:%02d", hour(), minute());
  } else {
    Wire.write(byte(0));
    sprintf(buffer, "%02d %02d", hour(), minute());
  }
  lcd.print(buffer);
  showDoubleDot = !showDoubleDot;
  return true;
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
#endif

void reconnect() {
  // Loop until we're reconnected
  if (!client.connected()) {
    if (lastConnectAttempt == 0 || lastConnectAttempt + connectDelay < millis()) {
      DEBUG_PRINT("Attempting MQTT connection...");
      // Attempt to connect
      if (client.connect(mqtt_base, mqtt_username, mqtt_key)) {
        DEBUG_PRINTLN("connected");
        // Once connected, publish an announcement...
        client.subscribe((String(mqtt_base) + "/#").c_str());
        client.subscribe(mqtt_topic_weather);
      } else {
        lastConnectAttempt = millis();
        DEBUG_PRINT("failed, rc=");
        DEBUG_PRINTLN(client.state());
      }
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
  strcat(format, "d");

  sprintf (buffer, format, (int)value); // send data to the buffer
  lcd.setCursor(x,y);
  lcd.print(buffer); // display line on buffer

  if (des>0) {
    lcd.print(F("."));
    lcd.print(abs((int)(value*(10*des))%(10*des)));
  }
}

// void PIREvent() {
  // if (digitalRead(PIRPIN)==1) {
    // DEBUG_PRINTLN("DISPLAY_ON");
    // lcd.backlight();
  // } else {
    // DEBUG_PRINTLN("DISPLAY OFF");
    // lcd.noBacklight();
  // }
// }


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

  StaticJsonDocument<1024> doc;

  doc["tempON"]                   = storage.tempON;
  doc["tempOFFDiff"]              = storage.tempOFFDiff;
  doc["tempAlarm"]                = storage.tempAlarm;
  doc["relayType"]                = storage.relayType;
  doc["sensorOrder[0]"]           = sensorOrder[0];
  doc["sensorOrder[1]"]           = sensorOrder[1];
  doc["sensorOrder[2]"]           = sensorOrder[2];
  doc["sensorOrder[3]"]           = sensorOrder[3];
  doc["sensorOrder[4]"]           = sensorOrder[4];
  doc["sensorOrder[5]"]           = sensorOrder[5];
  doc["sensorOrder[6]"]           = sensorOrder[6];
  doc["sensorOrder[7]"]           = sensorOrder[7];
  doc["sensorOrder[8]"]           = sensorOrder[8];
  doc["sensorOrder[9]"]           = sensorOrder[9];
  doc["sensorOrder[10]"]          = sensorOrder[10];
  doc["sensorOrder[11]"]          = sensorOrder[11];
  doc["sensorOrder[12]"]          = sensorOrder[12];
  File configFile = SPIFFS.open(CFGFILE, "w+");
  if (!configFile) {
    DEBUG_PRINTLN(F("Failed to open config file for writing"));
    SPIFFS.end();
    return false;
  } else {
    if (isDebugEnabled) {
      serializeJson(doc, Serial);
    }
    serializeJson(doc, configFile);
    configFile.close();
    SPIFFS.end();
    DEBUG_PRINTLN(F("\nSaved successfully"));
    return true;
  }
}

bool readConfig() {
  DEBUG_PRINTLN();
  DEBUG_PRINT(F("Mounting FS..."));
  if (SPIFFS.begin()) {
    DEBUG_PRINTLN(F(" mounted!"));
    if (SPIFFS.exists(CFGFILE)) {
      // file exists, reading and loading
      DEBUG_PRINTLN(F("Reading config file"));
      File configFile = SPIFFS.open(CFGFILE, "r");
      if (configFile) {
        DEBUG_PRINTLN(F("Opened config file"));

        char json[500];
        while (configFile.available()) {
         int l = configFile.readBytesUntil('\n', json, sizeof(json));
         json[l] = 0;
         DEBUG_PRINTLN(json);
        }
        
        DynamicJsonDocument doc(1024);
        deserializeJson(doc, json);

        storage.tempON        = doc["tempON"];
        storage.tempOFFDiff   = doc["tempOFFDiff"];
        storage.tempAlarm     = doc["tempAlarm"];
        storage.relayType     = doc["relayType"];
        
        sensorOrder[0] = doc["sensorOrder[0]"];
        DEBUG_PRINT(F("sensorOrder[0]: "));
        DEBUG_PRINTLN(sensorOrder[0]);
        sensorOrder[1] = doc["sensorOrder[1]"];
        DEBUG_PRINT(F("sensorOrder[1]: "));
        DEBUG_PRINTLN(sensorOrder[1]);
        sensorOrder[2] = doc["sensorOrder[2]"];
        DEBUG_PRINT(F("sensorOrder[2]: "));
        DEBUG_PRINTLN(sensorOrder[2]);
        sensorOrder[3] = doc["sensorOrder[3]"];
        DEBUG_PRINT(F("sensorOrder[3]: "));
        DEBUG_PRINTLN(sensorOrder[3]);
        sensorOrder[4] = doc["sensorOrder[4]"];
        DEBUG_PRINT(F("sensorOrder[4]: "));
        DEBUG_PRINTLN(sensorOrder[4]);
        sensorOrder[5] = doc["sensorOrder[5]"];
        DEBUG_PRINT(F("sensorOrder[5]: "));
        DEBUG_PRINTLN(sensorOrder[5]);
        sensorOrder[6] = doc["sensorOrder[6]"];
        DEBUG_PRINT(F("sensorOrder[6]: "));
        DEBUG_PRINTLN(sensorOrder[6]);
        sensorOrder[7] = doc["sensorOrder[7]"];
        DEBUG_PRINT(F("sensorOrder[7]: "));
        DEBUG_PRINTLN(sensorOrder[7]);
        sensorOrder[8] = doc["sensorOrder[8]"];
        DEBUG_PRINT(F("sensorOrder[8]: "));
        DEBUG_PRINTLN(sensorOrder[8]);
        sensorOrder[9] = doc["sensorOrder[9]"];
        DEBUG_PRINT(F("sensorOrder[9]: "));
        DEBUG_PRINTLN(sensorOrder[9]);
        sensorOrder[10] = doc["sensorOrder[10]"];
        DEBUG_PRINT(F("sensorOrder[10]: "));
        DEBUG_PRINTLN(sensorOrder[10]);
        sensorOrder[11] = doc["sensorOrder[11]"];
        DEBUG_PRINT(F("sensorOrder[11]: "));
        DEBUG_PRINTLN(sensorOrder[11]);
        sensorOrder[12] = doc["sensorOrder[12]"];
        DEBUG_PRINT(F("sensorOrder[12]: "));
        DEBUG_PRINTLN(sensorOrder[12]);

        return true;
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

bool sendSOMQTT(void *) {
  digitalWrite(BUILTIN_LED, LOW);
#ifdef time
  printSystemTime();
#endif
  DEBUG_PRINTLN(F("Sensor order"));
  SenderClass sender;
  sender.add("so0", sensorOrder[0]);
  sender.add("so1", sensorOrder[1]);
  sender.add("so2", sensorOrder[2]);
  sender.add("so3", sensorOrder[3]);
  sender.add("so4", sensorOrder[4]);
  sender.add("so5", sensorOrder[5]);
  sender.add("so6", sensorOrder[6]);
  sender.add("so7", sensorOrder[7]);
  sender.add("so8", sensorOrder[8]);
  sender.add("so9", sensorOrder[9]);
  sender.add("so10", sensorOrder[10]);
  sender.add("so11", sensorOrder[11]);
  sender.add("so11", sensorOrder[12]);

  //noInterrupts();
  sender.sendMQTT(mqtt_server, mqtt_port, mqtt_username, mqtt_key, mqtt_base);
  //interrupts();
  digitalWrite(BUILTIN_LED, HIGH);
  return true;
}

bool calcStat(void *) {  //run each second from timer
  if (relayStatus == RELAY_ON) {
    runSecToday ++;
  }
  return true;
}

void dsInit(void) {
  sensorsOUT.begin(); 
  sensorsIN.begin(); 
  sensorsUT.begin(); 
  
  if (sensorsUT.getDeviceCount()==1) {
    DEBUG_PRINTLN(" sensor found");
  } else {
    DEBUG_PRINTLN(" sensor(s) found");
  }

  sensorsIN.getAddress(inThermometer, 0); 
  sensorsOUT.getAddress(outThermometer, 0); 

  // Loop through each device, print out address
  for (byte i=0;i<sensorsUT.getDeviceCount(); i++) {
      // Search the wire for address
    if (sensorsUT.getAddress(tempDeviceAddress, i)) {
      memcpy(utT[i],tempDeviceAddress,8);
    }
  }
  sensorsIN.setResolution(TEMPERATURE_PRECISION);
  sensorsOUT.setResolution(TEMPERATURE_PRECISION);
  sensorsUT.setResolution(TEMPERATURE_PRECISION);
  
  sensorsIN.setWaitForConversion(false);
  sensorsOUT.setWaitForConversion(false);
  sensorsUT.setWaitForConversion(false);

  //lcd.clear();
}