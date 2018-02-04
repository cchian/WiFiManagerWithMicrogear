//created date Nov 01, 2017
//success date Nov 25, 2017
#include <ArduinoJson.h>
#include <EEPROM.h>

//uncomment below to use Arduino Board as IO Pins
//#define ARDUINO_AS_IO

#define SERIAL_BAUD_RATE      115200    //Serial_Baud_Rate
#define HTTP_PORT             80        //HTTP port for WebServer
#define TCP_PORT              1988      //TCP port for socket
#define DEVICE_NAME           "GearDev" //HostName

boolean canRungear      =     false;
boolean stringComplete  =     false;    // whether the string is complete on Serial
String inputString      =     "";       // a String to hold incoming data from Serial

unsigned long previousMillis = 0, pMillis = 0, pRunning = 0;
const long interval = 5000;
unsigned int nextTime = 0;

/*esp มีหน้าที่อัพเดท STA ไปให้บอร์ด arduino ไม่ให้ arduino เช็คว่า STA มีค่าเป็น 0
  บอร์ด Arduino จะวิเคราะห์ ถ้า STA มีค่า เป็น 0 แสดงว่า esp8266 ไม่ยอมส่งสวยหรือบกพร่องในการทำงาน
  Arduino ก็จะทำการประหาร esp8266 ทันที เพื่อให้ esp8266 เกิดใหม่หรือทำงานเป็นปกติเหมือนเดิมนั่นเอง
*/
int sta_running_duration = (1000 * 60) * 5; // 5 นาที

typedef struct Device {
  PGM_P label;   //ชื่อที่จะให้แสดง
  byte reserve;  //สงวนไว้ใช้ในอนาคต (ยังไม่ได้ใช้)
  byte cursta;   //สถานะปัจจุบันของอุปกรณ์
  byte pins[4];  //ขาสัญญาณทั้งหมดที่ใช้ในแต่ละแชนแนล (รองรับได้ไม่เกิน 4)
};

/*การเรียกชื่อแชลแนลจะเรียงตามการเรียกอินเด็กซ์ของอาเรย์ คือ เริ่มจาก 0
   ดังนั้นในการควบคุมอุปกรณ์ไฟฟ้า Message
   ต้องเขียนในรูปบบตาม >> อ่านได้ในหน้า Readme
*/
#if defined(ARDUINO_AS_IO)
Device dev[7] = {
  //{label,N / A, cursta, pin_used}
  {"Front",  0,    0,     2}, //ใช้ขา 2
  {"Inside", 0,    0,     3}, //     3
  {"Back",   0,    0,     4}, //     4
  {"Toilet", 0,    0,     5}, //     5
  {"BedRoom",0,    0,    12}, //    12
  {"FAN-A",  0,    0,    {6,  7, 8}},// 6 7 8 สำหรับพัดลมหรืออุปกรณ์ที่ปรับความเร็วได้ 3 ระดับ L/M/H (ต้องใช้ขาสัญญาณ 3 ขา)
  {"Flash Light 2D", 0,    0,   {22, 23}}   // 22 23 สำหรับพัดลมหรืออุปกรณ์ที่ปรับความเร็วได้ 2 ระดับ L/H (ที่ต้องใช้ขาสัญญาณ 2 ขา)
};
#else

const char naabaan[22] PROGMEM  = { //"ไฟตาแมว"
    0xE0,0xB9,0x84,0xE0,0xB8,0x9F,0xE0,0xB8,0x95,0xE0,0xB8,0xB2,0xE0,0xB9,0x81,0xE0,0xB8,0xA1,0xE0,0xB8,0xA7,0x00
};
const char hero_fan[31] PROGMEM   = {//"พัดลมฮีโร่"
    0xE0,0xB8,0x9E,0xE0,0xB8,0xB1,0xE0,0xB8,0x94,0xE0,0xB8,0xA5,0xE0,0xB8,0xA1,0xE0,0xB8,0xAE,0xE0,0xB8,0xB5,0xE0,0xB9,0x82,0xE0,0xB8,0xA3,0xE0,0xB9,0x88,0x00
};
Device dev[2] = {
  //หลีกเลี่ยงการใช้ขา D3
  //not working with pin D3, D3 mapped to pin GPIO0, 0 หรือ null byte ถูกใช้เป็นรหัสปิดท้ายข้อมูลในหน่วยความจำ
  //warning! D4 reserver for LED Indicator on ESP8266Webserver
  /*{label,   N/A, cursta, pin_used}*/
  {naabaan,  0,    0,     D0},
  {hero_fan, 0,    0,     {D2, D5, D6}}
};
#endif
//ตำแหน่ง EEPROM ที่จะให้กู้คืนค่าสถานะขาสัญญาณ
#define PREVIOS_STATUS_ADDRESS 2

#if defined(__AVR_ATmega328P__) || defined(__AVR_ATmega1280__) || defined(__AVR_ATmega2560__)


#include "Button.h"

//สำหรับนำไปทริกขารีเซ็ตของ ESP8266
#define RESET_TRICKER  10
Button resetSwitch(18);
//สำหรับนำไปทริกขาเคลีร์ยการตั้งค่า
#define CLEAR_SETTING_TRICKER  11

int STA = 1;
bool espboot = false;
#include "pitches.h"
#else
#define CLEAR_SETTING_TRICKER D1        //Clear Setting Tricker,common use with Reset pin
#endif
#ifdef ARDUINO_ESP8266_NODEMCU
#include <WebSocketsServer.h>

//for socket
//WiFiServer socketServer(TCP_PORT);
//WiFiClient socketClient;
WebSocketsServer webSocket = WebSocketsServer(TCP_PORT);
#endif
#include "MyFunction.h"

#ifdef ARDUINO_ESP8266_NODEMCU
//needed for library
#include <WiFiManager.h>         //https://github.com/cchian/WiFiManagerWithMicrogear (WiFiManager เวอร์ชันปรับปรุง โดย วิเชียร โตโส)
#include <WiFiClient.h>
#include <MicroGear.h>


//WiFiManager
//Local intialization. Once its business is done, there is no need to keep it around
WiFiManager wifiManager;
WiFiClient mgearClient;



//for GUI Webpage
ESP8266WebServer webServer(HTTP_PORT);
//for background Service
//ESP8266WebServer service(8888);
MicroGear microgear(mgearClient);

DynamicJsonBuffer json;

#include "WebContent.h"
/*//use this for set Microgear parameter
  MicrogearParameter mgear{
  "AppID",
  "Key",
  "Secret",
  "Alias"
  };*/

void dumpioIndex() {
  Serial.print("all=");
  for (int i = 0; i < sizeof(dev) / sizeof(dev[0]); i++) {
    Serial.print(dev[i].cursta);
    if (i < sizeof(dev) / sizeof(dev[0]) - 1)Serial.print(",");
  }
  Serial.println();
}

void commander(uint8_t* msg, unsigned int msglen) {
  String s = char2String(msg, msglen);
  s.replace("\"", "\"");
  JsonObject& root = json.parseObject(s);
  Serial.print("Cammander "); Serial.print(root["type"].as<String>()); Serial.print(" : "); Serial.println(root["alias"].as<String>());
}

/* If a new message arrives, do this */
void onMsghandler(char *topic, uint8_t* msg, unsigned int msglen) {
  String sMsg = "";
  for (int i = 0; i < msglen; i++)
    sMsg += (char)msg[i];

  //process.......................
  String sender = splitString(sMsg, '#', 0);
  String recv = splitString(sMsg, '#', 1);
  String body = splitString(sMsg, '#', 2);
  if (!recv.equals(String(wifiManager.mgear.Alias)))return;

  char bsender[32];
  sender.toCharArray(bsender, 32);
  if (body.startsWith("a")) {
    String strVal = splitString(body, '=', 1);
    if (isValidNumber(strVal)) {
      int intVal = strVal.toInt();
      if (intVal >= 0 && intVal <= 255) {
        for (int i = 0; i < sizeof(dev) / sizeof(dev[0]); i++) {
          dev[i].cursta = intVal;
#ifndef ARDUINO_AS_IO
          setPinsValue(i);
#endif
        }
      }
      Serial.println("a=" + strVal);
      //uncomment below 1 line if sending data to alias
      //microgear.chat(bsender, String(wifiManager.mgear.Alias) + "#" + String(sender) + "#a=" + strVal);
      //uncomment below 1 line if sending data to topic
      microgear.publish("/chat", String(wifiManager.mgear.Alias) + "#" + String(sender) + "#a=" + strVal,true);
      //dumpioIndex();
      return;
    }
  } else if (body.startsWith("?")) {
    body = "all=";
    for (int i = 0; i < sizeof(dev) / sizeof(dev[0]); i++) {
      body += String(getAvailablePins(dev[i].pins))+":"+dev[i].cursta;
      if (i < sizeof(dev) / sizeof(dev[0]) - 1)body += ",";
    }
    Serial.println("?");
    //microgear.chat(bsender, String(wifiManager.mgear.Alias) + "#" + String(sender) + "#" + body);
    microgear.publish("/chat", String(wifiManager.mgear.Alias) + "#" + String(sender) + "#" + body,true);
    
    return;
  } else if (body.indexOf("=") > 0) {
    String strCh = splitString(body, '=', 0);
    String strVal = splitString(body, '=', 1);
    if (isValidNumber(strCh)) {
      int intCh = strCh.toInt();
      if (isValidNumber(strVal)) {
        int intVal = strVal.toInt();
        dev[intCh].cursta = intVal;
#ifndef ARDUINO_AS_IO
        setPinsValue(intCh);
#endif
        Serial.println(body);
        //microgear.chat(bsender, String(wifiManager.mgear.Alias) + "#" + String(sender) + "#" + body);
        microgear.publish("/chat", String(wifiManager.mgear.Alias) + "#" + String(sender) + "#" + body,true);
        return;
      } else if (strVal.equals("!")) {
        dev[intCh].cursta = map(dev[intCh].cursta, 0, 255, 255, 0);
#ifndef ARDUINO_AS_IO
        setPinsValue(intCh);
#endif
        Serial.print(intCh); Serial.print("="); Serial.println(dev[intCh].cursta);
        //microgear.chat(bsender, String(wifiManager.mgear.Alias) + "#" + String(sender) + "#" + String(intCh) + "=" + String(dev[intCh].cursta));
        microgear.publish("/chat", String(wifiManager.mgear.Alias) + "#" + String(sender) + "#" + String(intCh) + "=" + String(dev[intCh].cursta),true);
        return;
      }
    }
  } else if (body.indexOf(">") > 0) {
    String strVal = splitString(body, '>', 0);
    String strCh = splitString(body, '>', 1);
    int iob[sizeof(dev) / sizeof(dev[0])];
    int count = 0;
    for (int i = 0; i < sizeof(iob); i++) {
      count++;
      String ch = splitString(strCh, ',', i);
      if (ch.equals("")) break;
      if (strVal.equals("!")) {
        dev[ch.toInt()].cursta = map(dev[ch.toInt()].cursta, 0, 255, 255, 0);
      }
      else dev[ch.toInt()].cursta = strVal.toInt();
#ifndef ARDUINO_AS_IO
      setPinsValue(i);
#endif
    }
    String bbody = "a=";
    for (int i = 0; i < sizeof(dev) / sizeof(dev[0]); i++) {
      bbody += dev[i].cursta;
      if (i < sizeof(dev) / sizeof(dev[0]) - 1)bbody += ",";
    }
    //microgear.chat(bsender, String(wifiManager.mgear.Alias) + "#" + String(sender) + "#" + bbody);
    microgear.publish("/chat", String(wifiManager.mgear.Alias) + "#" + String(sender) + "#" + bbody,true);
    Serial.println(body);
    return;
  }
}

void onFoundgear(char *attribute, uint8_t* msg, unsigned int msglen) {
  commander(msg, msglen);
}

void onLostgear(char *attribute, uint8_t* msg, unsigned int msglen) {
  commander(msg, msglen);
}

/* When a microgear is connected, do this */
void onConnected(char *attribute, uint8_t* msg, unsigned int msglen) {
  String s = char2String(msg, msglen);
  Serial.println("online...");
  microgear.setAlias(wifiManager.mgear.Alias);
}

void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length) {
  String sMsg = "";
  switch (type) {
    case WStype_DISCONNECTED:
      // Serial.printf("[%u] Disconnected!\n", num);
      break;
    case WStype_CONNECTED:
      {
        //IPAddress ip = webSocket.remoteIP(num);
        //Serial.printf("[%u] Connected from %d.%d.%d.%d url: %s\n", num, ip[0], ip[1], ip[2], ip[3], payload);

        // send message to client
        //webSocket.sendTXT(num, "Connected");
      }
      break;
    case WStype_TEXT:
      for (int i = 0; i < length; i++)
        sMsg += (char)payload[i];
      #ifdef ARDUINO_ESP8266_NODEMCU
      inputString = sMsg;
      stringComplete = true;
      #else
      Serial.println(sMsg);
      #endif
      // send message to client
      // webSocket.sendTXT(num, "message here");

      // send data to all connected clients
      // webSocket.broadcastTXT("message here");
      break;
    case WStype_BIN:;
      //Serial.printf("[%u] get binary length: %u\n", num, length);
      //hexdump(payload, length);

      // send message to client
      // webSocket.sendBIN(num, payload, length);
      break;
  }

}

void setup() {
  Serial.begin(SERIAL_BAUD_RATE);
  Serial.println(F("Starting..."));
  Serial.println(F("wait for requirement to clear setting"));
  pinMode(CLEAR_SETTING_TRICKER, INPUT_PULLUP);
#ifndef ARDUINO_AS_IO
  for (int i = 0; i < sizeof(dev) / sizeof(dev[0]); i++)
    for (byte j = 0; j < getAvailablePins(i); j++)
      pinMode(dev[i].pins[j], OUTPUT);
#endif
  delay(100);
  if (digitalRead(CLEAR_SETTING_TRICKER) == 0) {
    //reset saved settings
    //wifiManager.emptyGear(); //clear gear config
    wifiManager.resetSettings();
    Serial.println(F("Setting has been cleared!"));
    // tone(D2, 494, 20);
  } else {

  }
  // tone(D2, 440, 20);
  Serial.println(F("set microgear eepromoffset"));
  microgear.setEEPROMOffset(101);

  wifiManager.initGear();
  //for debuging
  wifiManager.setDebugOutput(false);

  //use for save gear param to EEPROM
  //wifiManager.putsGear(myGear);

  //set custom ip for portal
  Serial.print(F("setup WiFi...\nif have not last connection, pending to AP Mode with SSDI:"));
  Serial.print(String(DEVICE_NAME)); Serial.println(F(" and no password"));
  wifiManager.setAPStaticIPConfig(IPAddress(10, 0, 0, 1), IPAddress(10, 0, 0, 1), IPAddress(255, 255, 255, 0));

  //fetches ssid and pass from eeprom and tries to connect
  //if it does not connect it starts an access point with the specified name
  //here  "GearDev"
  //and goes into a blocking loop awaiting configuration
  wifiManager.autoConnect(DEVICE_NAME);
  //Serial.println("Set Hostname...");
  WiFi.hostname(DEVICE_NAME);
  MDNS.begin(DEVICE_NAME);
  //or use this for auto generated name ESP + ChipID
  //wifiManager.autoConnect();
  Serial.print(F("WiFi Connected to ")); Serial.println(wifiManager.getSSID());
  Serial.print(F("IPAdress:")); Serial.println(WiFi.localIP());

  /* Add Event listeners */
  Serial.println(F("Initialize Microgear..."));

  microgear.on(MESSAGE, onMsghandler);
  microgear.on(PRESENT, onFoundgear);
  microgear.on(ABSENT, onLostgear);
  microgear.on(CONNECTED, onConnected);

  if (wifiManager.mgear.AppID != "" && !wifiManager.mgear.Key != "" && !wifiManager.mgear.Secret != "") {
    canRungear = true;
    Serial.println(F("start Microgear..."));
    if (wifiManager.mgear.Alias == "")
      microgear.init(wifiManager.mgear.Key, wifiManager.mgear.Secret, DEVICE_NAME);
    /* Initial with KEY, SECRET and also set the ALIAS here */
    else microgear.init(wifiManager.mgear.Key, wifiManager.mgear.Secret, wifiManager.mgear.Alias);

    Serial.println("AppID:" + String(wifiManager.mgear.AppID) + "\nKey:" + String(wifiManager.mgear.Key) + "\nSecret:" + String(wifiManager.mgear.Secret) + "\nAlias:" + String(wifiManager.mgear.Alias));
    Serial.println(F("Connecting Microgear to NETPIE..."));
    /* connect to NETPIE to a specific APPID */
    microgear.connect(wifiManager.mgear.AppID);
    microgear.subscribe("/chat");
  } else {
    Serial.println(F("Microgear was silent! because anything is wrong!\nPlease press reset to reboot"));
  }


  initHandleds();
  webServer.begin();
  Serial.print(F("HTTP PORT:"));
  Serial.println(String(HTTP_PORT));

  //  socketServer.begin();
  webSocket.begin();
  webSocket.onEvent(webSocketEvent);
  Serial.print(F("TCP PORT:"));
  Serial.println(String(TCP_PORT));

  Serial.println(F("done"));
}

void loop() {
  unsigned long currentMillis = millis();

  //still running webserver
  webServer.handleClient();

  //still running websocket
  webSocket.loop();
  //update running status to arduino
  if (currentMillis - pRunning >= (sta_running_duration / 2)) {
    Serial.println("RUN:1");
    pRunning = currentMillis;
  }

  //if allow to use microgear.
  if (canRungear) {
    /* To check if the microgear is still connected */

    if (currentMillis - pMillis >= (1000 * 60)*nextTime) {
      if (microgear.connected()) {
        microgear.loop();
        previousMillis = millis();
        nextTime = 0;
      } else {
        //if connection lost more than 5s then reconnect.
        if (millis() - previousMillis >= interval) {
          Serial.println(F("Lost Connection, reconnecting...."));
          microgear.connect(wifiManager.mgear.AppID);
        } Serial.print(F("recheck connection in next "));
        nextTime += 1;
        if (microgear.connected()) {
          Serial.print(nextTime);
          Serial.println(F(" minutes"));
        }
      }
      pMillis = currentMillis;
    }
  }
  //Serial.println("test");
  //delay(1000);
  //other ........... .
  //serialEvent();

  /*
    if (!socketClient.connected()) {
      //try to connect to a new client
      socketClient = socketServer.available();
    } else {
      if (socketClient.available() > 0) {
        while (socketClient.available()) {
          byte b = socketClient.read();
          if (b == '\n') {
            stringComplete = true;
            break;
          }

          inputString += (char)b;
        }
        socketClient.flush();
      }
    }*/

  if (stringComplete) {
    processCommand();
    Serial.println(inputString);
    //    socketClient.println(inputString);
    inputString = "";
    stringComplete = false;
  }
}

#endif
