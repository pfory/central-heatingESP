/*
--------------------------------------------------------------------------------------------------------------------------
CENTRAL HEATING - control system for central heating
Petr Fory pfory@seznam.cz
GIT - https://github.com/pfory/central-heating
*/

#include "Configuration.h"

//byte            sensorOrder[NUMBER_OF_DEVICES];

// Setup a oneWire instance to communicate with any OneWire devices (not just Maxim/Dallas temperature ICs)
OneWire oneWireKAMNA(ONE_WIRE_BUS_KAMNA);
OneWire oneWireROOM(ONE_WIRE_BUS_ROOM);

// Pass our oneWire reference to Dallas Temperature. 
DallasTemperature sensorsKAMNA(&oneWireKAMNA);
DallasTemperature sensorROOM(&oneWireROOM);

DeviceAddress inThermometer, outThermometer, roomThermometer;
DeviceAddress tempDeviceAddress;

bool firstTempMeasDone                            = false;

float sensor[NUMBER_OF_DEVICES];


float                 tempOUT                     = 0.f;
float                 tempIN                      = 0.f;
float                 tempROOM                    = 0.f;
bool                  tempRefresh                 = false;
float                 temp[15];              

byte                  relayStatus                 = RELAY_ON;

uint32_t              runSecToday                 = 0;

bool                  todayClear                  = false;
bool                  dispClear                   = false;

//#define SIMTEMP

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
  58,
  3,
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

//MQTT callback
void callback(char* topic, byte* payload, unsigned int length) {
  String val =  String();
  DEBUG_PRINT("Message arrived [");
  DEBUG_PRINT(topic);
  DEBUG_PRINT("] ");
  for (unsigned int i=0;i<length;i++) {
    DEBUG_PRINT((char)payload[i]);
    val += (char)payload[i];
  }
  DEBUG_PRINTLN();
  
  if (strcmp(topic, (String(mqtt_base) + "/" + String(mqtt_topic_relay_type_set)).c_str())==0) {
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
  } else if (strcmp(topic, (String(mqtt_base) + "/" + String(mqtt_config_portal)).c_str())==0) {
    printMessageToLCD(topic, val);
    startConfigPortal();
  } else if (strcmp(topic, (String(mqtt_base) + "/" + String(mqtt_config_portal_stop)).c_str())==0) {
    printMessageToLCD(topic, val);
    stopConfigPortal();
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
    DEBUG_PRINT("Set alarm temperature: ");
    DEBUG_PRINTLN(val.toInt());
    storage.tempAlarm = val.toInt();
    saveConfig();
  } else if (strcmp(topic, mqtt_topic_tvymenikKamna)==0) {
    DEBUG_PRINT("Temperature vymenik kamna: ");
    DEBUG_PRINTLN(val.toFloat());
    lcd.setCursor(0,1);
    lcd.print("KAM:");
    displayValue(TEMPVYMENIKKAMNA_X,TEMPVYMENIKKAMNA_Y, (int)round(val.toFloat()), 2, 0);
    lcd.write(byte(0));
    lcd.print("C");
  } else if (strcmp(topic, mqtt_topic_tvymenikSolar)==0) {
    DEBUG_PRINT("Temperature vymenik solar: ");
    DEBUG_PRINTLN(val.toFloat());
    lcd.setCursor(10,1);
    lcd.print("SOL:");
    displayValue(TEMPVYMENIKSOLAR_X,TEMPVYMENIKSOLAR_Y, (int)round(val.toFloat()), 2, 0);
    lcd.write(byte(0));
    lcd.print("C");
  } else if (strcmp(topic, mqtt_topic_termohlavicePozice)==0) {
    DEBUG_PRINT("Poloha termohlavice: ");
    DEBUG_PRINTLN(val.toFloat());
    displayValue(POSTERMOHLAVICE_X,POSTERMOHLAVICE_Y, (int)round(val.toFloat()), 3, 0);
    lcd.write(byte(0));
    lcd.print("%");
  }
}

void printMessageToLCD(char* t, String v) {
  lcd.clear();
  lcd.print(t);
  lcd.print(": ");
  lcd.print(v);
  //delay(2000);
  lcd.clear();
}


/////////////////////////////////////////////   S  E  T  U  P   ////////////////////////////////////
void setup(void) {
  preSetup();
  client.setCallback(callback);

  pinMode(RELAYPIN, OUTPUT);
  digitalWrite(RELAYPIN, relayStatus);

  lcd.init();               // initialize the lcd 
  lcd.backlight();
  lcd.home();
  lcd.print(SW_NAME);
  PRINT_SPACE
  lcd.print(VERSION);
  lcd.createChar(0, customChar);

  pinMode(ONE_WIRE_BUS_KAMNA, INPUT);
  pinMode(ONE_WIRE_BUS_ROOM, INPUT);

  pinMode(BUZZERPIN, OUTPUT);
#ifdef PIR
  pinMode(PIRPIN, INPUT);
#endif

  //filesystem
  SPIFFS.begin();
  FSInfo fs_info;
  SPIFFS.info(fs_info);
  printf("SPIFFS: %lu of %lu bytes used.\n",
         fs_info.usedBytes, fs_info.totalBytes);
         
#ifdef beep
  //peep.Delay(100,40,1,255);
  //tone(BUZZERPIN, 5000, 5);
#endif  
  
  //keypad.begin();
  
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
    if (sensorsKAMNA.getDeviceCount()==0) {
// #endif
#ifdef beep
    //peep.Delay(100,40,1,255);
#endif
      DEBUG_PRINTLN(F("NO temperature sensor(s) DS18B20 found!!!!!!!!!"));
      DEBUG_PRINTLN(sensorROOM.getDeviceCount());
      DEBUG_PRINTLN(sensorsKAMNA.getDeviceCount());
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
  DEBUG_PRINT(sensorROOM.getDeviceCount());
  DEBUG_PRINT(F(" on bus ROOM - pin "));
  DEBUG_PRINTLN(ONE_WIRE_BUS_ROOM);
  DEBUG_PRINT(F("Device Address: "));
  printAddress(roomThermometer);
  DEBUG_PRINTLN();
  
  lcd.setCursor(0,0);
  lcd.print(F(SW_NAME));
  lcd.print(F(" "));
  lcd.print(F(VERSION));
  
  lcd.setCursor(0,1);
  lcd.print(F("Bus ROOM "));
  lcd.print(sensorROOM.getDeviceCount());
  lcd.print(F(" sen."));

  DEBUG_PRINT(F("Sensor(s) "));
  DEBUG_PRINT(sensorsKAMNA.getDeviceCount());
  DEBUG_PRINT(F(" on bus KAMNA - pin "));
  DEBUG_PRINTLN(ONE_WIRE_BUS_KAMNA);
  DEBUG_PRINT(F("Device Address: "));
  printAddress(inThermometer);
  DEBUG_PRINTLN();
  DEBUG_PRINT(F("Device Address: "));
  printAddress(outThermometer);
  DEBUG_PRINTLN();
   
  lcd.setCursor(0,2);
  lcd.print(F("Bus KAMNA "));
  lcd.print(sensorsKAMNA.getDeviceCount());
  lcd.print(F(" sen."));
 
  delay(2000);
  lcd.clear();
  
#ifdef timers
  //setup timers
  timer.every(SEND_DELAY,     sendDataMQTT);
  //timer.every(SENDSTAT_DELAY, sendStatisticMQTT);
  timer.every(MEAS_DELAY,     tempMeas);
  timer.every(1000,           calcStat);
  timer.every(CONNECT_DELAY,  reconnect);
#ifdef cas  
  timer.every(500,            displayTime);
#endif
#endif
  void * a;
  reconnect(a);
  sendStatisticMQTT(a);
  sendNetInfoMQTT();
  
  ticker.detach();
  //keep LED on
  digitalWrite(LED_BUILTIN, HIGH);

  drd.stop();

  DEBUG_PRINTLN(F("SETUP END......................."));
}

/////////////////////////////////////////////   L  O  O  P   ///////////////////////////////////////
void loop(void) { 
  firstTempMeasDone ? relay() : void();

  timer.tick(); // tick the timer
#ifdef ota
  ArduinoOTA.handle();
#endif

  client.loop();
  wifiManager.process();

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
  //keyBoard();
  
#ifdef PIR
  if (digitalRead(PIRPIN)==1) {
    lcd.backlight();
  } else {
    lcd.noBacklight();
  }  
#endif

#ifdef cas
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
    } else if (relayStatus == RELAY_ON && (tempOUT < (storage.tempON - storage.tempOFFDiff))) { 
      relayStatus = RELAY_OFF;
      changeRelay(relayStatus);
    }
  }
  dispRelayStatus();
}

#ifdef cas
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
  dispRelayStatus();
}

void dispRelayStatus() {
  lcd.setCursor(RELAY_STATUSX,RELAY_STATUSY);
  if (storage.relayType==RELAY_TYPE_AUTO) {
    if (relayStatus==RELAY_ON) {
      lcd.print("AON");
    } else if (relayStatus==RELAY_OFF) {
      lcd.print("AOF");
    }
  } else if (storage.relayType==RELAY_TYPE_MANUAL_ON) {
    lcd.print("MON");
  } else if (storage.relayType==RELAY_TYPE_MANUAL_OFF) {
    lcd.print("MOF");
  }
}


bool tempMeas(void *) {
  tempRefresh = true;
  DEBUG_PRINT(F("Requesting temperatures..."));
  sensorROOM.requestTemperatures(); // Send the command to get temperatures
  sensorsKAMNA.requestTemperatures(); // Send the command to get temperatures
  DEBUG_PRINTLN(F("DONE"));
  
  float tempTemp=(float)TEMP_ERR;
  for (byte j=0;j<10;j++) { //try to read temperature ten times
    tempTemp = sensorsKAMNA.getTempC(inThermometer);
    if (tempTemp>=-55) {
      break;
    }
  }
  tempIN = tempTemp;
  
  tempTemp=(float)TEMP_ERR;
  for (byte j=0;j<10;j++) { //try to read temperature ten times
    tempTemp = sensorsKAMNA.getTempC(outThermometer);
    if (tempTemp>=-55) {
      break;
    }
  }
  tempOUT = tempTemp;

  tempTemp=(float)TEMP_ERR;
  for (byte j=0;j<10;j++) { //try to read temperature ten times
    tempTemp = sensorROOM.getTempC(roomThermometer);
    if (tempTemp>=-55) {
      break;
    }
  }
  tempROOM = tempTemp;

  
  //printTemp();

  DEBUG_PRINTLN(tempIN);
  DEBUG_PRINTLN(tempOUT);
  DEBUG_PRINTLN(tempROOM);

  //obcas se vyskytne chyba a vsechna cidla prestanou merit
  //zkusim restartovat sbernici
  bool reset=false;
 
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
  
  
}


//---------------------------------------------D I S P L A Y ------------------------------------------------
/*
  01234567890123456789
  --------------------
0|20/56/66 CER   40/45
1|                    
2|                    
3|19:20 15°C     1440m
  --------------------
  01234567890123456789  
  
  
*/
void display() {
  if (!tempRefresh) {
    return;
  }
  tempRefresh=false;
  if (tempROOM==TEMP_ERR) {
    displayTempErr(TEMPROOMX,TEMPROOMY);
  }else {
    displayValue(TEMPROOMX,TEMPROOMY, (int)tempROOM, 2, 0);
  }
  lcd.print(F("/"));
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
  
  //statistics of pump run
  displayValue(RUNMINTODAY_X,RUNMINTODAY_Y, (int)(runSecToday / 60), 4, 0);
  lcd.print(MIN_UNIT);
}

void displayTempErr(int x, int y) {
  lcd.setCursor(x,y);
  lcd.print("--");
}

//zabranuje zatuhnuti cerpadla v lete
void testPumpProtect() {
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

#ifdef cas
bool displayTime(void *) {
  lcd.setCursor(TIMEX, TIMEY); //col,row
  char buffer[6];
  // Wire.beginTransmission(8);
  // Wire.write((byte)tempOUT);
  // Wire.endTransmission();
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
#endif

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


bool calcStat(void *) {  //run each second from timer
  if (relayStatus == RELAY_ON) {
    runSecToday ++;
  }
  return true;
}

void dsInit(void) {
  sensorsKAMNA.begin(); 
  sensorROOM.begin(); 

  sensorsKAMNA.getAddress(inThermometer, 1); 
  sensorsKAMNA.getAddress(outThermometer, 0); 
  sensorROOM.getAddress(roomThermometer, 0); 

  sensorROOM.setResolution(TEMPERATURE_PRECISION);
  sensorsKAMNA.setResolution(TEMPERATURE_PRECISION);
  
  sensorROOM.setWaitForConversion(false);
  sensorsKAMNA.setWaitForConversion(false);
}

void sendRelayMQTT(byte akce) {
  digitalWrite(LED_BUILTIN, LOW);

  client.publish((String(mqtt_base) + "/relayChange").c_str(), String(akce).c_str());
 
  digitalWrite(LED_BUILTIN, HIGH);
}

bool sendDataMQTT(void *) {
  digitalWrite(LED_BUILTIN, LOW);
  DEBUG_PRINTLN(F("Data"));

  client.publish((String(mqtt_base) + "/tINKamna").c_str(), String(tempIN).c_str());
  client.publish((String(mqtt_base) + "/tOUTKamna").c_str(), String(tempOUT).c_str());
  client.publish((String(mqtt_base) + "/tROOMKamna").c_str(), String(tempROOM).c_str());
  client.publish((String(mqtt_base) + "/storage.tempON").c_str(), String(storage.tempON).c_str());
  client.publish((String(mqtt_base) + "/storage.tempOFFDiff").c_str(), String(storage.tempOFFDiff).c_str());
  client.publish((String(mqtt_base) + "/sPumpKamna/status").c_str(), String(relayStatus==RELAY_ON ? 1 : 0).c_str());

  digitalWrite(LED_BUILTIN, HIGH);
  return true;
}

bool reconnect(void *) {
  if (!client.connected()) {
    DEBUG_PRINT("Attempting MQTT connection...");
     if (client.connect(mqtt_base, mqtt_username, mqtt_key, (String(mqtt_base) + "/LWT").c_str(), 2, true, "offline", true)) {
      client.subscribe((String(mqtt_base) + "/" + String(mqtt_topic_restart)).c_str());
      client.subscribe((String(mqtt_base) + "/" + String(mqtt_topic_netinfo)).c_str());
      client.subscribe((String(mqtt_base) + "/" + String(mqtt_config_portal)).c_str());
      client.subscribe((String(mqtt_base) + "/" + String(mqtt_config_portal_stop)).c_str());
      client.subscribe((String(mqtt_base) + "/" + String(mqtt_topic_relay_type_set)).c_str());
      client.subscribe((String(mqtt_base) + "/" + String(mqtt_topic_setTempON)).c_str());
      client.subscribe((String(mqtt_base) + "/" + String(mqtt_topic_setTempOFFDiff)).c_str());
      client.subscribe((String(mqtt_base) + "/" + String(mqtt_topic_setTempAlarm)).c_str());
      client.publish((String(mqtt_base) + "/LWT").c_str(), "online", true);
      client.subscribe(mqtt_topic_weather);
      client.subscribe(mqtt_topic_tvymenikKamna);
      client.subscribe(mqtt_topic_tvymenikSolar);
      client.subscribe(mqtt_topic_termohlavicePozice);
      DEBUG_PRINTLN("connected");
    } else {
      DEBUG_PRINT("disconected.");
      DEBUG_PRINT(" Wifi status:");
      DEBUG_PRINT(WiFi.status());
      DEBUG_PRINT(" Client status:");
      DEBUG_PRINTLN(client.state());
    }
  }
  return true;
}