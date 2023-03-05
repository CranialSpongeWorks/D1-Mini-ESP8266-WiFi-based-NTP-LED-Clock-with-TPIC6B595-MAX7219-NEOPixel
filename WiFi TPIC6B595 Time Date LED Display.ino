/***************************************************************************
 WiFi based TPIC6B595 Time/Date LED Display
 Large LED Clock Display with 10 TPIC6B595 Driver boards & NEO-Pixels
 
 Board: LOLIN (WEEMOS) D1 Mini

 Created by:
 ©2023 David C. Boyce
 email:pcboardrepair@gmail.com

 Youtube link: https://www.youtube.com/@PCBoardRepair
 GitHub link:  https://github.com/CranialSpongeWorks/D1-Mini-ESP8266-WiFi-based-NTP-LED-Clock-with-TPIC6B595-MAX7219-NEOPixel
 Added a small LED display for DATE but code is still there for extra 6digits
****************************************************************************/

#include <NTPClient.h>
#include <WiFiUdp.h>
#include <ESP8266WebServer.h>
#include <Adafruit_NeoPixel.h>
#include <DigitLedDisplay.h>

const char ssid[] = "_SSID_";            // your network SSID (name)
const char pass[] = "_PASSWORD_";       // your network password
ESP8266WebServer server(80);

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", -5 * 3600);  //define TimeClient for Time Zone = -5

int dataPin = D1;   //SER IN   TPIC6B595 (pin 2)
int clockPin = D2;  //SRCLK    TPIC6B595 (pin 15)
int strobePin = D3; //RCLK     TPIC6B595 (pin 10)
int clearPin = D0;  //CLR      TPIC6B595 (pin 7)

// Declare our NeoPixel strip object:
const int NEO_RED = 0xFF0000;
const int NEO_GREEN = 0x008000;
const int NEO_BLUE = 0x0000FF;
const int NEO_YELLOW = 0xFFFF00;
const int NEO_WHITE = 0x808080;
const int NEO_OFF = 0x000000;
#define LED_PIN   D4  //ESP8266 pin for NeoPixel output
#define LED_COUNT 8   //define number of pixels
Adafruit_NeoPixel strip(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);
// Argument 1 = Number of pixels in NeoPixel strip
// Argument 2 = Arduino pin number (most are valid)
// Argument 3 = Pixel type flags, add together as needed:
//   NEO_KHZ800  800 KHz bitstream (most NeoPixel products w/WS2812 LEDs)
//   NEO_KHZ400  400 KHz (classic 'v1' (not v2) FLORA pixels, WS2811 drivers)
//   NEO_GRB     Pixels are wired for GRB bitstream (most NeoPixel products)
//   NEO_RGB     Pixels are wired for RGB bitsream (v1 FLORA pixels, not v2)
//   NEO_RGBW    Pixels are wired for RGBW bitstream (NeoPixel RGBW products)

const uint8_t digitToSegment[] = {
 // GFpABCDE
  0b01011111,    // 0
  0b00001100,    // 1
  0b10011011,    // 2
  0b10011110,    // 3
  0b11001100,    // 4
  0b11010110,    // 5
  0b11010111,    // 6
  0b00011100,    // 7
  0b11011111,    // 8
  0b11011110,    // 9
  0b11011101,    // A
  0b11000111,    // B
  0b01010011,    // C
  0b10001111,    // D
  0b11011001,    // E
  0b11010001     // F
  };
// initialize with pin numbers for data, cs, and clock
DigitLedDisplay leddisplay = DigitLedDisplay(13, 15, 14); //D7 MOSI 13, D8 CS 15,D5 CLK 14

int currentHour, currentMinute, currentSecond, currentMonth, currentDate, currentYear, Hour;
unsigned long prevDisplay, currentMillis;
bool colon;

// setup() function -- runs once at startup --------------------------------
void setup() {
  Serial.begin(115200);
  while (!Serial) ;
  delay(500);

  // Set pins to output because they are addressed in the main loop
  pinMode(strobePin, OUTPUT);
  pinMode(clockPin, OUTPUT);
  pinMode(dataPin, OUTPUT);
  pinMode(clearPin, OUTPUT);

  digitalWrite(strobePin, LOW);
  digitalWrite(clockPin, LOW);
  digitalWrite(dataPin, LOW);
  digitalWrite(clearPin, HIGH);

  leddisplay.setDigitLimit(8);                              //setup MAX7219 Digit Limit
//  leddisplay.printDigit(0);                                 //default display

  //INITIALIZE NeoPixel strip object (REQUIRED)
  strip.begin(); strip.clear(); strip.setBrightness(60);  //Set BRIGHTNESS to about 1/5 (max = 255)
  strip.setPixelColor(0, NEO_OFF); strip.setPixelColor(1, NEO_OFF); strip.setPixelColor(2, NEO_OFF); strip.setPixelColor(3, NEO_OFF); strip.show();             //Turn OFF all pixels ASAP
  strip.setPixelColor(4, NEO_OFF); strip.setPixelColor(5, NEO_OFF); strip.setPixelColor(6, NEO_OFF); strip.setPixelColor(7, NEO_OFF); strip.show();
  strip.show();

  Serial.println();
  Serial.println("Connecting to ");
  Serial.print(ssid);
  WiFi.begin(ssid, pass);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  Serial.println("Connected!!!");

  //start Server
  server.begin(); Serial.println("HTTP server started");
  //start Time Client
  timeClient.begin(); getTime(); prevDisplay = 0;  // when the digital clock was displayed

  //if Time is not valid reset ESP8255
  if (timeClient.isTimeSet()==false) {ESP.reset();}
}

// loop() function -- runs repeatedly as long as board is on ---------------
void loop() {
 currentMillis = millis();
 if (currentMillis-prevDisplay > 500){
    getTime(); outputStream(); strip.clear();
    if (colon == false){
        colon=true; strip.setPixelColor(0, NEO_RED); strip.setPixelColor(1, NEO_RED); strip.setPixelColor(2, NEO_RED); strip.setPixelColor(5, NEO_RED); strip.setPixelColor(6, NEO_RED); strip.setPixelColor(7, NEO_RED);
      } else {
        colon=false; strip.setPixelColor(0, NEO_OFF); strip.setPixelColor(1, NEO_OFF); strip.setPixelColor(2, NEO_OFF); strip.setPixelColor(5, NEO_OFF); strip.setPixelColor(6, NEO_OFF); strip.setPixelColor(7, NEO_OFF);
    }
    print2Digits(currentHour); Serial.print(":"); print2Digits(currentMinute); Serial.print(":"); print2Digits(currentSecond);
    if (Hour >= 12){
      strip.setPixelColor(3, NEO_GREEN); strip.setPixelColor(4, NEO_GREEN); Serial.print("PM");
    } else {
      strip.setPixelColor(3, NEO_OFF); strip.setPixelColor(4, NEO_OFF); Serial.print("AM");
    }
    strip.show(); //  Update strip to match
    Serial.print(" "); print2Digits(currentMonth); Serial.print("/"); print2Digits(currentDate); Serial.print("/"); Serial.println(currentYear-2000);
    if (currentMonth < 10){ leddisplay.printDigit(0, 7); }
    //update small display with date
    leddisplay.printDigit(currentMonth, 6);
    leddisplay.write(6,1);
    if (currentDate < 10){ leddisplay.printDigit(0, 4); }
    leddisplay.printDigit(currentDate, 3);
    leddisplay.write(3,1);
    if ((currentYear-2000) < 10){ leddisplay.printDigit(0, 1); }
    leddisplay.printDigit(currentYear-2000,0);
    prevDisplay = millis();
 }
}

void outputStream() { //output bit stream to (10) TPIC6B595
  digitalWrite(strobePin, LOW); // Set strobe pin low to begin storing bits
  //Year
  Send595Data(DecToBCD(currentYear-2000)); //make into BCD and send
  //Day
  Send595Data(DecToBCD(currentDate)); //make into BCD and send
  //Month
  Send595Data(DecToBCD(currentMonth)); //make into BCD and send
  //Minute
  Send595Data(DecToBCD(currentMinute)); //make into BCD and send
  //Hour
  Send595Data(DecToBCD(currentHour)); //make into BCD and send
  digitalWrite(strobePin, HIGH); // Set strobe pin high to stop storing bits
}

void Send595Data(byte bValue){
  byte outputPattern; //declaration
  byte bTime = 0; byte bUpper = 0; byte bLower = 0;
  bUpper = bValue & B11110000; bUpper = bUpper >> 4; bLower = bValue & B00001111;
  outputPattern = digitToSegment[bLower]; shiftOut(dataPin, clockPin, MSBFIRST, outputPattern); digitalWrite(dataPin, LOW);
  outputPattern = digitToSegment[bUpper]; shiftOut(dataPin, clockPin, MSBFIRST, outputPattern); digitalWrite(dataPin, LOW);
}

void prntBits(byte b) {
  for(int i = 7; i >= 0; i--)
    Serial.print(bitRead(b,i));
  Serial.println();  
}

byte DecToBCD(byte dec){
	byte result = 0;
	result |= (dec / 10) << 4;
	result |= (dec % 10) << 0;
	return result;
}

void print2Digits(int digits){
  // utility for digital clock display: prints preceding colon and leading 0
  if (digits < 10) Serial.print('0');
  Serial.print(digits);
}

//read NTP Time
void getTime(){
  timeClient.update(); 
  Hour = timeClient.getHours();
  if (Hour > 12) { currentHour = Hour - 12; }else{ currentHour = Hour; }
  currentMinute = timeClient.getMinutes(); 
  currentSecond = timeClient.getSeconds();
  time_t epochTime = timeClient.getEpochTime();
  //Get a time structure
  struct tm *ptm = gmtime ((time_t *)&epochTime); 
  currentMonth = ptm->tm_mon+1;
  currentDate = ptm->tm_mday;
  currentYear = ptm->tm_year+1900;
}

//update Pixel Strip
void updatePixelStrip() {
  strip.clear();
  strip.setPixelColor(0, NEO_YELLOW);
  strip.show(); //  Update strip to match
}

//end