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

static uint32_t timer;
static uint32_t timer2;

static bool ethernetBusy = 0;
int debugLevel = 2; // 0 - no debug, 1 - button info, 2 - server answer, 4 - full

const char* pId;
const char* req0;
const char* req1;

bool button0Pressed;
bool button1Pressed;

String panelIdString;
String req0String;
String req1String;

byte Ethernet::buffer[600];
const char website[] PROGMEM = "192.168.1.109";
Adafruit_NeoPixel pixels = Adafruit_NeoPixel(2, rgbLED, NEO_GRB + NEO_KHZ800);

//---------------------------------------- json callback ----------------------------------------

static void jsonCallback (byte status, word off, word len) {

  Serial.print("hello from json callback: ");
  Serial.println(status);

  if (status != 0) {
    ethernetBusy = 0;
    return;
  }

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
    //Serial.println(websiteString);
    Serial.println(json);
  }

  StaticJsonBuffer<200> jsonBuffer;
  JsonObject& root = jsonBuffer.parseObject(json);

  if (!root.success()) {
    Serial.println("parseObject() failed");
    pixels.setPixelColor(0, 0, 255, 0); // parsing error - red led
    pixels.setPixelColor(1, 0, 255, 0);
    pixels.show();
    //delay(300);
    ethernetBusy = 0;
    return;
  }

  else {
    request0 = root["R0"];
    request1 = root["R1"];
    request0Picked = root["R1P"];
    request1Picked = root["R1P"];

    if ((debugLevel == 2) || (debugLevel == 4)) {
      Serial.println(request0);
      Serial.println(request1);
      Serial.println(request0Picked);
      Serial.println(request1Picked);
    }
  }

  ethernetBusy = 0;

  // Button 0 (request 0)

  if ((request0 == true) && (request0Picked == false)) {
    pixels.setPixelColor(0, 255, 255, 0); // button pressed, no confirmation - yellow led
    pixels.show();
    //delay(300);
  }

  if ((request0 == true) && (request0Picked == true)) {
    pixels.setPixelColor(0, 255, 0, 0); // button pressed, confirmed - green led
    pixels.show();
    //delay(300);
  }

  if (request0 == false) {
    pixels.setPixelColor(0, 0, 0, 255); // idle - blue led
    pixels.show();
    //delay(300);
  }

  // Button 1 (request 1)

  if ((request1 == true) && (request1Picked == false)) {
    pixels.setPixelColor(1, 255, 255, 0); // button pressed, no confirmation - yellow led
    pixels.show();
    //delay(300);
  }

  if ((request1 == true) && (request1Picked == true)) {
    pixels.setPixelColor(1, 255, 0, 0); // button pressed, confirmed - green led
    pixels.show();
    //delay(300);
  }

  if (request1 == false) {
    pixels.setPixelColor(1, 0, 0, 255); // idle - blue led
    pixels.show();
    //delay(300);
  }

}

//------------------------------------- request 0 callback --------------------------------------

static void req0Callback (byte status, word off, word len) {
  Ethernet::buffer[off + len] = 0;
  //delay(500);
  Serial.print("callback 0 ethernetBusy przed: ");
  Serial.println(ethernetBusy);
  ethernetBusy = 0;
  Serial.print("callback 0 ethernetBusy po: ");
  Serial.println(ethernetBusy);
}

//------------------------------------- request 1 callback --------------------------------------

static void req1Callback (byte status, word off, word len) {
  Ethernet::buffer[off + len] = 0;
  //delay(500);
  Serial.print("callback 1 ethernetBusy przed: ");
  Serial.println(ethernetBusy);
  ethernetBusy = 0;
  Serial.print("callback 1ethernetBusy po: ");
  Serial.println(ethernetBusy);
}
//------------------------------------------- setup ---------------------------------------------
void setup () {
  pinMode(button0, INPUT_PULLUP);
  pinMode(button1, INPUT_PULLUP);
  pinMode(rgbLED, OUTPUT);

  Serial.begin(57600);
  pixels.begin();
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

  pixels.setPixelColor(0, 0, 255, 0); // set leds to red initially
  pixels.setPixelColor(1, 0, 255, 0);
  pixels.show();
}

//-------------------------------------------- loop ---------------------------------------------

void loop() {

  int button0State = digitalRead(button0);
  int button1State = digitalRead(button1);

  if (millis() > timer2) {
    Serial.print("ethernetBusy: ");
    Serial.print(ethernetBusy);
    Serial.println();
    timer2 = millis() + 100;
  }


  if (button0State == 0) {
    pixels.setPixelColor(0, 55, 55, 0); // pressed - tinted yellow led
    pixels.show();
    button0Pressed = 1;
  }

  if (button1State == 0) {
    pixels.setPixelColor(1, 55, 55, 0); // pressed - tinted yellow led
    pixels.show();
    button1Pressed = 1;
  }

  byte len;
  ether.packetLoop(ether.packetReceive());

  //----------------------------------------- request 0 -------------------------------------------

  if (button0Pressed) {
    ethernetBusy = 1;
    button0Pressed = 0;
    Serial.println("Sending request 0:");
    ether.browseUrl(PSTR("/dispatcher/api/CreateRequest/"), req0, website, req0Callback);
    delay(1000);
  }

  //----------------------------------------- request 1 -------------------------------------------

  if (button1Pressed) {
    ethernetBusy = 1;
    button1Pressed = 0;
    Serial.println("Sending request 1:");
    ether.browseUrl(PSTR("/dispatcher/api/CreateRequest/"), req1, website, req1Callback);
    delay(1000);
  }

  //----------------------------------- check active requests -------------------------------------

  if (millis() > timer) {
    ethernetBusy = 1;
    Serial.println(millis());
    timer = millis() + 1000;
    Serial.println("Checking requests:");
    ether.browseUrl(PSTR("/dispatcher/api/ActiveRequests/"), pId, website, jsonCallback);
  }

  //timeout ??

}
