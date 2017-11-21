//created date Nov 01 2017
#include <ArduinoJson.h>

#define SERIAL_BAUD_RATE      115200    //Serial_Baud_Rate
#define HTTP_PORT             80        //HTTP port for WebServer
#define DEVICE_NAME           "GearDev" //HostName
#define CLEAR_SETTING_TRICKER D1        //Clear Setting Tricker,common use with Reset pin
#define ARDUINO_VIRTUAL_RESET 11        //Assigned Digital Pin 11 for Reset pin (for Arduino Boards)

boolean canRungear      =     false;
boolean stringComplete  =     false;    // whether the string is complete
String inputString      =     "";       // a String to hold incoming data

unsigned long previousMillis = 0,pMillis=0;
const long interval = 5000;
unsigned int nextTime=0;

typedef struct Device{
  char label[10];//ชื่อที่จะให้แสดง
  byte reserve;  //สงวนไว้ใช้ในอนาคต (ยังไม่ได้ใช้)
  byte cursta;   //สถานะปัจจุบันของอุปกรณ์
  byte pins[4];  //ขาสัญญาณทั้งหมดที่ใช้ในแต่ละแชนแนล (รองรับได้ไม่เกิน 4)
};

/*การเรียกชื่อแชลแนลจะเรียงตามการเรียกอินเด็กซ์ของอาเรย์ คือ เริ่มจาก 0 
 * ดังนั้นในการควบคุมอุปกรณ์ไฟฟ้า Message
 * ต้องเขียนในรูปบบตาม >> อ่านได้ในหน้า Readme
 */
Device dev[6]={
    /*{label,   N/A, cursta, pin_used}*/
      {"Front",  0,    0,     2}, //ใช้ขา 2
      {"Inside", 0,    0,     3}, //     3
      {"Back",   0,    0,     4}, //     4
      {"Toilet", 0,    0,     5}, //     5
      {"FAN-A",  0,    0,    {6,7,8}},// 6 7 8 สำหรับพัดลมหรืออุปกรณ์ที่ปรับความเร็วได้ 3 ระดับ L/M/H (ต้องใช้ขาสัญญาณ 3 ขา)
      {"BedRoom",0,    0,    {22,23}} // 22 23 สำหรับพัดลมหรืออุปกรณ์ที่ปรับความเร็วได้ 2 ระดับ L/H (ที่ต้องใช้ขาสัญญาณ 2 ขา)
};

#include "MyFunction.h"

#ifdef ARDUINO_ESP8266_NODEMCU
//needed for library
#include <WiFiManager.h>         //https://github.com/cchian/WiFiManagerWithMicrogear (WiFiManager เวอร์ชันปรับปรุง โดย วิเชียร โตโส)
#include <WiFiClient.h>
#include <MicroGear.h>


//WiFiManager
//Local intialization. Once its business is done, there is no need to keep it around
WiFiManager wifiManager;
WiFiClient client;
//for GUI Webpage
ESP8266WebServer server(HTTP_PORT);
//for background Service
//ESP8266WebServer service(8888);
MicroGear microgear(client);

DynamicJsonBuffer json;

#include "WebContent.h"
/*//use this for set Microgear parameter
MicrogearParameter mgear{
  "AppID",
  "Key",
  "Secret",
  "Alias"
};*/

void dumpioIndex(){
  Serial.print("all=");
  for(int i=0;i<sizeof(dev)/sizeof(dev[0]);i++){
    Serial.print(dev[i].cursta);
    if(i<sizeof(dev)/sizeof(dev[0])-1)Serial.print(",");
  }
  Serial.println();
}

void commander(uint8_t* msg, unsigned int msglen){
  String s=char2String(msg,msglen);
    s.replace("\"","\"");
    JsonObject& root = json.parseObject(s);
    Serial.print("Cammander ");Serial.print(root["type"].as<String>());Serial.print(" : ");Serial.println(root["alias"].as<String>());
}

/* If a new message arrives, do this */
void onMsghandler(char *topic, uint8_t* msg, unsigned int msglen) {
    String sMsg="";
    for (int i=0; i<msglen; i++)
        sMsg+=(char)msg[i];

   //process.......................
   String sender=splitString(sMsg,'#',0);
   String recv=splitString(sMsg,'#',1);
   String body=splitString(sMsg,'#',2);
   if(!recv.equals(String(wifiManager.mgear.Alias)))return;
   
   char bsender[32];
   sender.toCharArray(bsender,32);
   if(body.startsWith("a")){
    String strVal=splitString(body,'=',1);
    if(isValidNumber(strVal)){
      int intVal=strVal.toInt();
      if(intVal>=0&&intVal<=255){
        for(int i=0;i<sizeof(dev)/sizeof(dev[0]);i++){
          dev[i].cursta=intVal;
        }
      }
      Serial.println("a="+strVal);
      microgear.chat(bsender,String(wifiManager.mgear.Alias)+"#"+String(sender)+"#a="+strVal);
      //dumpioIndex();
      return;
    }
   }else if(body.startsWith("?")){
      body="all=";
      for(int i=0;i<sizeof(dev)/sizeof(dev[0]);i++){
        body+=dev[i].cursta;
        if(i<sizeof(dev)/sizeof(dev[0])-1)body+=",";
      }
      Serial.println("?");
      microgear.chat(bsender,String(wifiManager.mgear.Alias)+"#"+String(sender)+"#"+body);
      return;
   }else if(body.indexOf("=")>0){
     String strCh=splitString(body,'=',0);
     String strVal=splitString(body,'=',1);
     if(isValidNumber(strCh)){
      int intCh=strCh.toInt();
      if(isValidNumber(strVal)){
        int intVal=strVal.toInt();
        dev[intCh].cursta=intVal;
        Serial.println(body);
        microgear.chat(bsender,String(wifiManager.mgear.Alias)+"#"+String(sender)+"#"+body);
        return;
      }else if(strVal.equals("!")){
        dev[intCh].cursta=map(dev[intCh].cursta,0,255,255,0);
        Serial.print(intCh);Serial.print("=");Serial.println(dev[intCh].cursta);
        microgear.chat(bsender,String(wifiManager.mgear.Alias)+"#"+String(sender)+"#"+String(intCh)+"="+String(dev[intCh].cursta));
        return;
      }
     }
   }else if(body.indexOf(">")>0){
     String strVal=splitString(body,'>',0);
     String strCh=splitString(body,'>',1);
     int iob[sizeof(dev)/sizeof(dev[0])];
     int count=0;
     for(int i=0;i<sizeof(iob);i++){
      count++;
      String ch=splitString(strCh,',',i);
        if(ch.equals("")) break; 
        if(strVal.equals("!")){
          dev[ch.toInt()].cursta=map(dev[ch.toInt()].cursta,0,255,255,0);
        }
        else dev[ch.toInt()].cursta=strVal.toInt();
     }
     String bbody="a=";
      for(int i=0;i<sizeof(dev)/sizeof(dev[0]);i++){
        bbody+=dev[i].cursta;
        if(i<sizeof(dev)/sizeof(dev[0])-1)bbody+=",";
      }
      microgear.chat(bsender,String(wifiManager.mgear.Alias)+"#"+String(sender)+"#"+bbody);
      Serial.println(body);
      return;
   }
}

void onFoundgear(char *attribute, uint8_t* msg, unsigned int msglen) {
    commander(msg,msglen);
}

void onLostgear(char *attribute, uint8_t* msg, unsigned int msglen) {
    commander(msg,msglen);
}

/* When a microgear is connected, do this */
void onConnected(char *attribute, uint8_t* msg, unsigned int msglen) {
    String s=char2String(msg,msglen);
    Serial.println("online...");
    microgear.setAlias(wifiManager.mgear.Alias);
}

void setup() {
    Serial.begin(SERIAL_BAUD_RATE);
    Serial.println(F("Starting..."));
    Serial.println(F("wait for requirement to clear setting"));
    pinMode(CLEAR_SETTING_TRICKER,INPUT);
    delay(100);
    if(digitalRead(CLEAR_SETTING_TRICKER)==0){
      //reset saved settings
      //wifiManager.emptyGear(); //clear gear config
      wifiManager.resetSettings();
      Serial.println(F("Setting has been cleared!"));
    }
    Serial.println(F("set microgear eepromoffset"));
    microgear.setEEPROMOffset(101);

    wifiManager.initGear();
    //for debuging
    wifiManager.setDebugOutput(false);
    
    //use for save gear param to EEPROM
    //wifiManager.putsGear(myGear);
    
    //set custom ip for portal
    Serial.print(F("setup WiFi...\nif have not last connection, pending to AP Mode with SSDI:"));
    Serial.print(String(DEVICE_NAME));Serial.println(F(" and no password"));
    wifiManager.setAPStaticIPConfig(IPAddress(10,0,0,1), IPAddress(10,0,0,1), IPAddress(255,255,255,0));
    
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
    Serial.print(F("WiFi Connected to "));Serial.println(wifiManager.getSSID());
    Serial.print(F("IPAdress:"));Serial.println(WiFi.localIP());
        
        /* Add Event listeners */
    Serial.println(F("Initialize Microgear..."));
    
    microgear.on(MESSAGE,onMsghandler);
    microgear.on(PRESENT,onFoundgear);
    microgear.on(ABSENT,onLostgear);
    microgear.on(CONNECTED,onConnected);
    
    if(wifiManager.mgear.AppID!=""&&!wifiManager.mgear.Key!=""&&!wifiManager.mgear.Secret!=""){
      canRungear=true;
      Serial.println(F("start Microgear..."));
      if(wifiManager.mgear.Alias=="")
      microgear.init(wifiManager.mgear.Key,wifiManager.mgear.Secret,DEVICE_NAME);
      /* Initial with KEY, SECRET and also set the ALIAS here */
      else microgear.init(wifiManager.mgear.Key,wifiManager.mgear.Secret,wifiManager.mgear.Alias);

      Serial.println("AppID:"+String(wifiManager.mgear.AppID)+"\nKey:"+String(wifiManager.mgear.Key)+"\nSecret:"+String(wifiManager.mgear.Secret)+"\nAlias:"+String(wifiManager.mgear.Alias));
      Serial.println(F("Connecting Microgear to NETPIE..."));
      /* connect to NETPIE to a specific APPID */
      microgear.connect(wifiManager.mgear.AppID);
      microgear.subscribe("/chat");
    }else{
      Serial.println(F("Microgear was silent! because anything is wrong!\nPlease press reset to reboot"));
    }
    
    Serial.print(F("Start WebService port:"));
    Serial.println(String(HTTP_PORT));
    initHandleds();
    server.begin();
    Serial.println(F("done"));
}

void loop() {
    //still running webserver
    server.handleClient();
   
    //if allow to use microgear.
    if(canRungear){
      /* To check if the microgear is still connected */
      unsigned long currentMillis = millis();
      if (currentMillis - pMillis >= (1000*60)*nextTime) {
        if (microgear.connected()) {
            microgear.loop();
            previousMillis = millis();
            nextTime=0;
        }else {
            //if connection lost more than 5s then reconnect.
             if (millis() - previousMillis >= interval) {
                Serial.println(F("Lost Connection, reconnecting...."));
                microgear.connect(wifiManager.mgear.AppID);
            }Serial.print(F("recheck connection in next "));
            nextTime+=1;
            if (microgear.connected()) {
              Serial.print(nextTime);
              Serial.println(F(" minutes"));
            }
        }
        pMillis=currentMillis;
      }
    }
    //Serial.println("test");
    //delay(1000);
    //other ........... .
}

#endif
