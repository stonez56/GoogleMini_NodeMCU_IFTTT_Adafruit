/*
   TO DO LIST
   1. Blink LED before connection made to Adafruit
   2. Add FastLED to library
   3. Update Adafruit feed to color names instead of numbers - "ranbow", "fire" "random", "White", "Yellow"
   4. Before Ada server connect - LED red, after blue
   5. When get a message - blink Blue LED 3 times
*/


#include <FastLED.h>
#include <Wire.h>

// Information about the LED strip itself
#define NUM_LEDS    60 //Number of LED in the whole spripe
#define CHIPSET     WS2811
#define COLOR_ORDER GRB
#define BRIGHTNESS  128
// FastLED v2.1 provides two color-management controls:
//   (1) color correction settings for each LED strip, and
//   (2) master control of the overall output 'color temperature'
//
// THIS EXAMPLE demonstrates the second, "color temperature" control.
// It shows a simple rainbow animation first with one temperature profile,
// and a few seconds later, with a different temperature profile.
//
// The first pixel of the strip will show the color temperature.
//
// HELPFUL HINTS for "seeing" the effect in this demo:
// * Don't look directly at the LED pixels.  Shine the LEDs aganst
//   a white wall, table, or piece of paper, and look at the reflected light.
//
// * If you watch it for a bit, and then walk away, and then come back
//   to it, you'll probably be able to "see" whether it's currently using
//   the 'redder' or the 'bluer' temperature profile, even not counting
//   the lowest 'indicator' pixel.
//
//
// FastLED provides these pre-conigured incandescent color profiles:
//     Candle, Tungsten40W, Tungsten100W, Halogen, CarbonArc,
//     HighNoonSun, DirectSunlight, OvercastSky, ClearBlueSky,
// FastLED provides these pre-configured gaseous-light color profiles:
//     WarmFluorescent, StandardFluorescent, CoolWhiteFluorescent,
//     FullSpectrumFluorescent, GrowLightFluorescent, BlackLightFluorescent,
//     MercuryVapor, SodiumVapor, MetalHalide, HighPressureSodium,
// FastLED also provides an "Uncorrected temperature" profile
//    UncorrectedTemperature;
#define TEMPERATURE_1 Tungsten100W
#define TEMPERATURE_2 OvercastSky
// How many seconds to show each temperature before switching
#define DISPLAYTIME 20
// How many seconds to show black between switches
#define BLACKTIME   3
#define FASTLED_pin 16 //NodeMCU pin D6

CRGB leds[NUM_LEDS];

// Relay settings
#define relay2Pin 13 //NodeMCU pin D7
#define relay1Pin 15 //NodeMCU pin D8
int relay1Status = 0; //switch of the relay; either 0=off or 1=on
int relay2Status = 0; //switch of the relay; either 0=off or 1=on

// Internal LED settings


/***************************************************
  Adafruit MQTT Library ESP8266 Adafruit IO SSL/TLS example
  Must use the latest version of ESP8266 Arduino from:
    https://github.com/esp8266/Arduino
  Works great with Adafruit's Huzzah ESP board & Feather
  ----> https://www.adafruit.com/product/2471
  ----> https://www.adafruit.com/products/2821
  Adafruit invests time and resources providing this open source code,
  please support Adafruit and open-source hardware by purchasing
  products from Adafruit!
  Written by Tony DiCola for Adafruit Industries.
  SSL/TLS additions by Todd Treece for Adafruit Industries.
  MIT license, all text above must be included in any redistribution
 ****************************************************/
#include <ESP8266WiFi.h>
#include "Adafruit_MQTT.h"
#include "Adafruit_MQTT_Client.h"

/************************* WiFi Access Point *********************************/

//#define WLAN_SSID       "SkySwimmer"
#define WLAN_SSID       "Stonez24"
#define WLAN_PASS       "a0101010101a"

/************************* Adafruit.io Setup *********************************/

#define AIO_SERVER      "io.adafruit.com"
#define AIO_SERVERPORT  8883                   // 8883 for MQTTS sercure, 1883 for non-Secure
#define AIO_USERNAME    "stonez56"
#define AIO_KEY         "4cf46fc5f2c146e48f2cc81d167764c3"

/************ Global State (you don't need to change this!) ******************/

// WiFiFlientSecure for SSL/TLS support
WiFiClientSecure client;

//WiFiClient for non-secure
//WiFiClient client;

// Setup the MQTT client class by passing in the WiFi client and MQTT server and login details.
Adafruit_MQTT_Client mqtt(&client, AIO_SERVER, AIO_SERVERPORT, AIO_USERNAME, AIO_KEY);

// io.adafruit.com SHA1 fingerprint
const char* fingerprint = "AD 4B 64 B3 67 40 B5 FC 0E 51 9B BD 25 E9 7F 88 B6 2A A3 5B";

/****************************** Feeds ***************************************/

// Setup a feed called 'test' for publishing.
// Notice MQTT paths for AIO follow the form: <username>/feeds/<feedname>
//Adafruit_MQTT_Publish light_color = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/light-color");
//Adafruit_MQTT_Publish light_1 = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/light-1");

/*************************** Sketch Code ************************************/
// Bug workaround for Arduino 1.6.6, it seems to need a function declaration
// for some reason (only affects ESP8266, likely an arduino-builder bug).
void MQTT_connect();
void verifyFingerprint();

//set up a feed called 'light_1' / 'light_color' for subscribing to changes
Adafruit_MQTT_Subscribe light_1 = Adafruit_MQTT_Subscribe(&mqtt, AIO_USERNAME "/feeds/light-1");
Adafruit_MQTT_Subscribe light_color = Adafruit_MQTT_Subscribe(&mqtt, AIO_USERNAME "/feeds/light-color");

void setup() {
  Serial.begin(115200);
  delay(10);

  pinMode(relay1Pin, OUTPUT);
  pinMode(relay2Pin, OUTPUT);

  Serial.println(F("Home MQTT Light Control System"));

  // Connect to WiFi access point.
  Serial.println(); Serial.println();
  Serial.print("Connecting to ");
  Serial.println(WLAN_SSID);

  delay(1000);

  WiFi.begin(WLAN_SSID, WLAN_PASS);
  delay(2000);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();

  Serial.println("WiFi connected");
  Serial.print("IP address: "); Serial.println(WiFi.localIP());

  // check the fingerprint of io.adafruit.com's SSL cert
  verifyFingerprint();

  //Subscribe to Adafruit Feed!
  mqtt.subscribe(&light_1);
  mqtt.subscribe(&light_color);

}

uint32_t x = 0;

void loop() {

  String feed_lastread;
  // Ensure the connection to the MQTT server is alive (this will make the first
  // connection and automatically reconnect when disconnected).  See the MQTT_connect
  // function definition further below.
  MQTT_connect();


  Adafruit_MQTT_Subscribe *subscription;
  while ((subscription = mqtt.readSubscription(5000))) {


    if (subscription == &light_1) {

      Serial.print(F("Light-1:"));
      Serial.println((char *)light_1.lastread);
      feed_lastread = (char *)light_1.lastread;
      feed_lastread.trim(); //
      // Serial.println("Light 1:" + feed_lastread); //for verifying the varialble
      // *** NOTICE: adafruit.io publishes the data as strings, not numbers!!!
      if (feed_lastread == "ON") {
        switchRelay(2, 1);
      }
      if (feed_lastread == "OFF") {
        switchRelay(2, 0);
      }
    }


    if (subscription == &light_color) {
      Serial.print(F("Color-Light:"));
      Serial.println((char *)light_color.lastread);
      feed_lastread = (char *)light_color.lastread;
      feed_lastread.trim(); //
      // Serial.println("Color Light:" + feed_lastread); //for verifying the variable
      // *** NOTICE: adafruit.io publishes the data as strings, not numbers!!!
      if (feed_lastread == "ON") {
        switchRelay(1, 1);
      } else if (feed_lastread == "OFF") {
        switchRelay(1, 0);
      } else {
        colorLightFunction(feed_lastread.toInt());
      }
    }
  }
  /*
    switchRelay();
    Serial.print(F("\nSwitch is:"));
    if (relay1Status == 1) {
    Serial.print(F("On"));
    } else {
    Serial.print(F("Off"));
    }

    // Now we can publish stuff!
    Serial.print(F("\nSending val "));
    Serial.print(x);
    Serial.print(F(" to Relay1 feed..."));
    if (! Relay1.publish(x++)) {
    Serial.println(F("Failed"));
    } else {
    Serial.println(F("OK!"));
    }

    // Now we can publish stuff!
    Serial.print(F("\nSending val "));
    Serial.print(x);
    Serial.print(F(" to Relay2 feed..."));
    if (! Relay2.publish(x)) {
    Serial.println(F("Failed"));
    } else {
    Serial.println(F("OK!"));
    }

    // wait a couple seconds to avoid rate limit
    delay(5000);
  */

}
/*
   This function
*/
void colorLightFunction(int colorVariant) {
  Serial.println(F("Show different LED animiations"));
  switch (colorVariant) {
    case 10:
      break;
    case 20:
      break;
    case 30:
      break;
    case 40:
      break;
    case 50:
      break;
    case 60:
      break;
    case 70:
      break;
    case 80:
      break;
    case 90:
      break;
    case 100:
      break;
  }
}


/* This turn on/off relay switch

*/
void switchRelay(int relay, int stat) {

  if (relay == 2) { //this is light-1; 
    if (stat == 0) {
      digitalWrite(relay2Pin, LOW);
      Serial.println(F("Relay off"));
    }
    if (stat == 1) {
      digitalWrite(relay2Pin, HIGH);
      Serial.println(F("Relay on"));
    }
  }
  if (relay == 1) { //this is color-light; 
    if (stat == 0) {
      digitalWrite(relay1Pin, LOW);
      Serial.println(F("Relay off"));
    }
    if (stat == 1) {
      digitalWrite(relay1Pin, HIGH);
      Serial.println(F("Relay on"));
    }
  }
}


void verifyFingerprint() {

  const char* host = AIO_SERVER;

  Serial.print("Connecting to ");
  Serial.println(host);

  if (! client.connect(host, AIO_SERVERPORT)) {
    Serial.println("Connection failed. Halting execution.");
    while (1);
  }

  //  if (client.verify(fingerprint, host)) {
  //    Serial.println("Connection secure.");
  //  } else {
  //    Serial.println("Connection insecure! Halting execution.");
  //    while(1);
  //  }

}

// Function to connect and reconnect as necessary to the MQTT server.
// Should be called in the loop function and it will take care if connecting.
void MQTT_connect() {
  int8_t ret;

  // Stop if already connected.
  if (mqtt.connected()) {
    return;
  }

  Serial.print("Connecting to MQTT... ");

  uint8_t retries = 30;
  while ((ret = mqtt.connect()) != 0) { // connect will return 0 for connected
    Serial.println(mqtt.connectErrorString(ret));
    Serial.println("Retrying MQTT connection in 5 seconds...");
    mqtt.disconnect();
    delay(5000);  // wait 5 seconds
    retries--;
    if (retries == 0) {
      // basically die and wait for WDT to reset me
      while (1);
    }
  }

  Serial.println("MQTT Connected!");
}
