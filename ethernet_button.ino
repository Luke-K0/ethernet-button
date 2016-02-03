#include <EtherCard.h>
#include <ArduinoJson.h>
#include <Adafruit_NeoPixel.h>

#define button0 4
#define button1 5
#define rgbLED 6

// unique panel config
static byte mac[] = { 0x74, 0x69, 0x69, 0x2D, 0x30, 0x2C };
int panelID = 3;

byte Ethernet::buffer[600];
static uint32_t timer;
String websiteString;
String json;
int openingBracket;
int closingBracket;

bool button0Pressed = 0;
bool button1Pressed = 0;
int button0State;
int button1State;

bool request0;
bool request1;
bool request0Picked;
bool request1Picked;

const char* req0;
const char* req1;

String req0String;
String req1String;

Adafruit_NeoPixel pixels = Adafruit_NeoPixel(2, rgbLED, NEO_GRB + NEO_KHZ800);

const char website[] PROGMEM = "192.168.1.109";

// url to parse:
// http://192.168.1.109/dispatcher/api/ActiveRequests/3

// called when the client request is complete
static void my_callback (byte status, word off, word len) {
  Ethernet::buffer[off + 500] = 0;
  websiteString = (const char*) Ethernet::buffer + off;

}

void setup () {
  pinMode(button0, INPUT_PULLUP);
  pinMode(button1, INPUT_PULLUP);
  pinMode(rgbLED, OUTPUT);

  Serial.begin(57600);
  pixels.begin();
  Serial.println(F("\n[webClient]"));

  req0String = String(panelID) + "/0";
  req1String = String(panelID) + "/1";

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
}

void loop () {

  button0State = digitalRead(button0);
  button1State = digitalRead(button1);

  Serial.println(button0State);
  Serial.println(button0Pressed);

  Serial.println(button1State);
  Serial.println(button1Pressed);

  ether.packetLoop(ether.packetReceive());

  if (millis() > timer) {
    timer = millis() + 5000;
    ether.browseUrl(PSTR("/dispatcher/api/ActiveRequests/"), "3", website, my_callback);

    openingBracket = websiteString.lastIndexOf('{');
    closingBracket = websiteString.indexOf('}');

    Serial.println(openingBracket);
    Serial.println(closingBracket);

    json = websiteString.substring(openingBracket, closingBracket + 1);

    websiteString = "";
    openingBracket = 0;
    closingBracket = 0;

    Serial.println(json);

    StaticJsonBuffer<200> jsonBuffer;
    JsonObject& root = jsonBuffer.parseObject(json);

    if (!root.success()) {
      Serial.println("parseObject() failed");
      pixels.setPixelColor(0, 0, 255, 0);
      pixels.setPixelColor(1, 0, 255, 0);
      pixels.show();
      delay(300);
      return;
    }

    else {
      request0 = root["R0"];
      request1 = root["R1"];
      request0Picked = root["R1P"];
      request1Picked = root["R1P"];

      Serial.println(request0);
      Serial.println(request1);
      Serial.println(request0Picked);
      Serial.println(request1Picked);

    }

    // Button 0 (request 0)

    if ((button0State == 0) && (button0Pressed == 0)) {
      button0Pressed = 1;
      ether.browseUrl(PSTR("/dispatcher/api/CreateRequest/"), req0, website, my_callback);
    }

    if ((request0 == true) && (request0Picked == false)) {
      pixels.setPixelColor(0, 255, 255, 0); // button pressed, no confirmation - yellow led
      pixels.show();
      delay(300);
    }

    if ((request0 == true) && (request0Picked == true)) {
      pixels.setPixelColor(0, 255, 0, 0); // button pressed, confirmed - green led
      pixels.show();
      delay(300);
    }

    if (request0 == false) {
      pixels.setPixelColor(0, 0, 0, 255); // idle - blue led
      pixels.show();
      button0Pressed = false;
      delay(300);
    }

    // Button 1 (request 1)

    if ((button1State == 0) && (button1Pressed == 0)) {
      button1Pressed = 1;
      ether.browseUrl(PSTR("/dispatcher/api/CreateRequest/"), req1, website, my_callback);
    }

    if ((request1 == true) && (request1Picked == false)) {
      pixels.setPixelColor(1, 255, 255, 0); // button pressed, no confirmation - yellow led
      pixels.show();
      delay(300);
    }

    if ((request1 == true) && (request1Picked == true)) {
      pixels.setPixelColor(1, 255, 0, 0); // button pressed, confirmed - green led
      pixels.show();
      delay(300);
    }

    if (request1 == false) {
      pixels.setPixelColor(1, 0, 0, 255); // idle - blue led
      pixels.show();
      button0Pressed = false;
      delay(300);
    }
  }

}
