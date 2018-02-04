#if defined(__AVR_ATmega328P__) || defined(__AVR_ATmega1280__) || defined(__AVR_ATmega2560__)
/* การสั่งงานบอร์ด ต้องกระทำผ่าน Serial โดย แพทเทิลของข้อความที่จะส่งมา ดูได้ในหน้า Readme */

void setup() {
  //pinMode(THIS_RESET_TRICKER, OUTPUT);
  //digitalWrite(THIS_RESET_TRICKER, 1);
  pinMode(13,OUTPUT);
  delay(500);
  pinMode(RESET_TRICKER, OUTPUT);
  pinMode(CLEAR_SETTING_TRICKER, OUTPUT);
  digitalWrite(CLEAR_SETTING_TRICKER, 1);
#if defined(__AVR_ATmega1280__) || defined(__AVR_ATmega2560__)
  Serial2.begin(SERIAL_BAUD_RATE);
#endif
  Serial.begin(SERIAL_BAUD_RATE);
  Serial.println("STARTING!!!");

  resetSwitch.setLongHoldTime(5000); //assign long hold time to 5s
  resetSwitch.eventClick(resetSwitchClick);
  resetSwitch.eventHold(resetSwitchHold);
  resetSwitch.eventLongHold(resetSwitchLongHold);

  for (int i = 0; i < sizeof(dev) / sizeof(dev[0]); i++)
    for (byte j = 0; j < getAvailablePins(i); j++)
      pinMode(dev[i].pins[j], OUTPUT);
  if (EEPROM.read(0) > 0)
    for (int i = 0; i < sizeof(dev) / sizeof(dev[0]); i++)
      dev[i].cursta = EEPROM.read(i + PREVIOS_STATUS_ADDRESS);
  resetESP();
  //tone(13, NOTE_C6, 40);
  beep(40);
  Serial.println("Ready!");
}

void software_Reset() { // Restarts program from beginning but does not reset the peripherals and registers
  asm volatile ("  jmp 0");
}

void resetSwitchClick(int sender) {
  //nomal reset
  resetESP();
  //digitalWrite(THIS_RESET_TRICKER, 0);
  software_Reset();
}
void resetESP() {
  digitalWrite(RESET_TRICKER, 0);
  delay(100);
  digitalWrite(RESET_TRICKER, 1);
}

void resetSwitchHold(int sender) {

}
void resetSwitchLongHold(int sender) {
  resetESPWithClearSetting();
  //tone(13, NOTE_A6, 120);
  beep(500);
  delay(500);
  //digitalWrite(THIS_RESET_TRICKER, 0);
  software_Reset();
}
void resetESPWithClearSetting() {
  //if reset switch is longhol for 10 clear setting and reset

  digitalWrite(CLEAR_SETTING_TRICKER, 0);
  resetESP();
  delay(500);
  digitalWrite(CLEAR_SETTING_TRICKER, 1);
  Serial.println("Reset ESP8266 with request Clear Setting");
}

void loop() {
  unsigned long currentMillis = millis();
  resetSwitch.handleButton();

  //check esp8266 running status
  if (currentMillis - pRunning >= (sta_running_duration)) {
    Serial.print("check esp is running:");
    if (espboot) {
      Serial.println(STA);
      if (STA > 0) {
        STA -= 1;
      } else {
        //resetSwitchClick(0);
        //espboot=false;
        //tone(13, NOTE_C6, 100);
        beep(100);
        resetESP();
        //tone(13, NOTE_CS6, 100);
        beep(100);
        //digitalWrite(THIS_RESET_TRICKER, 0);
      }
    } else {
      Serial.println("booting yet!");
    }
    pRunning = currentMillis;
  }

#if defined(__AVR_ATmega1280__) || defined(__AVR_ATmega2560__)
  serialEvent1();
  serialEvent();
#endif
  if (stringComplete) {
#if defined(__AVR_ATmega1280__) || defined(__AVR_ATmega2560__)
    Serial.println("cmd:\"" + inputString + "\"");
#endif
    inputString.trim();
    processCommand();
    inputString = "";
    stringComplete = false;
  }
  //Serial.println("e");

}
























#else
#ifndef ARDUINO_ESP8266_NODEMCU
#error "Please specify other board for this project!"
#endif
#endif
