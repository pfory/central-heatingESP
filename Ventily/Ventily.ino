/*
--------------------------------------------------------------------------------------------------------------------------
VENTILY - control system for central heating
Petr Fory pfory@seznam.cz
GIT - https://github.com/pfory/central-heatingESP
*/

#include "Configuration.h"

PCF8574 PCF_01(0x20);
PCF8574 PCF_02(0x21);

<<<<<<< HEAD
// Rotační enkodér KY-040

//PCF-01
int pinPolarityRelay    = 0;
int pinRelayValve1      = 1;
int pinRelayValve2      = 2;
int pinRelayValve3      = 3;
int pinRelayValve4      = 4;
int pinRelayValve5      = 5;
int pinCLKValve1        = 6;
int pinDTValve1         = 7;
//PCF-02
int pinCLKValve2        = 0;
int pinDTValve2         = 1;
int pinCLKValve3        = 2;
int pinDTValve3         = 3;
int pinCLKValve4        = 4;
int pinDTValve4         = 5;
int pinCLKValve5        = 6;
int pinDTValve5         = 7;
//PCF-03

int poziceEnkod         = 0;
int poziceEnkodOld      = 0;
int stavPredValve1      = 0;
int stavPredValve2      = 0;
int stavPredValve3      = 0;
int stavPredValve4      = 0;
int stavPredValve5      = 0;
int stavCLK             = 0;
int stavPred            = 0;

int valve1Set           = 0;
int valve2Set           = 0;
int valve3Set           = 0;
int valve4Set           = 0;
int valve5Set           = 0;
bool change             = false;
int changeValve         = 0;
=======
/*HW
1x Wemos D1
2x I2C expander PCF8574
5x Rotační enkodér KY-040
5x motor
1x relay module 8 channel

*/

//PCF-01                           
int pinCLKValve1                                  = 0;
int pinDTValve1                                   = 1;
int pinPolarityRelay                              = 2;
int pinRelayValve1                                = 3;
int pinCLKValve2                                  = 4;
int pinDTValve2                                   = 5;
int pinRelayValve2                                = 6;
int pinCLKValve3                                  = 7;
//PCF-02                           
int pinDTValve3                                   = 0;
int pinRelayValve3                                = 1;
int pinCLKValve4                                  = 2;
int pinDTValve4                                   = 3;
int pinRelayValve4                                = 4;
int pinCLKValve5                                  = 5;
int pinDTValve5                                   = 6;
int pinRelayValve5                                = 7;
                           
int poziceEnkod                                   = 0;
int poziceEnkodOld                                = 0;
int stavPredValve1                                = 0;
int stavPredValve2                                = 0;
int stavPredValve3                                = 0;
int stavPredValve4                                = 0;
int stavPredValve5                                = 0;
int stavCLK                                       = 0;
int stavPred                                      = 0;
                           
int valve1Set                                     = 0;
bool change                                       = false;
int changeValve                                   = 0;
int activeValve                                   ;
>>>>>>> b7cc490b87852ac4721b783287aa020408e2f196

uint32_t              heartBeat                   = 0;



#include <timer.h>
auto timer = timer_create_default(); // create a timer with default settings
Timer<> default_timer; // save as above

//for LED status
#include <Ticker.h>
Ticker ticker;

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
  
  
  if (strcmp(topic, (String(mqtt_base) + "/" + String(mqtt_topic_restart)).c_str())==0) {
    DEBUG_PRINT("RESTART");
    ESP.restart();
  } else if (strcmp(topic, (String(mqtt_base) + "/" + String(mqtt_topic_valve1)).c_str())==0) {
    DEBUG_PRINT("set valve1 to ");
    valve1Set = val.toInt();
    setValve(valve1Set);
    changeValve = 1;
  } else if (strcmp(topic, (String(mqtt_base) + "/" + String(mqtt_topic_valve2)).c_str())==0) {
    DEBUG_PRINT("set valve2 to ");
    valve2Set = val.toInt();
    setValve(valve2Set);
    changeValve = 2;
  } else if (strcmp(topic, (String(mqtt_base) + "/" + String(mqtt_topic_valve3)).c_str())==0) {
    DEBUG_PRINT("set valve3 to ");
    valve3Set = val.toInt();
    setValve(valve3Set);
    changeValve = 3;
  } else if (strcmp(topic, (String(mqtt_base) + "/" + String(mqtt_topic_valve4)).c_str())==0) {
    DEBUG_PRINT("set valve4 to ");
    valve4Set = val.toInt();
    setValve(valve4Set);
    changeValve = 4;
  } else if (strcmp(topic, (String(mqtt_base) + "/" + String(mqtt_topic_valve5)).c_str())==0) {
    DEBUG_PRINT("set valve5 to ");
    valve5Set = val.toInt();
    setValve(valve5Set);
    changeValve = 5;
  } else if (strcmp(topic, (String(mqtt_base) + "/" + String(mqtt_topic_valveStop)).c_str())==0) {
    DEBUG_PRINT("stop valve");
    valveStop();
  }
}


void setValve(int val) {
  change = true;
	if (val==1) {
	  DEBUG_PRINTLN(F("ON"));
	} else if (val==0) {
	  DEBUG_PRINTLN(F("OFF"));
	}
}

bool isDebugEnabled()
{
#ifdef verbose
  return true;
#endif // verbose
  return false;
}

WiFiClient espClient;
PubSubClient client(espClient);


/////////////////////////////////////////////   S  E  T  U  P   ////////////////////////////////////
void setup() {
  SERIAL_BEGIN;
  DEBUG_PRINT(F(SW_NAME));
  DEBUG_PRINT(F(" "));
  DEBUG_PRINTLN(F(VERSION));

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

#ifdef ota
  ArduinoOTA.setHostname(HOSTNAMEOTA);

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
    if (error == OTA_AUTH_ERROR) DEBUG_PRINTLN("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) DEBUG_PRINTLN("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) DEBUG_PRINTLN("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) DEBUG_PRINTLN("Receive Failed");
    else if (error == OTA_END_ERROR) DEBUG_PRINTLN("End Failed");
  });
  ArduinoOTA.begin();
#endif

  PCF_01.begin();
  PCF_02.begin();

  stavPredValve1 = PCF_01.read(pinCLKValve1);
  stavPredValve2 = PCF_01.read(pinCLKValve2);
  stavPredValve3 = PCF_01.read(pinCLKValve3);
  stavPredValve4 = PCF_02.read(pinCLKValve4);
  stavPredValve5 = PCF_02.read(pinCLKValve5);
  
  timer.every(800, checkStatus);
  
  timer.every(SENDSTAT_DELAY, sendStatisticHA);
  void * a;
  sendStatisticHA(a);


  ticker.detach();
  //keep LED on
  digitalWrite(BUILTIN_LED, HIGH);
  
  DEBUG_PRINTLN(F("Setup end."));
  
  activeValve = SOLAROUT;
}

/////////////////////////////////////////////   L  O  O  P   ///////////////////////////////////////
void loop() {
  timer.tick(); // tick the timer

#ifdef ota
  ArduinoOTA.handle();
#endif

  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  if (change) {
    change = false;
    ticker.attach(1, tick);

    if (changeValve == 1) {
      if (valve1Set == 0) {
        //close
        Serial.println("Zavirani ventilu ");
<<<<<<< HEAD
        PCF_01.write(pinPolarityRelay,  HIGH);
        PCF_01.write(pinRelayValve1,    LOW);
        poziceEnkod = 0;
      } else {
        //open
        Serial.println("Otevirani ventilu ");
        PCF_01.write(pinPolarityRelay,   LOW);
        PCF_01.write(pinRelayValve1,    LOW);
        poziceEnkod = 0;
      }
    } else if (changeValve == 2) {
      if (valve2Set == 0) {
        //close
        Serial.println("Zavirani ventilu ");
        PCF_01.write(pinPolarityRelay,  HIGH);
        PCF_01.write(pinRelayValve2,    LOW);
        poziceEnkod = 0;
      } else {
        //open
        Serial.println("Otevirani ventilu ");
        PCF_01.write(pinPolarityRelay,   LOW);
        PCF_01.write(pinRelayValve2,    LOW);
        poziceEnkod = 0;
      }
    } else if (changeValve == 3) {
      if (valve3Set == 0) {
        //close
        Serial.println("Zavirani ventilu ");
        PCF_01.write(pinPolarityRelay,  HIGH);
        PCF_01.write(pinRelayValve3,    LOW);
        poziceEnkod = 0;
      } else {
        //open
        Serial.println("Otevirani ventilu ");
        PCF_01.write(pinPolarityRelay,   LOW);
        PCF_01.write(pinRelayValve3,    LOW);
        poziceEnkod = 0;
      }
    } else if (changeValve == 4) {
      if (valve4Set == 0) {
        //close
        Serial.println("Zavirani ventilu ");
        PCF_01.write(pinPolarityRelay,  HIGH);
        PCF_01.write(pinRelayValve4,    LOW);
        poziceEnkod = 0;
      } else {
        //open
        Serial.println("Otevirani ventilu ");
        PCF_01.write(pinPolarityRelay,   LOW);
        PCF_01.write(pinRelayValve4,    LOW);
        poziceEnkod = 0;
      }
    } else if (changeValve == 5) {
      if (valve5Set == 0) {
        //close
        Serial.println("Zavirani ventilu ");
        PCF_01.write(pinPolarityRelay,  HIGH);
        PCF_01.write(pinRelayValve5,    LOW);
        poziceEnkod = 0;
      } else {
        //open
        Serial.println("Otevirani ventilu ");
        PCF_01.write(pinPolarityRelay,   LOW);
        PCF_01.write(pinRelayValve5,    LOW);
        poziceEnkod = 0;
=======
        start(CLOSE);
        // PCF_01.write(pinPolarityRelay,  HIGH);
        // PCF_01.write(pinRelayValve1,    HIGH);
      } else {
        //open
        Serial.println("Otevirani ventilu ");
        start(OPEN);
        // PCF_01.write(pinPolarityRelay,   LOW);
        // PCF_01.write(pinRelayValve1,    HIGH);
>>>>>>> b7cc490b87852ac4721b783287aa020408e2f196
      }
    }
  }
  if (Serial.available() > 0) {
    int incomingByte = Serial.read();

    if (incomingByte == 48) {
      //close
      Serial.println("Zavirani ventilu ");
<<<<<<< HEAD
      PCF_01.write(pinPolarityRelay,  HIGH);
      if (changeValve == 1) {
        PCF_01.write(pinRelayValve1,    LOW);
      } else if (changeValve == 2) {
        PCF_01.write(pinRelayValve2,    LOW);
      } else if (changeValve == 3) {
        PCF_01.write(pinRelayValve3,    LOW);
      } else if (changeValve == 4) {
        PCF_01.write(pinRelayValve4,    LOW);
      } else if (changeValve == 5) {
        PCF_01.write(pinRelayValve5,    LOW);
      }
      poziceEnkod = 0;
    } else if (incomingByte == 49) {
      //open
      Serial.println("Otevirani ventilu ");
      PCF_01.write(pinPolarityRelay,   LOW);
      if (changeValve == 1) {
        PCF_01.write(pinRelayValve1,    LOW);
      } else if (changeValve == 2) {
        PCF_01.write(pinRelayValve2,    LOW);
      } else if (changeValve == 3) {
        PCF_01.write(pinRelayValve3,    LOW);
      } else if (changeValve == 4) {
        PCF_01.write(pinRelayValve4,    LOW);
      } else if (changeValve == 5) {
        PCF_01.write(pinRelayValve5,    LOW);
      }
      poziceEnkod = 0;
=======
      start(CLOSE);
      // PCF_01.write(pinPolarityRelay,  HIGH);
      // PCF_01.write(pinRelayValve1,    HIGH);
    } else if (incomingByte == 49) {
      //open
      Serial.println("Otevirani ventilu ");
      start(OPEN);
      // PCF_01.write(pinPolarityRelay,   LOW);
      // PCF_01.write(pinRelayValve1,    HIGH);
>>>>>>> b7cc490b87852ac4721b783287aa020408e2f196
    } else {
      // digitalWrite(D3, LOW);
    }
   }
    
    
  stavCLK = getStavCLK();
  if (stavCLK != stavPred) {
    int valve;
    if (activeValve == SOLAROUT) valve = pinDTValve1;
    if (activeValve == SOLARIN) valve = pinDTValve2;
    if (activeValve == RADIATOR) valve = pinDTValve3;
    if (activeValve == BOJLERIN) valve = pinDTValve4;
    if (activeValve == BOJLEROUT) valve = pinDTValve5;
  
    if (PCF_01.read(valve) != stavCLK) {
      poziceEnkod++;
    } else {
      poziceEnkod--;
    }
    DEBUG_PRINT(".");
  }
  stavPred = stavCLK;
}

/////////////////////////////////////////////   F  U  N  C   ///////////////////////////////////////
bool checkStatus(void *) {
  if (poziceEnkod == poziceEnkodOld) {
    // PCF_01.write(pinPolarityRelay,      HIGH);
    // PCF_01.write(pinRelayValve1,        HIGH);
    // PCF_01.write(pinRelayValve2,        HIGH);
    // PCF_01.write(pinRelayValve3,        HIGH);
    // PCF_01.write(pinRelayValve4,        HIGH);
    // PCF_01.write(pinRelayValve5,        HIGH);
  } else {
    poziceEnkodOld = poziceEnkod;
  }
  return true;
}

<<<<<<< HEAD
void valveStop() {
  PCF_01.write(pinPolarityRelay,      HIGH);
  PCF_01.write(pinRelayValve1,        HIGH);
  PCF_01.write(pinRelayValve2,        HIGH);
  PCF_01.write(pinRelayValve3,        HIGH);
  PCF_01.write(pinRelayValve4,        HIGH);
  PCF_01.write(pinRelayValve5,        HIGH);
}

int getStavCLK(int valve) {
  if (valve == 1) { return PCF_01.read(pinCLKValve1); }
  else if (valve == 2) { return PCF_01.read(pinCLKValve2); }
  else if (valve == 3) { return PCF_01.read(pinCLKValve3); }
  else if (valve == 4) { return PCF_01.read(pinCLKValve4); }
  else if (valve == 5) { return PCF_02.read(pinCLKValve5); }
=======
int getStavCLK() {
  if      (activeValve == SOLAROUT) { return PCF_01.read(pinCLKValve1); }
  else if (activeValve == 2) { return PCF_01.read(pinCLKValve2); }
  else if (activeValve == 3) { return PCF_01.read(pinCLKValve3); }
  else if (activeValve == 4) { return PCF_01.read(pinCLKValve4); }
  else if (activeValve == 5) { return PCF_02.read(pinCLKValve5); }
>>>>>>> b7cc490b87852ac4721b783287aa020408e2f196
  else { return 0; }
}

void start(int action) {
  if (action == CLOSE) {
    PCF_01.write(pinPolarityRelay,  HIGH);
  }
  if (action == OPEN) {
    PCF_01.write(pinPolarityRelay,   LOW);
  }
  if (activeValve == SOLAROUT) PCF_01.write(pinRelayValve1,    HIGH);
  if (activeValve == 2) PCF_01.write(pinRelayValve2,    HIGH);
  if (activeValve == 3) PCF_02.write(pinRelayValve3,    HIGH);
  if (activeValve == 4) PCF_02.write(pinRelayValve4,    HIGH);
  if (activeValve == 5) PCF_02.write(pinRelayValve5,    HIGH);

  poziceEnkod = 0;
}

// int getStavPred(int valve) {
  // if (valve == 1) { return stavPredValve1; }
  // else if (valve == 2) { return stavPredValve2; }
  // else if (valve == 3) { return stavPredValve3; }
  // else if (valve == 4) { return stavPredValve4; }
  // else if (valve == 5) { return stavPredValve5; }
  // else { return 0; }
// }

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    DEBUG_PRINT("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect(mqtt_base, mqtt_username, mqtt_key)) {
      DEBUG_PRINTLN("connected");
      // Once connected, publish an announcement...
       client.subscribe((String(mqtt_base) + "/#").c_str());
      // client.subscribe((String(mqtt_base) + "/" + "restart").c_str());
      // client.subscribe((String(mqtt_base) + "/" + "valve1").c_str());
      // client.subscribe((String(mqtt_base) + "/" + "valve2").c_str());
      // client.subscribe((String(mqtt_base) + "/" + "valve3").c_str());
      // client.subscribe((String(mqtt_base) + "/" + "valve4").c_str());
      // client.subscribe((String(mqtt_base) + "/" + "valve5").c_str());
    } else {
      DEBUG_PRINT("failed, rc=");
      DEBUG_PRINT(client.state());
      DEBUG_PRINTLN(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

bool sendDataHA(void *) {
  digitalWrite(BUILTIN_LED, LOW);
  SenderClass sender;
  DEBUG_PRINTLN(F(" - I am sending data to HA"));

  sender.sendMQTT(mqtt_server, mqtt_port, mqtt_username, mqtt_key, mqtt_base);

  digitalWrite(BUILTIN_LED, HIGH);
  return true;
}

bool sendStatisticHA(void *) {
  digitalWrite(BUILTIN_LED, LOW);
  //printSystemTime();
  DEBUG_PRINTLN(F(" - I am sending statistic to HA"));

  SenderClass sender;
  sender.add("VersionSWVentily",  VERSION);
  sender.add("HeartBeat",         heartBeat++);
  if (heartBeat % 10 == 0) sender.add("RSSI",              WiFi.RSSI());
  
  DEBUG_PRINTLN(F("Calling MQTT"));
  
  sender.sendMQTT(mqtt_server, mqtt_port, mqtt_username, mqtt_key, mqtt_base);
  digitalWrite(BUILTIN_LED, HIGH);
  return true;
}

