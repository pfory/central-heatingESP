#include "Configuration.h"

int number[]      = {0x7E, 0x0C, 0xB6, 0x9E, 0xCC, 0xDA, 0xFA, 0x0E, 0xFE, 0xDE, 0x00};

int temperature   = 0;
int cerpadlo      = 0;
int blik          = 0;

//ADC_MODE(ADC_VCC);

#ifdef serverHTTP
void handleRoot() {
	char temp[600];
  DEBUG_PRINT("Web client request...");
  digitalWrite(LED_BUILTIN, LOW);
  DEBUG_PRINTLN("done.");
  digitalWrite(LED_BUILTIN, HIGH);
}
#endif

//MQTT callback
void callback(char* topic, byte* payload, unsigned int length) {
  // char * pEnd;
  String val =  String();
  DEBUG_PRINT("\nMessage arrived [");
  DEBUG_PRINT(topic);
  DEBUG_PRINT("] ");
  for (unsigned int i=0;i<length;i++) {
    DEBUG_PRINT((char)payload[i]);
    val += (char)payload[i];
  }
  DEBUG_PRINTLN();
  
  if (strcmp(topic, (String(mqtt_base) + "/" + String(mqtt_topic_restart)).c_str())==0) {
    DEBUG_PRINT("RESTART");
    ESP.restart();
  } else if (strcmp(topic, (String(mqtt_base) + "/" + String(mqtt_topic_netinfo)).c_str())==0) {
    sendNetInfoMQTT();
  } else if (strcmp(topic, (String(mqtt_base) + "/" + String(mqtt_config_portal)).c_str())==0) {
    startConfigPortal();
  } else if (strcmp(topic, (String(mqtt_base) + "/" + String(mqtt_config_portal_stop)).c_str())==0) {
    stopConfigPortal();
  } else if (strcmp(topic, mqtt_topic_temperature)==0) {
    DEBUG_PRINT("Temperature from kamna: ");
    DEBUG_PRINTLN(val.toFloat());
    temperature = val.toFloat();
  } else if (strcmp(topic, mqtt_topic_cerpadlo)==0) {
    DEBUG_PRINT("Cerpadlo: ");
    DEBUG_PRINTLN(val.toInt());
    cerpadlo = val.toInt();
  }
}


void setup() {
  preSetup();
  
#ifdef serverHTTP
  server.on ( "/", handleRoot );
  server.begin();
  DEBUG_PRINTLN ( "HTTP server started!!" );
#endif

Wire.begin(SDA, SCL);
  
init_TLC59116(I2C_ADDR);
set_all(I2C_ADDR, 0);

#ifdef timers
  //setup timers
  //timer.every(SEND_DELAY, sendDataMQTT);
  timer.every(CONNECT_DELAY, reconnect);
  timer.every(SHOW_DISPLAY, show_display);
#endif

  void * a;
  reconnect(a);
  // sendDataMQTT(a);

  ticker.detach();
  //keep LED on
  digitalWrite(LED_BUILTIN, HIGH);

  //drd.stop();

  DEBUG_PRINTLN(F("SETUP END......................."));
}


void loop() {
  timer.tick(); // tick the timer
#ifdef serverHTTP
  server.handleClient();
#endif
  client.loop();
  wifiManager.process();
#ifdef ota
  ArduinoOTA.handle();
#endif
  drd.loop();
}
bool show_display(void *) {
  
  int jas = map(analogRead(A0), 0, 1024, 0, 255);
  
  int n1 = number[temperature / 10];
  int n2 = number[temperature % 10];

  // n1 |= 1 << 8; // desetinna tecka na prvni pozici zleva
  if (cerpadlo==1) {
    if (blik==0) {
      blik=1;
      n2 |= 1 << 8; // desetinna tecka na druhe pozici zleva
    } else {
      blik=0;
      n2 |= 0 << 8; // desetinna tecka na druhe pozici zleva
    }
  }
  print_num(I2C_ADDR, (n1 << 8) | n2, jas, false);
  
  //Serial.println(analogRead(A0));
  Serial.println(jas);
  
  return true;
}

void init_TLC59116(int addr) {
  
  Wire.beginTransmission(addr);
  Wire.write(0x80);

  Wire.write(0x00);  // Register 00 /  Mode1  
  Wire.write(0x00);  // Register 01 /  Mode2 

  Wire.write(0x50);  // Register 02 /  PWM LED 1
  Wire.write(0x00);  // Register 03 /  PWM LED 2    
  Wire.write(0x00);  // Register 04 /  PWM LED 3
  Wire.write(0x00);  // Register 05 /  PWM LED 4
  Wire.write(0x00);  // Register 06 /  PWM LED 5
  Wire.write(0x00);  // Register 07 /  PWM LED 6
  Wire.write(0x00);  // Register 08 /  PWM LED 7
  Wire.write(0x00);  // Register 09 /  PWM LED 8
  Wire.write(0x00);  // Register 0A /  PWM LED 9
  Wire.write(0x00);  // Register 0B /  PWM LED 10
  Wire.write(0x00);  // Register 0C /  PWM LED 11
  Wire.write(0x00);  // Register 0D /  PWM LED 12
  Wire.write(0x00);  // Register 0E /  PWM LED 13
  Wire.write(0x00);  // Register 0F /  PWM LED 14
  Wire.write(0x00);  // Register 10 /  PWM LED 15
  Wire.write(0x00);  // Register 11 /  PWM LED 16

  Wire.write(0xFF);  // Register 12 /  Group duty cycle control
  Wire.write(0x00);  // Register 13 /  Group frequency
  Wire.write(0xAA);  // Register 14 /  LED output state 0  // Default alle LEDs auf PWM 0xAA
  Wire.write(0xAA);  // Register 15 /  LED output state 1  // Default alle LEDs auf PWM
  Wire.write(0xAA);  // Register 16 /  LED output state 2  // Default alle LEDs auf PWM
  Wire.write(0xAA);  // Register 17 /  LED output state 3  // Default alle LEDs auf PWM
  Wire.write(0x00);  // Register 18 /  I2C bus subaddress 1
  Wire.write(0x00);  // Register 19 /  I2C bus subaddress 2
  Wire.write(0x00);  // Register 1A /  I2C bus subaddress 3
  Wire.write(0x00);  // Register 1B /  All Call I2C bus address
  Wire.write(0xFF);  // Register 1C /  IREF configuration  
  Wire.endTransmission();
}

void set_all(int addr, int pwm) {
  Wire.begin();                     
  Wire.beginTransmission(addr);
  Wire.write(0x82);                 // Startregister 02h 
  for (int i=1 ; i < 17; i++){      // 16Bytes (Register 02h bis 11h)
    Wire.write(pwm);
  }
  Wire.endTransmission();
}

void set_pin(int addr, int pin, int pwm) {
  Wire.begin();         
  Wire.beginTransmission(addr); 
  Wire.write(0x01 + pin);    // Register LED-Nr
  Wire.write(pwm);
  Wire.endTransmission(); 
}

void print_num(int addr, int number, int pwm, bool d) {
  Wire.begin();                 
  Wire.beginTransmission(addr); 
  Wire.write(0x82);                 // Startregister 02h 
  for (int i=1 ; i < 17; i++){      // 16Bytes (Register 02h bis 11h)
    if (bitRead(number, i)){
      Wire.write(pwm);
      if (d) Serial.print(1);
    }else{
      Wire.write(0);
      if (d) Serial.print(0);
    }
    if (d) if (i == 8) Serial.print(" ");
  }
  if (d) Serial.println();
  Wire.endTransmission();
}

// bool sendDataMQTT(void *) {
  // digitalWrite(LED_BUILTIN, LOW);
  // DEBUG_PRINT(F("Send data..."));

  // // client.publish((String(mqtt_base) + "/Temperature").c_str(), String(temperature).c_str());
  // // client.publish((String(mqtt_base) + "/Press").c_str(), String(pressure).c_str());
  // // client.publish((String(mqtt_base) + "/Temp085").c_str(), String(temperature085).c_str());
  // // client.publish((String(mqtt_base) + "/Humidity").c_str(), String(humidity).c_str());
  // // client.publish((String(mqtt_base) + "/Temp7021").c_str(), String(tempSI7021).c_str());
  // // client.publish((String(mqtt_base) + "/DewPoint").c_str(), String(dewPoint).c_str());

  // digitalWrite(LED_BUILTIN, HIGH);
  // DEBUG_PRINTLN(F("DONE!"));
  // return true;
// }

bool reconnect(void *) {
  if (!client.connected()) {
    DEBUG_PRINT("Attempting MQTT connection...");
    if (client.connect(mqtt_base, mqtt_username, mqtt_key, (String(mqtt_base) + "/LWT").c_str(), 2, true, "offline", true)) {
      client.subscribe((String(mqtt_base) + "/" + String(mqtt_topic_restart)).c_str());
      client.subscribe((String(mqtt_base) + "/" + String(mqtt_topic_netinfo)).c_str());
      client.subscribe((String(mqtt_base) + "/" + String(mqtt_config_portal)).c_str());
      client.subscribe((String(mqtt_base) + "/" + String(mqtt_config_portal_stop)).c_str());
      client.subscribe(String(mqtt_topic_temperature).c_str());
      client.subscribe(String(mqtt_topic_cerpadlo).c_str());
      client.publish((String(mqtt_base) + "/LWT").c_str(), "online", true);
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