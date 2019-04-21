// software to drive a slave clock with a pulse every minute

#include <ESP8266WiFi.h>
#include <EEPROM.h>
#include <Ticker.h>
#include <time.h>
#include <simpleDSTadjust.h>

// -------------- Configuration options -----------------

 
#define NTP_UPDATE_INTERVAL_SEC 3600 // once an hour

// Maximum of 3 servers
#define NTP_SERVERS "1.uk.pool.ntp.org", "0.uk.pool.ntp.org", "time.nist.gov"

#define timezone 0 
struct dstRule StartRule = {"BST", Last, Sun, Mar, 1, 3600};  
struct dstRule EndRule = {"GMT", Last, Sun, Oct, 1, 0};       

const char* ssid = "SSID";
const char* password = "PASSWORD";

// --------- End of configuration section ---------------

Ticker ticker1;
int32_t tick;

// flag changed in the ticker function to start NTP Update
bool readyForNtpUpdate = false;

// Setup simpleDSTadjust Library rules
simpleDSTadjust dstAdjusted(StartRule, EndRule);

int te;
void setup() {
  Serial.begin(115200);
  Serial.setDebugOutput(true);
  EEPROM.begin(BYTES_TO_STORE);
  delay(1000);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.println("\nConnecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(1000);
  }
  delay(1000);
  Serial.println("\nDone");
  updateNTP(); // Init the NTP time
  updateNTP(); // Init the NTP time  
  printTime(0); // print initial time time now.

  tick = NTP_UPDATE_INTERVAL_SEC; // Init the NTP update countdown ticker
  ticker1.attach(1, secTicker); // Run a 1 second interval Ticker
  Serial.print("Next NTP Update: ");
  printTime(tick);
  te = 0;
}

void loop()
{
  if(readyForNtpUpdate)
   {
    readyForNtpUpdate = false;
    printTime(0);
    updateNTP();
    Serial.print("\nUpdated time from NTP Server: ");
    printTime(0);
    Serial.print("Next NTP Update: ");
    printTime(tick);
  }

  int encodedMinutes = getClockMinutesPastTwelve();
  Serial.printf("encodedMinutes %d \n", encodedMinutes);
  int currentTMP12 = (getCurrentTimeHours(0)*60)+getCurrentTimeMinutes(0);
  Serial.printf("current %d \n", currentTMP12);
  int minutes2move = (720+currentTMP12-encodedMinutes)%720;
  Serial.printf("move %d \n", minutes2move);
  if (minutes2move > 0 && minutes2move < 700) // don't update if its 20 minutes fast - just wait
  {
    Serial.println("PULSE");
    pulseAndWait();
  }

  
  
  delay(100);  // to reduce upload failures
//  Serial.printf("real: %d:%d \n", getCurrentTimeHours(0), getCurrentTimeMinutes(0));
  
  
//  Serial.printf("save: %d:%d \n", encodedMinutes / 60, encodedMinutes % 60);
  
}

void pulseAndWait()
{
//  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(D7, OUTPUT);
  
//  digitalWrite(LED_BUILTIN, LOW);
  digitalWrite(D7, HIGH);
  delay(150);
  digitalWrite(D7, LOW);
//  digitalWrite(LED_BUILTIN, HIGH);
  delay(800);
  saveClockTimeToEEPROM((getClockMinutesPastTwelve()+1)%720);  
}


//----------------------- Functions -------------------------------


// NTP timer update ticker
void secTicker()
{
  tick--;
  if(tick<=0)
   {
    readyForNtpUpdate = true;
    tick= NTP_UPDATE_INTERVAL_SEC; // Re-arm
   }

//   printTime(0);  // Uncomment if you want to see time printed every second
}


void updateNTP() {
  
  configTime(timezone * 3600, 0, NTP_SERVERS);

  delay(1000);
  while (!time(nullptr)) {
    Serial.print("#");
    delay(1000);
  }
}

int getCurrentMinutesPastTwelve(time_t offset) {
  char *dstAbbrev;
  time_t t = dstAdjusted.time(&dstAbbrev)+offset;
  struct tm *timeinfo = localtime (&t);
  return (60*((timeinfo->tm_hour)%12))+timeinfo->tm_min;  // take care of noon and midnight
}

int getCurrentTimeHours(time_t offset) // 12 o'clock is 0 in this world
{
  char *dstAbbrev;
  time_t t = dstAdjusted.time(&dstAbbrev)+offset;
  struct tm *timeinfo = localtime (&t);
  
  return (timeinfo->tm_hour)%12;  // take care of noon and midnight
}

int getCurrentTimeMinutes(time_t offset) 
{
  char *dstAbbrev;
  time_t t = dstAdjusted.time(&dstAbbrev)+offset;
  struct tm *timeinfo = localtime (&t);
  return timeinfo->tm_min;
}

int getSeconds(time_t offset) 
{
  char *dstAbbrev;
  time_t t = dstAdjusted.time(&dstAbbrev)+offset;
  struct tm *timeinfo = localtime (&t);
  return timeinfo->tm_sec;
}


void printTime(time_t offset) {
  char buf[40];
  int h = getCurrentTimeHours(offset);
  int m = getCurrentTimeMinutes(offset);
  int mp12 = getCurrentMinutesPastTwelve(offset);
  sprintf(buf, "%02d,%02d,%04d\n", h,m,mp12);
  Serial.print(buf);
//  char *x = getEncoding(mp12);
}

//void printTime(time_t offset)
//{
//  char buf[30];
//  char *dstAbbrev;
//  time_t t = dstAdjusted.time(&dstAbbrev)+offset;
//  struct tm *timeinfo = localtime (&t);
//  
//  int hour = (timeinfo->tm_hour+11)%12+1;  // take care of noon and midnight
//  sprintf(buf, "%02d/%02d/%04d %02d:%02d:%02d%s %s\n",timeinfo->tm_mon+1, timeinfo->tm_mday, timeinfo->tm_year+1900, hour, timeinfo->tm_min, timeinfo->tm_sec, timeinfo->tm_hour>=12?"pm":"am", dstAbbrev);
//  Serial.print(buf);
//}


//-----------------------------------------

// EEPROM handling
#define BYTES_TO_STORE 90 // 12 hours * 60 minutes / 8 bits per byte

#define BYTE_TO_BINARY_PATTERN "%c%c%c%c%c%c%c%c"
#define BYTE_TO_BINARY(byte)  \
  (byte & 0x80 ? '1' : '0'), \
  (byte & 0x40 ? '1' : '0'), \
  (byte & 0x20 ? '1' : '0'), \
  (byte & 0x10 ? '1' : '0'), \
  (byte & 0x08 ? '1' : '0'), \
  (byte & 0x04 ? '1' : '0'), \
  (byte & 0x02 ? '1' : '0'), \
  (byte & 0x01 ? '1' : '0') 

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

int getMinutesFromMP12(int mp12) {
  return mp12 % 60;
}

int getHoursFromMP12(int mp12) {
  return mp12 / 60;
}

int getClockMinutesPastTwelve() { // we will simply deal in a number from 0 (midday / midnight) to 719 (11:59)
  byte buffer[BYTES_TO_STORE];

  for (int i = 0; i< BYTES_TO_STORE; i++)
  {
    buffer[i] = EEPROM.read(i);
  }
  
  int numBits = 0;
  for (int i = 0; i < BYTES_TO_STORE; i++) {
      
      byte c = buffer[i];
      int b = bitsIn(c);
      if (b==-1) {
        return -1; // error
      }
      numBits += b;
  }
  return numBits;
}

int bitsIn(byte c) {
  if (c == 255) return 8;
  if (c == 127) return 7;
  if (c == 63) return 6;
  if (c == 31) return 5;
  if (c == 15) return 4;
  if (c == 7) return 3;
  if (c == 3) return 2;
  if (c == 1) return 1;
  if (c == 0) return 0;
  return -1; // error
}

void printByte(byte x) {
  for (int b = 7; b >= 0; b--)
  {
    Serial.print(bitRead(x, b));
  }
}
