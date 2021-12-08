/*
--------------------------------------------------------------------------------------------------------------------------
VENTILY - control system for central heating
Petr Fory pfory@seznam.cz
GIT - https://github.com/pfory/central-heatingESP
deska LOLIN(WEMOS)D1 R2 & mini
*/

#include "Configuration.h"

PCF8574 PCF_01(0x20);
PCF8574 PCF_02(0x24);


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
int pinCLKValve4        = 4;  //bily
int pinDTValve4         = 5;  //modry
int pinCLKValve5        = 6;  //bily
int pinDTValve5         = 7;  //modry


/* variables */
int poziceEnkod         = 0;
int poziceEnkodOld      = 0;
int stavPredValve1      = 0;
int stavPredValve2      = 0;
int stavPredValve3      = 0;
int stavPredValve4      = 0;
int stavPredValve5      = 0;
int stavCLK             = 0;
int stavPred            = 0;

bool change             = false;
int activeValve         = 0;

uint32_t heartBeat      = 0;

int direction           = 0;

bool blockSendingData   = false;

#include <timer.h>
auto timer = timer_create_default(); // create a timer with default settings
Timer<> default_timer; // save as above

//for LED status
#include <Ticker.h>
Ticker ticker;

DoubleResetDetector drd(DRD_TIMEOUT, DRD_ADDRESS);

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
  } else if (strcmp(topic, (String(mqtt_base) + "/" + String(mqtt_topic_netinfo)).c_str())==0) {
    DEBUG_PRINT("NET INFO");
    sendNetInfoMQTT();
  } else if (strcmp(topic, (String(mqtt_base) + "/" + String(mqtt_topic_valve1)).c_str())==0) {
    setValve(val.toInt(), 0);
  } else if (strcmp(topic, (String(mqtt_base) + "/" + String(mqtt_topic_valve2)).c_str())==0) {
    DEBUG_PRINT("set valve2 to ");
    setValve(val.toInt(), 1);
  } else if (strcmp(topic, (String(mqtt_base) + "/" + String(mqtt_topic_valve3)).c_str())==0) {
    DEBUG_PRINT("set valve3 to ");
    setValve(val.toInt(), 2);
  } else if (strcmp(topic, (String(mqtt_base) + "/" + String(mqtt_topic_valve4)).c_str())==0) {
    DEBUG_PRINT("set valve4 to ");
    setValve(val.toInt(), 3);
  } else if (strcmp(topic, (String(mqtt_base) + "/" + String(mqtt_topic_valve5)).c_str())==0) {
    DEBUG_PRINT("set valve5 to ");
    setValve(val.toInt(), 4);
  } else if (strcmp(topic, (String(mqtt_base) + "/" + String(mqtt_topic_valveStop)).c_str())==0) {
    DEBUG_PRINTLN("STOP");
    valveStop();
  }
}


void setValve(int dir, int valve) {
  DEBUG_PRINT("set valve ");
  DEBUG_PRINT(valve);
  DEBUG_PRINT(" to ");
	if (dir==1) {
	  DEBUG_PRINTLN(F("OPEN"));
	} else if (dir==0) {
	  DEBUG_PRINTLN(F("CLOSE"));
	}
  change = true;
  direction = dir;
  activeValve = valve;
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


WiFiManager wifiManager;
  
/////////////////////////////////////////////   S  E  T  U  P   ////////////////////////////////////
void setup() {
  SERIAL_BEGIN;
  DEBUG_PRINT(F(SW_NAME));
  DEBUG_PRINT(F(" "));
  DEBUG_PRINTLN(F(VERSION));

  pinMode(BUILTIN_LED, OUTPUT);
  ticker.attach(1, tick);

  WiFi.mode(WIFI_STA); // explicitly set mode, esp defaults to STA+AP

  //set callback that gets called when connecting to previous WiFi fails, and enters Access Point mode
  wifiManager.setAPCallback(configModeCallback);
  wifiManager.setConfigPortalTimeout(CONFIG_PORTAL_TIMEOUT);
  wifiManager.setConnectTimeout(CONNECT_TIMEOUT);

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
  
  sendNetInfoMQTT();


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
  
  timer.every(SENDSTAT_DELAY, sendStatisticMQTT);
  void * a;
  sendStatisticMQTT(a);

  ticker.detach();
  digitalWrite(BUILTIN_LED, HIGH);
  
  DEBUG_PRINTLN(F("Setup end."));
  
  activeValve = SOLAROUT;
  
  valveStop();
#ifdef test
  valveTest();
#endif

  drd.stop();

  DEBUG_PRINTLN(F("Setup end."));
}

/////////////////////////////////////////////   L  O  O  P   ///////////////////////////////////////
void loop() {
  timer.tick(); // tick the timer

  if (!blockSendingData) {
#ifdef ota
    ArduinoOTA.handle();
#endif

    if (!client.connected()) {
      reconnect();
    }
    client.loop();
  }
  
  stavCLK = getStavCLK();

  if (change) {
    change = false;
    blockSendingData = true;
    ticker.attach(1, tick);

    DEBUG_PRINT("START ");
    start(direction);
    poziceEnkod = 0;
  }
      
  if (stavCLK != stavPred) {
    int valve;
    int stav;
    
    if (activeValve == SOLAROUT) {
      valve = pinDTValve1;
      stav = PCF_01.read(valve);
    } else {
      if (activeValve == SOLARIN)   valve = pinDTValve2;
      if (activeValve == RADIATOR)  valve = pinDTValve3;
      if (activeValve == BOJLERIN)  valve = pinDTValve4;
      if (activeValve == BOJLEROUT) valve = pinDTValve5;
      stav = PCF_02.read(valve);
    }
    
    if (stav != stavCLK) {
      poziceEnkod++;
    } else {
      poziceEnkod--;
    }
    DEBUG_PRINT(poziceEnkod);
  }
  stavPred = stavCLK;
  //DEBUG_PRINT(".");
}

/////////////////////////////////////////////   F  U  N  C   ///////////////////////////////////////
bool checkStatus(void *) {
  if (poziceEnkod == poziceEnkodOld) {
    valveStop();
  } else {
    poziceEnkodOld = poziceEnkod;
  }
  return true;
}

void valveStop() {
  DEBUG_PRINTLN("VALVE STOP");
  PCF_01.write(pinPolarityRelay,      HIGH);
  PCF_01.write(pinRelayValve1,        HIGH);
  PCF_01.write(pinRelayValve2,        HIGH);
  PCF_01.write(pinRelayValve3,        HIGH);
  PCF_01.write(pinRelayValve4,        HIGH);
  PCF_01.write(pinRelayValve5,        HIGH);
  blockSendingData = false;
  ticker.detach();
  digitalWrite(BUILTIN_LED, HIGH);
}

int getStavCLK() {
  if      (activeValve == SOLAROUT)   { return PCF_01.read(pinCLKValve1); }
  else if (activeValve == SOLARIN)    { return PCF_02.read(pinCLKValve2); }
  else if (activeValve == RADIATOR)   { return PCF_02.read(pinCLKValve3); }
  else if (activeValve == BOJLERIN)   { return PCF_02.read(pinCLKValve4); }
  else if (activeValve == BOJLEROUT)  { return PCF_02.read(pinCLKValve5); }
}

void start(int action) {
  if (action == CLOSE) {
    DEBUG_PRINTLN("CLOSE");
    PCF_01.write(pinPolarityRelay,  HIGH);
  }
  if (action == OPEN) {
    DEBUG_PRINTLN("OPEN");
    PCF_01.write(pinPolarityRelay,   LOW);
  }
  if (activeValve == SOLAROUT)  PCF_01.write(pinRelayValve1,    LOW);
  if (activeValve == SOLARIN)   PCF_01.write(pinRelayValve2,    LOW);
  if (activeValve == RADIATOR)  PCF_01.write(pinRelayValve3,    LOW);
  if (activeValve == BOJLERIN)  PCF_01.write(pinRelayValve4,    LOW);
  if (activeValve == BOJLEROUT) PCF_01.write(pinRelayValve5,    LOW);

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
    } else {
      DEBUG_PRINT("failed, rc=");
      DEBUG_PRINT(client.state());
      DEBUG_PRINTLN(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

// bool sendDataMQTT(void *) {
  //if (!blockSendingData) {
    // digitalWrite(BUILTIN_LED, LOW);
    // SenderClass sender;
    // DEBUG_PRINTLN(F(" - I am sending data to MQTT"));

    // sender.sendMQTT(mqtt_server, mqtt_port, mqtt_username, mqtt_key, mqtt_base);

    // digitalWrite(BUILTIN_LED, HIGH);
  //}
  // return true;
// }

bool sendStatisticMQTT(void *) {
  if (!blockSendingData) {
    digitalWrite(BUILTIN_LED, LOW);
    //printSystemTime();
    DEBUG_PRINTLN(F("Statistic"));

    SenderClass sender;
    sender.add("VersionSWVentily",  VERSION);
    sender.add("HeartBeat",         heartBeat++);
    if (heartBeat % 10 == 0) sender.add("RSSI",              WiFi.RSSI());
    
    DEBUG_PRINTLN(F("Calling MQTT"));
    
    sender.sendMQTT(mqtt_server, mqtt_port, mqtt_username, mqtt_key, mqtt_base);
    digitalWrite(BUILTIN_LED, HIGH);
  }
  return true;
}

void sendNetInfoMQTT() {
  if (!blockSendingData) {
    digitalWrite(BUILTIN_LED, LOW);
    //printSystemTime();
    DEBUG_PRINTLN(F("Net info"));

    SenderClass sender;
    sender.add("IP",              WiFi.localIP().toString().c_str());
    sender.add("MAC",             WiFi.macAddress());
    
    DEBUG_PRINTLN(F("Calling MQTT"));
    
    sender.sendMQTT(mqtt_server, mqtt_port, mqtt_username, mqtt_key, mqtt_base);
    digitalWrite(BUILTIN_LED, HIGH);
  }
  return;
}


#ifdef test
void valveTest() {
  PCF_01.write(pinPolarityRelay,   LOW);
  PCF_01.write(pinRelayValve1,    HIGH);
  DELAY
  PCF_01.write(pinRelayValve1,    LOW);
  PCF_02.write(pinRelayValve2,    HIGH);
  DELAY
  PCF_02.write(pinRelayValve2,    LOW);
  PCF_02.write(pinRelayValve3,    HIGH);
  DELAY
  PCF_02.write(pinRelayValve3,    LOW);
  PCF_02.write(pinRelayValve4,    HIGH);
  DELAY
  PCF_02.write(pinRelayValve4,    LOW);
  PCF_02.write(pinRelayValve5,    HIGH);
  DELAY
  PCF_02.write(pinRelayValve5,    LOW);
  PCF_01.write(pinPolarityRelay,   HIGH);
  PCF_01.write(pinRelayValve1,    HIGH);
  DELAY
  PCF_01.write(pinRelayValve1,    LOW);
  PCF_02.write(pinRelayValve2,    HIGH);
  DELAY
  PCF_02.write(pinRelayValve2,    LOW);
  PCF_02.write(pinRelayValve3,    HIGH);
  DELAY
  PCF_02.write(pinRelayValve3,    LOW);
  PCF_02.write(pinRelayValve4,    HIGH);
  DELAY
  PCF_02.write(pinRelayValve4,    LOW);
  PCF_02.write(pinRelayValve5,    HIGH);
  DELAY
  PCF_02.write(pinRelayValve5,    LOW);
  return;
}
#endif