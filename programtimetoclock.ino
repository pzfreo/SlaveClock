// software to drive a slave clock with a pulse every minute

#define BYTES_TO_STORE 90 // 12 hours * 60 minutes / 8 bits per byte
#include <EEPROM.h>
void setup() {
  Serial.begin(115200);
  Serial.setDebugOutput(true);
  EEPROM.begin(BYTES_TO_STORE);
  
  delay(1000);
  saveClockTimeToEEPROM(300);
  Serial.println("done");


}

void loop() {
  Serial.println("loop");
  delay(1000);
}

char encodingBuf[BYTES_TO_STORE];
void saveClockTimeToEEPROM(int minspast12) 
{
  int b = minspast12 / 8; 
  int r = minspast12 % 8;
  Serial.print(b); Serial.print(" "); Serial.println(r);
  
  for (int i=0; i<BYTES_TO_STORE; i++) {
      if (i < b) {
         encodingBuf[BYTES_TO_STORE-i-1] = 0xFF;
      }
      if (i == b) {
         char y = 0;
         for (int j = 0; j < r ; j++)
         {
            y = ((y<<1) | 1);
         }
         encodingBuf[BYTES_TO_STORE-i-1] = y;
         
      }
      if (i>b) {
        encodingBuf[BYTES_TO_STORE-i-1] = 0;
      }
  }

//  for (int i=0; i<BYTES_TO_STORE; i++) {
//      Serial.printf(BYTE_TO_BINARY_PATTERN,BYTE_TO_BINARY(encodingBuf[i]));
//  }
//  Serial.println();
  for (int i = 0; i< BYTES_TO_STORE; i++) {
    byte current = EEPROM.read(i);
    byte updated = encodingBuf[i];
    if (updated != current) 
    {
      EEPROM.write(i, updated);
    }
  }
  EEPROM.commit();
}
