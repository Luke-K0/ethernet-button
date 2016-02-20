#include <TimerOne.h>
#include <EtherCard.h>
#include <ArduinoJson.h>
#include <Adafruit_NeoPixel.h>


#define button0 4
#define button1 5
#define rgbLED 6

//-----------------------------------------------------------------------------------------------
//------------------------------------- unique panel config -------------------------------------
//-----------------------------------------------------------------------------------------------

static byte mac[] = { 0xAA, 0xBB, 0xCC, 0xDD, 0x00, 0x01 };
int panelID = 1;

//-----------------------------------------------------------------------------------------------
//-----------------------------------------------------------------------------------------------
//-----------------------------------------------------------------------------------------------

static uint32_t checkRequestsTimer;
static uint32_t button0Timer;
static uint32_t button1Timer;

int debugLevel = 2; // 0 - no debug, 1 - button info, 2 - server answer, 4 - full

const char* pId;
const char* req0;
const char* req1;

bool button0Pressed;
bool button1Pressed;

String panelIdString;
String req0String;
String req1String;

int colorValue = 150;

byte Ethernet::buffer[600];
const char website[] PROGMEM = "192.168.1.109";
Adafruit_NeoPixel pixels = Adafruit_NeoPixel(2, rgbLED, NEO_GRB + NEO_KHZ800);

//last time the WDT was ACKd by the application
unsigned long lastUpdate=0;
 
//time, in ms, after which a reset should be triggered
unsigned long timeout=10000;

void (*resetFunc)(void) = 0;

//-------------------------------------- WatchDog Timer -----------------------------------------
 
void longWDT(void)
{
  if((millis()-lastUpdate) > timeout)
  {
    //enable interrupts so serial can work
    sei();
 
    //detach Timer1 interrupt so that if processing goes long, WDT isn't re-triggered
    Timer1.detachInterrupt();
 
    //flush, as Serial is buffered; and on hitting reset that buffer is cleared
    Serial.println("WDT triggered");
//    Serial.flush();
 
    //call to bootloader / code at address 0
    resetFunc();
  }
}

//---------------------------------------- json callback ----------------------------------------

static void jsonCallback (byte status, word off, word len) {

  Serial.print("hello from json callback: ");
  Serial.println(status);

  String websiteString;

  bool request0;
  bool request1;
  bool request0Picked;
  bool request1Picked;
  int openingBracket;
  int closingBracket;

  Ethernet::buffer[off + len] = 0;

  websiteString = (const char*) Ethernet::buffer + off;
  openingBracket = websiteString.lastIndexOf('{');
  closingBracket = websiteString.indexOf('}');
  String json = websiteString.substring(openingBracket, closingBracket + 1);

  if ((debugLevel == 2) || (debugLevel == 4)) {
    Serial.println(openingBracket);
    Serial.println(closingBracket);
    Serial.println(websiteString);
    Serial.println(json);
  }

  StaticJsonBuffer<200> jsonBuffer;
  JsonObject& root = jsonBuffer.parseObject(json);

  if (!root.success()) {
    Serial.println("parseObject() failed");
    pixels.setPixelColor(0, 0, colorValue, 0); // parsing error - red led
    pixels.setPixelColor(1, 0, colorValue, 0);
    pixels.show();
  }

  else {
    request0 = root["R0"];
    request1 = root["R1"];
    request0Picked = root["R0P"];
    request1Picked = root["R1P"];

    if ((debugLevel == 2) || (debugLevel == 4)) {
      Serial.println(request0);
      Serial.println(request1);
      Serial.println(request0Picked);
      Serial.println(request1Picked);
    }

    // Button 0 (request 0)

    if ((request0 == true) && (request0Picked == false)) {
      pixels.setPixelColor(0, colorValue, colorValue, 0); // button pressed, no confirmation - yellow led
      pixels.show();
    }

    if ((request0 == true) && (request0Picked == true)) {
      pixels.setPixelColor(0, colorValue, 0, 0); // button pressed, confirmed - green led
      pixels.show();
    }

    if (request0 == false) {
      pixels.setPixelColor(0, 0, 0, colorValue); // idle - blue led
      pixels.show();
    }

    // Button 1 (request 1)

    if ((request1 == true) && (request1Picked == false)) {
      pixels.setPixelColor(1, colorValue, colorValue, 0); // button pressed, no confirmation - yellow led
      pixels.show();
    }

    if ((request1 == true) && (request1Picked == true)) {
      pixels.setPixelColor(1, colorValue, 0, 0); // button pressed, confirmed - green led
      pixels.show();
    }

    if (request1 == false) {
      pixels.setPixelColor(1, 0, 0, colorValue); // idle - blue led
      pixels.show();
    }
    lastUpdate=millis(); 
  }
}

//------------------------------------- request 0 callback --------------------------------------

static void req0Callback (byte status, word off, word len) {
  Ethernet::buffer[off + len] = 0;
}

//------------------------------------- request 1 callback --------------------------------------

static void req1Callback (byte status, word off, word len) {
  Ethernet::buffer[off + len] = 0;
}
//------------------------------------------- setup ---------------------------------------------
void setup () {
  timeout = 30000; 
  lastUpdate=millis();
 
  Timer1.initialize(1000000); //1 second pulses
  Timer1.attachInterrupt(longWDT); //code to execute
  
  pinMode(button0, INPUT_PULLUP);
  pinMode(button1, INPUT_PULLUP);
  pinMode(rgbLED, OUTPUT);

  Serial.begin(57600);
  pixels.begin();

  pixels.setPixelColor(0, 0, colorValue, 0); // set leds to red initially
  pixels.setPixelColor(1, 0, colorValue, 0);
  pixels.show();
  
  Serial.println(F("\n[webClient]"));

  panelIdString = String(panelID);
  req0String = String(panelID) + "/0";
  req1String = String(panelID) + "/1";

  pId = panelIdString.c_str ();
  req0 = req0String.c_str ();
  req1 = req1String.c_str ();

  if (ether.begin(sizeof Ethernet::buffer, mac) == 0)
    Serial.println(F("Failed to access Ethernet controller"));
  if (!ether.dhcpSetup())
    Serial.println(F("DHCP failed"));

  ether.printIp("IP:  ", ether.myip);
  ether.printIp("GW:  ", ether.gwip);
  ether.printIp("DNS: ", ether.dnsip);

  if (!ether.dnsLookup(website))
    Serial.println("DNS failed");

  ether.printIp("SRV: ", ether.hisip);

  timeout = 10000;
}

//-------------------------------------------- loop ---------------------------------------------

void loop() {

  int button0State = digitalRead(button0);
  int button1State = digitalRead(button1);

  if (button0State == 0) {
    pixels.setPixelColor(0, 0, colorValue, colorValue); // pressed - tinted yellow led
    pixels.show();
    button0Pressed = 1;
  }

  if (button1State == 0) {
    pixels.setPixelColor(1, 0, colorValue, colorValue); // pressed - tinted yellow led
    pixels.show();
    button1Pressed = 1;
  }

  byte len;
  ether.packetLoop(ether.packetReceive());

//----------------------------------------- request 0 -------------------------------------------

  if ((button0Pressed) && (millis() > button0Timer)) {
    button0Pressed = 0;
    button0Timer = millis() + 5000;
    Serial.println("Sending request 0:");
    ether.browseUrl(PSTR("/api/CreateRequest/"), req0, website, req0Callback);
  }

//----------------------------------------- request 1 -------------------------------------------

  else if ((button1Pressed) && (millis() > button1Timer)) {
    button1Pressed = 0;
    button1Timer = millis() + 5000;
    Serial.println("Sending request 1:");
    ether.browseUrl(PSTR("/api/CreateRequest/"), req1, website, req1Callback);
  }

//----------------------------------- check active requests -------------------------------------

  else if (millis() > checkRequestsTimer) {
    Serial.println(millis());
    checkRequestsTimer = millis() + 1000;
    Serial.println("Checking requests:");
    ether.browseUrl(PSTR("/api/ActiveRequests/"), pId, website, jsonCallback);
  }
}
