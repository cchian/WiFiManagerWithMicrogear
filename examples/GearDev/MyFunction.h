 #ifndef MYFUNCTION
 #define MYFUNCTION
 byte getAvailablePins(byte pin[]){
  byte count=0;
  for(byte i=0;i<sizeof(pin);i++){
    if(pin[i]>0){
      count+=1;
      }
  }
  return count;
}
 byte getAvailablePins(int id){
  byte count=0;
  for(byte i=0;i<sizeof(dev[id].pins)/sizeof(dev[id].pins[0]);i++){
    if(dev[id].pins[i]>0){
      count+=1;
      }
  }
  return count;
}
 String splitString(String data, char separator, int index){
  int found = 0;
  int strIndex[] = {0, -1};
  int maxIndex = data.length()-1;

  for(int i=0; i<=maxIndex && found<=index; i++){
    if(data.charAt(i)==separator || i==maxIndex){
        found++;
        strIndex[0] = strIndex[1]+1;
        strIndex[1] = (i == maxIndex) ? i+1 : i;
    }
  }
  return found>index ? data.substring(strIndex[0], strIndex[1]) : "";
}

boolean isValidNumber(String str){
   boolean isNum=false;
   for(byte i=0;i<str.length();i++)
   {
       isNum = isDigit(str.charAt(i)) || str.charAt(i) == '+' || str.charAt(i) == '.' || str.charAt(i) == '-';
       if(!isNum) return false;
   }
   return isNum;
}

String char2String(uint8_t* msg,unsigned int msglen){
  String msgs;
    for (unsigned int i=0; i<msglen; i++)
        msgs+=(char)msg[i];
   return msgs;
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
#endif
