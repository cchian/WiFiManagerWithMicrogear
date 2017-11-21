/*  WiFiManager with NETPIE ESP8266 basic sample                                     */
/*  More information visit : https://github.com/cchian/WiFiManagerWithMicrogear      */
#include<WiFiManager.h>
#include <WiFiClient.h>
#include <MicroGear.h>
/* for
struct MicrogearParameter {
  char AppID[32];
  char Key[32];
  char Secret[32];
  char Alias[32];
};*/

WiFiManager wifiManager;
WiFiClient client;
MicroGear microgear(client);

/* If a new message arrives, do this */
void onMsghandler(char *topic, uint8_t* msg, unsigned int msglen) {
    Serial.print("Incoming message --> ");
    msg[msglen] = '\0';
    Serial.println((char *)msg);
}

void onFoundgear(char *attribute, uint8_t* msg, unsigned int msglen) {
    Serial.print("Found new member --> ");
    for (int i=0; i<msglen; i++)
        Serial.print((char)msg[i]);
    Serial.println();  
}

void onLostgear(char *attribute, uint8_t* msg, unsigned int msglen) {
    Serial.print("Lost member --> ");
    for (int i=0; i<msglen; i++)
        Serial.print((char)msg[i]);
    Serial.println();
}

/* When a microgear is connected, do this */
void onConnected(char *attribute, uint8_t* msg, unsigned int msglen) {
    Serial.println("Connected to NETPIE...");
    /* Set the alias of this microgear ALIAS */
    microgear.setAlias(wifiManager.mgear.Alias);
}

void setup() {
  
  Serial.begin(115200);

  microgear.setEEPROMOffset(101);
 /* Call onMsghandler() when new message arraives */
  microgear.on(MESSAGE,onMsghandler);

  /* Call onFoundgear() when new gear appear */
  microgear.on(PRESENT,onFoundgear);

  /* Call onLostgear() when some gear goes offline */
  microgear.on(ABSENT,onLostgear);

  /* Call onConnected() when NETPIE connection is established */
  microgear.on(CONNECTED,onConnected);

  wifiManager.initGear();
  //for debuging
  wifiManager.setDebugOutput(false);

  /*clear gear config*/
  //wifiManager.emptyGear(); 
  
  /*reset WiFiManager Setting*/
  //wifiManager.resetSettings();
  
  /*use for save gear param to EEPROM*/
  //wifiManager.putsGear(myGear);

  //set static ip IPAddress(IPAddress, GateWay, Netmask)
  wifiManager.setAPStaticIPConfig(IPAddress(10,0,0,1), IPAddress(10,0,0,1), IPAddress(255,255,255,0));

  Serial.print("AppID:");Serial.println(wifiManager.mgear.AppID);
  Serial.print("Key:");Serial.println(wifiManager.mgear.Key);
  Serial.print("Secret:");Serial.println(wifiManager.mgear.Secret);
  Serial.print("Alias:");Serial.println(wifiManager.mgear.Alias);

  //fetches ssid and pass from eeprom and tries to connect
  //if it does not connect it starts an access point with the specified name
  //here  "WM_MG"
  //and goes into a blocking loop awaiting configuration
  wifiManager.autoConnect("WM_MG");
  //or use this for auto generated name ESP + ChipID
  //wifiManager.autoConnect();

  /*init and connect microgear with WiFiManager Setting (Key, Secret, Alias)*/
  microgear.init(wifiManager.mgear.Key,wifiManager.mgear.Secret,wifiManager.mgear.Alias);
  microgear.connect(wifiManager.mgear.AppID);
  microgear.subscribe("/chat");

  // put your setup code here, to run once:
  
}

int timer = 0;
void loop() {
  /* To check if the microgear is still connected */
    if (microgear.connected()) {
        Serial.println("connected");

        /* Call this method regularly otherwise the connection may be lost */
        microgear.loop();

        if (timer >= 1000) {
            Serial.println("Publish...");

            /* Chat with the microgear named ALIAS which is myself */
            microgear.chat(wifiManager.mgear.Alias,"Hello");
            timer = 0;
        } 
        else timer += 100;
    }
    else {
        Serial.println("connection lost, reconnect...");
        if (timer >= 5000) {
            microgear.connect(wifiManager.mgear.AppID);
            timer = 0;
        }
        else timer += 100;
    }
    delay(100);
}
