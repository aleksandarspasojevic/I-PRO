

#include <Adafruit_CC3000.h>
#include <ccspi.h>
#include <SPI.h>
#include <string.h>
#include "utility/debug.h"
#include <avr/pgmspace.h>
#include <avr/wdt.h>

//#define developerMode
//#define serialMode 

// These are the interrupt and control pins
#define ADAFRUIT_CC3000_IRQ   3  // MUST be an interrupt pin!
// These can be any two pins
#define ADAFRUIT_CC3000_VBAT  5
#define ADAFRUIT_CC3000_CS    10
// Use hardware SPI for the remaining pins
// On an UNO, SCK = 13, MISO = 12, and MOSI = 11
Adafruit_CC3000 cc3000 = Adafruit_CC3000(ADAFRUIT_CC3000_CS, ADAFRUIT_CC3000_IRQ, ADAFRUIT_CC3000_VBAT,
                                         SPI_CLOCK_DIVIDER); // you can change this clock speed

#define WLAN_SSID       "ALEXANDAR"       // cannot be longer than 32 characters!
#define WLAN_PASS       "123581321"
// Security can be WLAN_SEC_UNSEC, WLAN_SEC_WEP, WLAN_SEC_WPA or WLAN_SEC_WPA2
#define WLAN_SECURITY   WLAN_SEC_WPA2

#define IDLE_TIMEOUT_MS  5000      // Amount of time to wait (in milliseconds) with no data 
                                   // received before closing the connection.  If you know the server
                                   // you're accessing is quick to respond, you can reduce this value.

// What page to grab!
#define WEBSITE      "api.thingspeak.com"
#define CORONAWEBPAGE      "/apps/thinghttp/send_request?api_key=01ZOMOSH6SFGZ1BM"
//#define WEATHERWEBPAGE      "/apps/thinghttp/send_request?api_key=9EQS42DETPICD6BR"
#define TEMPWEBPAGE "/apps/thinghttp/send_request?api_key=YOFD3IZX2RQB59U0"
#define CURRENTTIME "/apps/thinghttp/send_request?api_key=WT41X2OFVKLIT5OG"
#define FACTWEBPAGE "/apps/thinghttp/send_request?api_key=FGXEUU7YPAHBHKH3"
#define WEATHERWEBPAGE "/apps/thinghttp/send_request?api_key=D48K5KT1N4ES7RYL"
uint32_t ip =  64934711;  //ip adress of WEBSITE




const byte load = 8;
const byte maxInUse = 2;    //change this variable to set how many MAX7219's you'll use


// define max7219 registers
const byte max7219_reg_noop        = 0x00;
const byte max7219_reg_decodeMode  = 0x09;
const byte max7219_reg_intensity   = 0x0a;
const byte max7219_reg_scanLimit   = 0x0b;
const byte max7219_reg_shutdown    = 0x0c;
const byte max7219_reg_displayTest = 0x0f;

void maxAll (byte reg, byte col) {    // initialize  all  MAX7219's in the system
  SPI.beginTransaction(SPISettings(10000000, MSBFIRST, SPI_MODE0));
  digitalWrite(load, LOW);  // begin     
  for (int c = 1; c <= maxInUse; c++) {
  SPI.transfer(reg);  // specify register
  SPI.transfer(col);//((data & 0x01) * 256) + data >> 1); // put data
    }
  digitalWrite(load, LOW);
  digitalWrite(load,HIGH);
}

void maxOne(byte maxNr, byte reg, byte col) { 
  SPI.beginTransaction(SPISettings(10000000, MSBFIRST, SPI_MODE0));
  //maxOne is for adressing different MAX7219's, 
  //whilele having a couple of them cascaded

  digitalWrite(load, LOW);  // begin     

  for (byte c = maxInUse; c > maxNr; c--) {
    SPI.transfer(0);    // means no operation
    SPI.transfer(0);    // means no operation
  }

  SPI.transfer(reg);  // specify register
  SPI.transfer(col);//((data & 0x01) * 256) + data >> 1); // put data 

  for (byte c = maxNr-1; c >= 1; c--) {
    SPI.transfer(0);    // means no operation
    SPI.transfer(0);    // means no operation
  }

  digitalWrite(load, LOW); // and load da shit
  digitalWrite(load,HIGH); 
}

void softwareReset( uint8_t prescaller) {
  // start watchdog with the provided prescaller
  wdt_enable( prescaller);
  // wait for the prescaller time to expire
  // without sending the reset signal by using
  // the wdt_reset() method
  while(1) {}
}

void initScreen(){
  
  SPI.begin();
  pinMode(load,   OUTPUT);

  //beginSerial(9600);  

  //initiation of the max 7219
  maxAll(max7219_reg_scanLimit, 0x06);      
  maxAll(max7219_reg_decodeMode, 0x00);  // using an led matrix (not digits)
  maxAll(max7219_reg_shutdown, 0x01);    // not in shutdown mode
  maxAll(max7219_reg_displayTest, 0x00); // no display test
  for (byte e=1; e<=8; e++) {    // empty registers, turn all LEDs off 
    maxAll(e,(e%2 ? 1365:2730));
  }
  maxAll(max7219_reg_intensity, 0x0a);    // the first 0x0f is the value you can set
                                                  // range: 0x00 to 0x0f
}

uint16_t matrix[7] = {};

void setup(void){
  
  #ifdef serialMode
  Serial.begin(115200);
  #endif
  initScreen();
  
  #ifdef developerMode
  Serial.println(F("Hello, CC3000!\n")); 
  Serial.print("Free RAM: "); Serial.println(getFreeRam(), DEC);
  
  Serial.println(F("\nInitializing..."));
  #endif
  if (!cc3000.begin())
  {
    #ifdef developerMode
      Serial.println(F("Couldn't begin()! Check your wiring?"));
    #endif
    while(1);
  }

  matrix[1]=96, matrix[2]=144, matrix[3]=360, matrix[4]=660, matrix[5]=96, matrix[6]=96;
  updateScreen();
  #ifdef developerMode
  Serial.print(F("\nAttempting to connect to ")); Serial.println(WLAN_SSID);
  #endif
  if (!cc3000.connectToAP(WLAN_SSID, WLAN_PASS, WLAN_SECURITY)) {
    #ifdef developerMode
    Serial.println(F("Failed!"));
    #endif
    while(1);
  }

  #ifdef developerMode
  Serial.println(F("Connected!"));
  

  
  Serial.println(F("Request DHCP"));
  #endif
  while (!cc3000.checkDHCP())
  {
    delay(100); // ToDo: Insert a DHCP timeout!
  }  
  
  while (! displayConnectionDetails()) {
    /*
    for(byte i = 0; i<8; i++){
      matrix[i] = 4095;
    }
    updateScreen();
    */
    softwareReset( WDTO_1S);
  }
   
}




inline void updateScreen(){
  for(byte i=1; i<=7;i++){
    maxOne(2, i, matrix[i-1]&255);
    maxOne(1, i, (matrix[i-1]&3840) >> 4);
  }
}


const byte PROGMEM fontWidth[70] = {
  5,  //0
  5, 
  5, 
  5, 
  5,
  5, 
  5, 
  5, 
  5,
  5,  //9

  5,  //A
  5,
  5,
  5, 
  5, 
  5, 
  5, 
  5, 
  4, 
  5, //J 
  5, 
  4, //L
  6, 
  6, 
  5, 
  5, 
  6,
  5,
  5,  //S
  6, 
  5,
  6, 
  6, 
  6, 
  6,
  6,  //Z

  5,  //a
  5, 
  5, 
  5, 
  5, 
  5,   //f
  5, 
  5, 
  2, 
  4,  //j
  4, 
  5,  //l
  6,
  5, 
  5, 
  4, 
  5, 
  5,  //r
  5, 
  5, 
  5, 
  6, 
  6, 
  6, 
  6, 
  6, //z
  
  3, //,
  2, //.
  4, //stepen
  2, //:
  4, //-
  6, //%
  1, //'
  2 //space
  
};
const byte PROGMEM font[70][7] = {
  30, 18, 18, 18, 18, 18, 30,    //0
  4, 12, 20, 4, 4, 4, 14,
  30, 2, 2, 30, 16, 16, 30,
  30, 2, 2, 30, 2, 2, 30,
  18, 18, 18, 30, 2, 2, 2,
  30, 16, 16, 30, 2, 2, 30, 
  30, 16, 16, 30, 18, 18, 30,
  30, 2, 4, 8, 8, 8, 8, 
  30, 18, 18, 30, 18, 18 ,30,
  30, 18, 18, 30, 2, 2, 30,      //9
  12, 18, 18, 30, 18, 18, 18,    //A
  28, 18, 18, 28, 18, 18, 28, 
  12, 18, 16, 16, 16, 18, 12,
  28, 18, 18, 18, 18, 18, 28, 
  30, 16, 16, 28, 16, 16, 30, 
  30, 16, 16, 28, 16, 16, 16, 
  12, 18, 16, 16, 22, 18, 12,
  18, 18, 18, 30 ,18, 18, 18,
  28, 8, 8, 8, 8, 8, 28,
  14, 4, 4, 4, 4, 20, 8,
  18, 20, 24, 16, 24, 20, 18,
  16, 16, 16, 16, 16, 16, 30,
  17, 27, 21, 17, 17, 17, 17, 
  17, 17, 25, 21, 19, 17, 17,
  12, 18, 18, 18, 18, 18, 12,
  28, 18, 18, 28, 16, 16, 16, 
  14, 17, 17, 17, 21, 18, 13, 
  28, 18, 18, 28, 24, 20, 18,
  14, 16, 16, 12, 2, 2, 28, 
  31, 4, 4, 4, 4, 4, 4, 
  18, 18, 18, 18, 18, 18, 12,
  17, 17, 17, 17, 17, 10, 4,
  17, 17, 17, 21, 21, 21, 10,
  17, 17, 10, 4, 10, 17, 17,
  17, 17, 10, 4, 4, 4, 4,
  31, 1, 2, 4, 8, 16, 31,    //Z
  0, 0, 12, 2, 14, 18, 14,   //a
  0, 16, 16, 28, 18, 18, 28,
  0, 0, 12, 18, 16, 16, 12,
  0, 2, 2, 14, 18, 18, 14,
  0, 0, 12, 18, 28, 16, 12,
  0, 4, 10, 8, 28, 8, 8,
  0, 0, 12, 18, 14, 2, 12,
  0, 16, 16, 20, 26, 18, 18,
  0, 16, 0, 16, 16, 16, 16,
  0, 4, 0, 4, 4, 20, 8, 
  0, 16, 16, 20, 24, 20, 18,
  0, 8, 8, 8, 8, 10, 4,
  0, 0, 26, 21, 21, 21, 21, 
  0, 0, 28, 18, 18, 18, 18, 
  0, 0, 12, 18, 18, 18, 12,
  0, 0, 24, 20, 24, 16, 16,
  0, 0, 12, 18, 14, 2, 2,
  0, 0, 28, 18, 16, 16, 16,
  0, 0, 14, 16, 12, 2, 28,
  0, 8, 28, 8, 8, 10, 4, 
  0, 0, 18, 18, 18, 22, 10,
  0, 0, 17, 17, 17, 10, 4,
  0, 0, 17, 17, 21, 21, 10, 
  0, 0, 17, 10, 4, 10, 17,
  0, 0, 18, 18, 14, 2, 14,
  0, 0, 30, 2, 4, 8, 30,   //z
  0, 0, 0, 0, 0, 8, 16,     //,
  0, 0, 0, 0, 0, 0, 16,     //.
  8, 20, 8, 0, 0, 0, 0,     //stepen
  0, 0, 16, 0, 16
  , 0, 0,      //:
  0, 0, 0, 28, 0, 0, 0,     //-
  0, 12, 13, 2, 4, 11, 3,      //%
  16, 16, 0, 0, 0, 0, 0     //'
};


const uint16_t PROGMEM rainAnimation[18][7] = {
  2313, 2121, 64, 258, 1314, 1064, 136,
288, 2313, 2121, 64, 258, 1314, 1064,
1058, 288, 2313, 2121, 64, 258, 1314,
1154, 1058, 288, 2313, 2121, 64, 258,
144, 1154, 1058, 288, 2313, 2121, 64,
2577, 144, 1154, 1058, 288, 2313, 2121, 
2625, 2577, 144, 1154, 1058, 288, 2313,
68, 2625, 2577, 144, 1154, 1058, 288,
1300, 68, 2625, 2577, 144, 1154, 1058,
1296, 1300, 68, 2625, 2577, 144, 1154,
2, 1296, 1300, 68, 2625, 2577, 144,
130, 2, 1296, 1300, 68, 2625, 2577,
136, 130, 2, 1296, 1300, 68, 2625,
1064, 136, 130, 2, 1296, 1300, 68,
1314, 1064, 136, 130, 2, 1296, 1300,
258, 1314, 1064, 136, 130, 2, 1296,
64, 258, 1314, 1064, 136, 130, 2,
2121, 64, 258, 1314, 1064, 136, 130
  
};

const uint16_t PROGMEM lightRainAnimation[14][7] = {
128, 4, 0, 0, 1024, 16, 0,
0, 128, 4, 0, 0, 1024, 16, 
2048, 0, 128, 4, 0, 0, 1024, 
1, 2048, 0, 128, 4, 0, 0,
0, 1, 2048, 0, 128, 4, 0, 
32, 0, 1, 2048, 0, 128, 4,
0, 32, 0, 1, 2048, 0, 128,
512, 0, 32, 0, 1, 2048, 0,
0, 512, 0, 32, 0, 1, 2048,
16, 0, 512, 0, 32, 0, 1, 
1024, 16, 0, 512, 0, 32, 0,
0, 1024, 16, 0, 512, 0, 32,
0, 0, 1024, 16, 0, 512, 0, 
4, 0, 0, 1024, 16, 0, 512
};


const uint16_t PROGMEM fogAnimation[15][7] = {
1008, 1038, 2049, 2046, 3959, 3551, 1981,
1008, 1038, 2049, 2046, 1979, 3823, 3038,
1008, 1038, 2049, 2046, 3037, 3959, 3567,
1008, 1038, 2049, 2046, 3566, 1979, 3831,
1008, 1038, 2049, 2046, 3831, 3037, 3963,
1008, 1038, 2049, 2046, 3963, 3566, 1981, 
1008, 1038, 2049, 2046, 4029, 3831, 3038,
1008, 1038, 2049, 2046, 2014, 3963, 3567, 
1008, 1038, 2049, 2046, 3055, 4029, 3831,
1008, 1038, 2049, 2046, 3575, 2014, 3963,
1008, 1038, 2049, 2046, 3835, 3055, 1981,
1008, 1038, 2049, 2046, 1917, 3575, 3038,
1008, 1038, 2049, 2046, 3006, 3835, 3567, 
1008, 1038, 2049, 2046, 3551, 1917, 3831, 
1008, 1038, 2049, 2046, 3823, 3006, 3963
};


const uint16_t PROGMEM sunAnimation[4][7] = {
  0, 96, 240, 240, 96, 0, 0, 
  264, 96, 756, 240, 96, 264, 0, 
  264, 96, 1782, 240, 96, 264, 516, 
  264, 96, 756, 240, 96, 264, 0

};

const uint16_t PROGMEM cloudAnimation[23][7] = {
  0, 48, 200, 774, 1025, 1022, 0,
0, 24, 100, 387, 512, 511, 0,
0, 12, 2098, 193, 256, 255, 0,
0, 2054, 1049, 2144, 128, 127, 0, 
0, 3075, 524, 3120, 64, 63, 0, 
2048, 1537, 262, 3608, 32, 31, 0,
3072, 768, 131, 3852, 16, 15, 0,
3584, 384, 65, 3974, 8, 7, 0, 
1792, 2240, 32, 4035, 4, 3, 0, 
896, 1120, 2064, 2017, 2, 1, 0,
448, 560, 1032, 1008, 1, 0, 0,
224, 280, 516, 504, 0, 0, 0,
112, 140, 258, 252, 2048, 0, 0,
56, 70, 129, 2174, 1024, 2048, 0,
28, 35, 64, 3135, 512, 3072, 0,
14, 17, 2080, 1567, 256, 3584, 0,
7, 2056, 1040, 783, 128, 3840, 0,
3, 3076, 520, 391, 64, 3968, 0,
1, 1538, 2308, 195, 32, 4032, 0,
0, 769, 3202, 97, 16, 4064, 0,
0, 384, 1601, 2096, 8, 4080, 0,
0, 192, 800, 3096, 4, 4088, 0,
0, 96, 400, 1548, 2050, 2044, 0
};

const uint16_t PROGMEM windAnimation[30][7] = {
  0, 0, 2048, 2048, 2048, 0, 0, 
0, 0, 3072, 3072, 3072, 0, 0, 
0, 0, 3584, 3584, 3584, 0, 0, 
0, 256, 3584, 3584, 3584, 0, 0, 
0, 256, 3584, 3840, 3840, 0, 0, 
512, 256, 3584, 3840, 3840, 0, 0, 
1536, 256, 3584, 3968, 3968, 0, 0, 
1536, 256, 3584, 4032, 4032, 0, 0, 
1536, 256, 3584, 4064, 4064, 0, 0, 
1536, 256, 3584, 4080, 4064, 16, 0, 
1536, 256, 3584, 4088, 4064, 16, 32, 
1536, 256, 3588, 4088, 4064, 16, 96, 
1536, 260, 3588, 4088, 4064, 16, 96, 
1544, 260, 3588, 4088, 4064, 16, 96, 
1560, 260, 3588, 4088, 4064, 16, 96, 
1560, 292, 3588, 4088, 4064, 16, 96, 
1560, 260, 3588, 4088, 4064, 16, 96, 
1544, 260, 3588, 4088, 4064, 16, 96, 
1536, 260, 3588, 4088, 4064, 16, 32, 
1536, 256, 3588, 4088, 4064, 16, 0, 
1536, 256, 3584, 4088, 4064, 16, 0, 
1536, 256, 3584, 4080, 4064, 0, 0, 
1536, 256, 3584, 4064, 4032, 0, 0, 
1536, 256, 3584, 3968, 3968, 0, 0, 
512, 256, 3584, 3840, 3840, 0, 0, 
0, 256, 3584, 3584, 3840, 0, 0, 
0, 0, 3584, 3584, 3584, 0, 0, 
0, 0, 3072, 3584, 3072, 0, 0, 
0, 0, 2048, 3072, 2048, 0, 0, 
0, 0, 2048, 2048, 2048, 0, 0

};

const uint16_t PROGMEM snowAnimation[13][7] = {
  2114, 3415, 2274, 1016, 225, 1363, 3649,
16, 2114, 3415, 2274, 1016, 225, 1363,
568, 16, 2114, 3415, 2274, 1016, 225,
1809, 568, 16, 2114, 3415, 2274, 1016,
515, 1809, 568, 16, 2114, 3415, 2274,
1, 515, 1809, 568, 16, 2114, 3415,
1024, 1, 515, 1809, 568, 16, 2114,
3649, 1024, 1, 515, 1809, 568, 16,
1363, 3649, 1024, 1, 515, 1809, 568,
225, 1363, 3649, 1024, 1, 515, 1809,
1016, 225, 1363, 3649, 1024, 1, 515,
2274, 1016, 225, 1363, 3649, 1024, 1,
3415, 2274, 1016, 225, 1363, 3649, 1024
};

const uint16_t PROGMEM stormAnimation[18][7] = {
 1026, 1020, 0, 0, 0, 0, 0, 
1026, 1020, 32, 64, 128, 0, 0, 
2046, 1020, 32, 64, 224, 0, 0, 
 1026, 1020, 32, 64, 224, 64, 128, 
 1026, 1020, 32, 64, 224, 64, 128, 
2046, 1020, 32, 64, 224, 64, 128, 
1026, 1020, 32, 64, 224, 64, 0, 
1026, 1020, 32, 64, 224, 0, 0, 
2046, 1020, 32, 64, 224, 0, 0, 
1026, 1020, 32, 64, 192, 0, 0, 
1026, 1020, 32, 64, 128, 0, 0, 
2046, 1020, 32, 64, 0, 0, 0, 
1026, 1020, 32, 0, 0, 0, 0, 
1026, 1020, 0, 0, 0, 0, 0, 
2046, 1020, 0, 0, 0, 0, 0, 
1026, 1020, 0, 0, 0, 0, 0, 
2046, 1020, 0, 0, 0, 0, 0, 
1026, 1020, 0, 0, 0, 0, 0


};

const uint16_t PROGMEM clearSkyAnimation[14][7] = {
 0, 112, 226, 192, 224, 112, 0, 
0, 112, 224, 192, 224, 1136, 0, 
0, 112, 224, 192, 228, 112, 0, 
0, 624, 224, 192, 224, 112, 0, 
0, 113, 224, 192, 224, 112, 0, 
0, 112, 224, 192, 224, 624, 0, 
0, 2160, 224, 192, 224, 112, 0, 
0, 112, 224, 192, 224, 112, 1, 
0, 112, 224, 192, 224, 1136, 0, 
0, 114, 224, 192, 224, 112, 0, 
0, 112, 224, 192, 2272, 112, 0, 
0, 112, 224, 192, 224, 112, 256, 
0, 112, 224, 192, 225, 112, 0, 
256, 112, 224, 192, 224, 112, 0
};



int letterPos=0, offset=7, spacing=5;
void showLetter(char& ch, int& offset){
  if(isDigit(ch)){
    ch-=48;
  }else if(isAlpha(ch)){
    if(isUpperCase(ch)){
      ch = ch-65+10;
    }else{
      ch = ch-97+36;
    }
  }else{
    switch(ch){
      case ',':
      ch=62;
      break;
      
      case '.':
      ch=63;
      break;

      case '#':   //stepen ustvari
      ch=64;
      break;

      case ':':
      ch=65;
      break;

      case '-':   
      ch=66;
      break;

      case '%':
      ch=67;
      break;

      case '!':   //zamenjeno sa .
      ch = 63;
      break;

      case '(':   //zamenjeno sa ,
      ch = 62;
      break;

      case ')':
      ch = 62;
      break;

      case '"':
      return;
      break;

      case '?':
      ch = 63;
      break;

      case '\'':
      ch=68;
      break;

      case ' ':
      ch=69;
      break;

    }
  }

  int pos=offset - letterPos;
  for(byte i=0;i<7;i++){
    byte progmemFont = pgm_read_word_near(&font[ch][i]);
    if(pos > 11 || pos <-4) break;
    if(pos>0){
      matrix[i] |= progmemFont<<pos;
    }else{
      matrix[i] |= progmemFont>>-pos;
    }
  }
  byte progmemFontWidth = pgm_read_word_near(&fontWidth[ch]);
  letterPos+=progmemFontWidth;
}

void clearScreen(){
  for(byte i=0;i<7;i++){
    matrix[i] =0;
  }
}



void showText(String& text, int offset){
  letterPos=0;
  char letter;
  for(uint16_t i=0;i<text.length(); i++){
    letter = text[i];
    switch (letter){
    case -60:
      letter='C';
      i++;
    break;
    case -59:
      letter='S';
      i++;
    break;
    case -62:
      letter='#';
      i++;
    break;
    }
    showLetter(letter, offset);
  }
}


void showAnimation(uint16_t* frame){
  for(byte i = 0; i<7; i++){
    uint16_t s = pgm_read_word_near(&frame[i]);
    matrix[i] = s;
  }
}

int cnt=0;
uint64_t t=0;
String dataReceived="G";

uint8_t hour=12;
byte animationFrames = 5;
uint16_t animationDelay = 300;
const uint16_t (*animation)[7] = nullptr;


enum MODE{ANIMATIONMODE, TEMPMODE, CORONAMODE, TIMEMODE, FACTMODE}mode=ANIMATIONMODE;
uint8_t order[10] = {ANIMATIONMODE, TEMPMODE, ANIMATIONMODE, FACTMODE, CORONAMODE, FACTMODE, TIMEMODE, FACTMODE};

byte modeCounter=0;

void loop(void){
  clearScreen();
  if(millis() - t> 60000 || dataReceived=="G"){
    dataReceived="G";
    mode = (MODE)order[modeCounter];
    modeCounter = ++modeCounter%8;
    updateScreen();
    //getDataFromWebSite(CORONAWEBPAGE ,WEBSITE, ip);
    byte numOfFails=0;
    while(dataReceived == "G"){
      //Serial.println("CORRECTED");
      switch(mode){
      case TEMPMODE:
       dataReceived = getDataFromWebSite(TEMPWEBPAGE, WEBSITE, ip);
      break;
      case CORONAMODE:
        dataReceived = getDataFromWebSite(CORONAWEBPAGE, WEBSITE, ip);
      break;
      case TIMEMODE:
        dataReceived = getDataFromWebSite(CURRENTTIME, WEBSITE, ip);
      break;
      case FACTMODE:
        dataReceived = getDataFromWebSite(FACTWEBPAGE, WEBSITE, ip);
      break;
      case ANIMATIONMODE:
        dataReceived = getDataFromWebSite(WEATHERWEBPAGE, WEBSITE, ip);
      break;
      }
      if(numOfFails>20){
        softwareReset( WDTO_1S);
      }
      numOfFails++;
    }
    switch(mode){
      case TEMPMODE:
       dataReceived = "Temperature is " + dataReceived + "C";
      break;
      case CORONAMODE:
       dataReceived = "Corona Serbia: " + dataReceived;
      break;
      
      case TIMEMODE:
        dataReceived.remove(dataReceived.length()-6, 3);
      break;

      case ANIMATIONMODE:
        if(dataReceived.indexOf(F("Rain"))!=-1){
          animation = lightRainAnimation;
          animationDelay=125;
          animationFrames = 14;

          if(dataReceived.indexOf(F("Shower"))!=-1 || dataReceived.indexOf(F("Heavy"))!=-1){
            animation = rainAnimation;
            animationDelay=75;
            animationFrames = 18;
          }
          
        }else if(dataReceived.indexOf(F("Sun"))!=-1 || (dataReceived.indexOf(F("Clear"))!=-1 && hour>7 && hour<22)){
          animation = sunAnimation;
          animationFrames = 4;
        }
        else if(dataReceived.indexOf(F("Cloud"))!=-1){
          animation = cloudAnimation;
          animationDelay=120;
          animationFrames = 23;
        }
        else if(dataReceived.indexOf(F("Snow"))!=-1){
          animation = snowAnimation;
          animationDelay=175;
          animationFrames = 13;
        }
        else if(dataReceived.indexOf(F("Wind"))!=-1){
          animation = windAnimation;
          animationDelay=75;
          animationFrames = 30;
        }
        else if(dataReceived.indexOf(F("Clear"))!=-1){
          if(hour>=22 || hour<=7){   //clear night
            animation = clearSkyAnimation;
            animationDelay=1600;
            animationFrames = 14;
          }
        }
        else if(dataReceived.indexOf(F("Storm"))!=-1 || dataReceived.indexOf(F("Thunder"))!=-1){
          animation = stormAnimation;
          animationDelay=80;
          animationFrames = 18;
        }
        else if(dataReceived.indexOf(F("Fog"))!=-1 ){
          animation = fogAnimation;
          animationDelay=500;
          animationFrames = 15;
        }
      break;
      
      /*
      case FACTMODE:
        dataReceived = dataReceived;   //raw data
      break;
      */
     }
    cnt=0;
    
    //cc3000.disconnect(); 
    t=millis();
  }

  if(mode == ANIMATIONMODE){
    showAnimation(animation[cnt]);
    cnt = ++cnt%animationFrames;
    delay(animationDelay);
  }else{
    showText(dataReceived, cnt);
    cnt = ++cnt%(int)(dataReceived.length()*4.75);
    delay(100);
  }
  
  
  updateScreen();
}


String getDataFromWebSite(char* webpage, char* website, uint32_t ip){
  //cc3000.printIPdotsRev(ip);

  unsigned long aucDHCP = 14400;
  unsigned long aucARP = 3600;
  unsigned long aucKeepalive = 10;
  unsigned long aucInactivity = 20;
  
  if (netapp_timeout_values(&aucDHCP, &aucARP, &aucKeepalive, &aucInactivity) != 0) {
    #ifdef developerMode
      Serial.println("Error setting inactivity timeout!");
    #endif
  }
  
  Adafruit_CC3000_Client www = cc3000.connectTCP(ip, 80);
  
  if (www.connected()) {
    www.fastrprint(F("GET "));
    www.fastrprint(webpage);
    www.fastrprint(F(" HTTP/1.1\r\n"));
    www.fastrprint(F("Host: ")); www.fastrprint(website); www.fastrprint(F("\r\n"));
    www.fastrprint(F("\r\n"));
    www.println();
  } else {
    #ifdef developerMode
      Serial.println(F("Connection failed")); 
    #endif   
    return "G";
    //getDataFromWebSite(webpage, website, ip);
  }

  //Serial.println(F("\n-------------------------------------"));
  unsigned long lastRead = millis();
  uint16_t lineCounter=0;
  String dataString="";
  while (www.connected() && (millis() - lastRead < IDLE_TIMEOUT_MS)) {
    while (www.available()) {
      String line = www.readStringUntil('\n');
      if(lineCounter>=19){
        dataString+=line;
      }
      if(lineCounter == 1 ){
        hour = line.substring(23, 25).toInt()+1;
        uint8_t val = (hour>=7 && hour<=22) ?(((-hour*hour + 29*hour - 154)/56.25)*10 + 1) : 1;
        maxAll(max7219_reg_intensity, val);
      }
      if(lineCounter>120) return "G";
      //Serial.print(line+'\n');
      lineCounter++;
      lastRead = millis();
    }
  }
  www.close();
  //Serial.println(F("-------------------------------------"));
  
  dataString.trim();
  dataString.remove(dataString.length()-1,1);
  dataString.trim();
  if(dataString.length()==0) return "G";
  /*
  if(dataString.length()==0){
    getDataFromWebSite(webpage, website, ip);
  }
  */
  return dataString;
}


/*
String getDataFromWebSite(char* webpage, char* website, uint32_t ip, String preString, String postString){
  String dataToProcess = getDataFromWebSite(webpage, website, ip);
  dataToProcess = dataToProcess.substring(dataToProcess.indexOf(preString) + preString.length(), dataToProcess.indexOf(postString));
  return dataToProcess;
}*/



bool displayConnectionDetails(void){
  
  uint32_t ipAddress, netmask, gateway, dhcpserv, dnsserv;
  
  if(!cc3000.getIPAddress(&ipAddress, &netmask, &gateway, &dhcpserv, &dnsserv))
  {
    #ifdef developerMode
      Serial.println(F("Unable to retrieve the IP Address!\r\n"));
    #endif
    return false;
  }
  #ifdef developerMode
    Serial.print(F("\nIP Addr: ")); cc3000.printIPdotsRev(ipAddress);
    Serial.print(F("\nNetmask: ")); cc3000.printIPdotsRev(netmask);
    Serial.print(F("\nGateway: ")); cc3000.printIPdotsRev(gateway);
    Serial.print(F("\nDHCPsrv: ")); cc3000.printIPdotsRev(dhcpserv);
    Serial.print(F("\nDNSserv: ")); cc3000.printIPdotsRev(dnsserv);
    Serial.println();
   #endif
   return true;
}

