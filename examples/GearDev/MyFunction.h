#ifndef MYFUNCTION
#define MYFUNCTION
void beep(int duration){
  digitalWrite(LED_BUILTIN, HIGH);   // turn the LED on (HIGH is the voltage level)
  delay(duration);   
  digitalWrite(LED_BUILTIN, LOW);   // turn the LED on (HIGH is the voltage level)
}
byte getAvailablePins(byte pin[]) {
  byte count = 0;
  for (byte i = 0; i < sizeof(pin); i++) {
    if (pin[i] > 0) {
      count += 1;
    }
  }
  return count;
}
byte getAvailablePins(int id) {
  byte count = 0;
  for (byte i = 0; i < sizeof(dev[id].pins) / sizeof(dev[id].pins[0]); i++) {
    if (dev[id].pins[i] > 0) {
      count += 1;
    }
  }
  return count;
}
String splitString(String data, char separator, int index) {
  int found = 0;
  int strIndex[] = {0, -1};
  int maxIndex = data.length() - 1;

  for (int i = 0; i <= maxIndex && found <= index; i++) {
    if (data.charAt(i) == separator || i == maxIndex) {
      found++;
      strIndex[0] = strIndex[1] + 1;
      strIndex[1] = (i == maxIndex) ? i + 1 : i;
    }
  }
  return found > index ? data.substring(strIndex[0], strIndex[1]) : "";
}

boolean isValidNumber(String str) {
  boolean isNum = false;
  for (byte i = 0; i < str.length(); i++)
  {
    isNum = isDigit(str.charAt(i)) || str.charAt(i) == '+' || str.charAt(i) == '.' || str.charAt(i) == '-';
    if (!isNum) return false;
  }
  return isNum;
}

String char2String(uint8_t* msg, unsigned int msglen) {
  String msgs;
  for (unsigned int i = 0; i < msglen; i++)
    msgs += (char)msg[i];
  return msgs;
}

void setPinsValue(int i) {
  //dev[i].cursta=intVal;
#ifdef ARDUINO_AS_IO
  EEPROM.write(i + PREVIOS_STATUS_ADDRESS, dev[i].cursta);
#endif
  byte pin_use = getAvailablePins(i);
  if (pin_use == 1) {
    //ถ้าเป็นขาสัญญาณ pwm
    
    if (
  #ifdef ARDUINO_AS_IO
    #ifdef __AVR_ATmega328P__
          // ใช้ตัวนี้ถ้าเป็นบอร์ด Arduino ที่ใช้ชิป Atmega328p
          (dev[i].pins[0] == 3 || dev[i].pins[0] == 5 || dev[i].pins[0] == 6 || dev[i].pins[0] == 9 || dev[i].pins[0] == 10 || dev[i].pins[0] == 11)
    #elif __AVR_ATmega1280__ || __AVR_ATmega2560__
          // ใช้ตัวนี้ถ้าเป็นบอร์ด Arduino ที่ใช้ชิป Atmega1280 หรือ Atmega2560
          ((dev[i].pins[0] >= 2 && dev[i].pins[0] <= 13) || (dev[i].pins[0] > 44 && dev[i].pins[0] <= 46))
    #else
          false
    #endif
      &&
  #else
      //true ||
  #endif
      (dev[i].cursta < 255 && dev[i].cursta > 0)) {
      //ให้ใช้ analogWrite()
      //Serial.println(dev[i].cursta);
      analogWrite(dev[i].pins[0], dev[i].cursta);

    } else {
      //ถ้าไม่ใช่ให้คำนวนค่าที่จะใช้กับ digitalWrite() เนื่องจากค่าที่ส่งมาอยู่ในช่วง 0-255
      #ifdef ARDUINO_ESP8266_NODEMCU
      analogWrite(dev[i].pins[0], dev[i].cursta);
      #else
      Serial.println(dev[i].cursta);
      digitalWrite(dev[i].pins[0], map(dev[i].cursta, 0, 255, 0, 1));//use this for active low opto.
      #endif
    }
  } else if (pin_use == 2) {
    /*chanel นี้ใช้ IO 2 ขา มี 3 status คือ เปิด2/เปิด1/ปิด ลักษณะการกำหนดสถานะของขา IO จออ้างอิงตามการเขียนเลขฐานสอง
      dev[i].pins[1] | dev[i].pin[0]
            0                0         =       ปิด
            0                1         =       เปิด1
            1                0         =       เปิด2
            1                1         =       N/A (ไม่ได้กำหนดให้มี status นี้)
    */
    if (dev[i].cursta <= 84) {
      //0 - 84
      digitalWrite(dev[i].pins[0], 0);
      digitalWrite(dev[i].pins[1], 0);
    } else if (dev[i].cursta <= 169) {
      //85 - 169
      digitalWrite(dev[i].pins[0], 1);
      digitalWrite(dev[i].pins[1], 0);
    } else {
      //170 - 255
      digitalWrite(dev[i].pins[0], 0);
      digitalWrite(dev[i].pins[1], 1);
    }
  } else if (pin_use == 3) {
    /*chanel นี้ใช้ IO 3 ขา มี 4 status คือ เปิด3/เปิด2/เปิด1/ปิด ลักษณะการกำหนดสถานะของขา IO จออ้างอิงตามการเขียนเลขฐานสอง
      dev[i].pins[2] | dev[i].pins[1] | dev[i].pin[0]
            0                0                0         =       ปิด
            0                0                1         =       เปิด1
            0                1                0         =       เปิด2
            1                0                0         =       เปิด3
            1                1                1         =       N/A (ไม่ได้กำหนดให้มี status นี้)
    */
    if (dev[i].cursta <= 63) {
      //0-63
      digitalWrite(dev[i].pins[0], 0);
      digitalWrite(dev[i].pins[1], 0);
      digitalWrite(dev[i].pins[2], 0);
    } else if (dev[i].cursta <= 127) {
      //64-127
      digitalWrite(dev[i].pins[0], 1);
      digitalWrite(dev[i].pins[1], 0);
      digitalWrite(dev[i].pins[2], 0);
    } else if (dev[i].cursta <= 191) {
      //128-191
      digitalWrite(dev[i].pins[0], 0);
      digitalWrite(dev[i].pins[1], 1);
      digitalWrite(dev[i].pins[2], 0);
    } else {
      //192-255
      digitalWrite(dev[i].pins[0], 0);
      digitalWrite(dev[i].pins[1], 0);
      digitalWrite(dev[i].pins[2], 1);
    }
  } else if (pin_use == 4) {
    /*chanel นี้ใช้ IO 4 ขา มี 5 status คือ เปิด4/เปิด3/เปิด2/เปิด1/ปิด ลักษณะการกำหนดสถานะของขา IO จออ้างอิงตามการเขียนเลขฐานสอง
      dev[i].pins[3] | dev[i].pins[2] | dev[i].pins[1] | dev[i].pin[0]
            0                0                0                0         =       ปิด
            0                0                0                1         =       เปิด1
            0                0                1                0         =       เปิด2
            0                1                0                0         =       เปิด3
            1                0                0                0         =       เปิด4
            1                1                1                1         =       N/A (ไม่ได้กำหนดให้มี status นี้)
    */
    if (dev[i].cursta <= 51) {
      //0-51
      digitalWrite(dev[i].pins[0], 0);
      digitalWrite(dev[i].pins[1], 0);
      digitalWrite(dev[i].pins[2], 0);
      digitalWrite(dev[i].pins[3], 0);
    } else if (dev[i].cursta <= 102) {
      //52-102
      digitalWrite(dev[i].pins[0], 1);
      digitalWrite(dev[i].pins[1], 0);
      digitalWrite(dev[i].pins[2], 0);
      digitalWrite(dev[i].pins[3], 0);
    } else if (dev[i].cursta <= 153) {
      //103-153
      digitalWrite(dev[i].pins[0], 0);
      digitalWrite(dev[i].pins[1], 1);
      digitalWrite(dev[i].pins[2], 0);
      digitalWrite(dev[i].pins[3], 0);
    } else if (dev[i].cursta <= 204) {
      //154-204
      digitalWrite(dev[i].pins[0], 0);
      digitalWrite(dev[i].pins[1], 0);
      digitalWrite(dev[i].pins[2], 1);
      digitalWrite(dev[i].pins[3], 0);
    } else {
      //205-255
      digitalWrite(dev[i].pins[0], 0);
      digitalWrite(dev[i].pins[1], 0);
      digitalWrite(dev[i].pins[2], 0);
      digitalWrite(dev[i].pins[3], 1);
    }
  }
}

void serialEvent() {
  while (Serial.available()) {
    char inChar = (char)Serial.read();
    if (inChar == '\n') {
      stringComplete = true;
      return;
    }
    inputString += inChar;
  }
}

#if defined(__AVR_ATmega1280__) || defined(__AVR_ATmega2560__)
void serialEvent1() {
  while (Serial2.available()) {
    char inChar = (char)Serial2.read();
    if (inChar == '\n') {
      stringComplete = true;
      return;
    }
    inputString += inChar;
  }
}
#endif


void endableRestorePreviosPinState(boolean b) {
  if (b)EEPROM.write(0, 1);
  else EEPROM.write(0, 0);
}

void processCommand() {
  String body = inputString;
  if (body.startsWith("a")) {
    String strVal = splitString(body, '=', 1);
    if (isValidNumber(strVal)) {
      int intVal = strVal.toInt();
      if (intVal >= 0 && intVal <= 255) {
        for (int i = 0; i < sizeof(dev) / sizeof(dev[0]); i++) {
          //chanel นี้ใช้ IO 1 ขา มี 2 status คือ ปิด/เปิด หรือ ถ้ารองรับ pwm ก็สามารถกำหนดค่าให้ได้ในช่วง 0-255
          dev[i].cursta = intVal;
          setPinsValue(i);
        }
      }
    }
  } else if (body.startsWith("enr")) {
    String strVal = splitString(body, '=', 1);
    if (strVal.equals("")) {
#if defined(__AVR_ATmega1280__) || defined(__AVR_ATmega2560__)
      Serial2.println("enr=" + String(EEPROM.read(0)));
#endif
      Serial.println("enr=" + String(EEPROM.read(0)));
    } else if (strVal.equals("true") || strVal.equals("1")) {
      endableRestorePreviosPinState(true);
    } else {
      endableRestorePreviosPinState(false);
    }
  } else if (body.startsWith("msg")) {
#ifdef ARDUINO_ESP8266_NODEMCU
    Serial.println("MSG:");
    body.replace("msg:", "");
    Serial.println("\t" + body);
    webSocket.broadcastTXT("Hello" + body);
#endif
  } else if (body.startsWith("?")) {
    body = "a=";
    for (int i = 0; i < sizeof(dev) / sizeof(dev[0]); i++) {
      body += dev[i].cursta;
      if (i < sizeof(dev) / sizeof(dev[0]) - 1)body += ",";
    }
#if defined(__AVR_ATmega1280__) || defined(__AVR_ATmega2560__)
    Serial2.println(body);
#endif
    Serial.println(body);
  } else if (body.indexOf("=") > 0) {
    String strCh = splitString(body, '=', 0);
    String strVal = splitString(body, '=', 1);
    if (isValidNumber(strCh)) {
      int intCh = strCh.toInt();
      if (isValidNumber(strVal)) {
        int intVal = strVal.toInt();
        dev[intCh].cursta = intVal;
        setPinsValue(intCh);
      } else if (strVal.equals("!")) {
        dev[intCh].cursta = map(dev[intCh].cursta, 0, 255, 255, 0);
        setPinsValue(intCh);
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
      } else {
        dev[ch.toInt()].cursta = strVal.toInt();
      }
      setPinsValue(ch.toInt());
    }
  }
#if defined(ARDUINO_AS_IO) &&  defined(__AVR_ATmega328P__) || defined(__AVR_ATmega1280__) || defined(__AVR_ATmega2560__)
  else if (body.startsWith("RUN:")) {
    body.replace("RUN:", "");
    STA = body.toInt();
  } else if (body.equals("done")) {
    tone(13, NOTE_CS7, 20);
    delay(80);
    tone(13, NOTE_CS7, 20);
    espboot = true;
  } else if (body.equals("r")) {
    //digitalWrite(THIS_RESET_TRICKER, 0);
    delay(5000);
    software_Reset();
  } else {
#ifndef ARDUINO_ESP8266_NODEMCU
#if defined(__AVR_ATmega1280__) || defined(__AVR_ATmega2560__)
    Serial2.println("not a command!");
#endif
    Serial.println("not a command!");
#endif
  }
#endif
}
#endif
