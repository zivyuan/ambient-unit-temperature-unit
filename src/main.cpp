#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <ESP8266HTTPClient.h>

#include <DHT.h>

// #define DEBUG_OFF
#include "SerialHelper.h"
ADC_MODE(ADC_TOUT_3V3);

#define LED               2
#define DHTTYPE           DHT22
#define DHTPIN            13
#define SOIL_HUMIDITY     12
#define RAINDROP          14
#define LIGHT_INTENSITY   16

#define PM25_LED          5
#define PM25              4


//
// == DHT sensor block
//
DHT dht;

void setupDHT() {
  dht.setup(DHTPIN);
}

void readDHT() {
  delay(dht.getMinimumSamplingPeriod());

  float humidity = dht.getHumidity();
  float temperature = dht.getTemperature();

  sprint("DHT: ");
  sprint(dht.getStatusString());
  sprint("\t");
  sprint(String(humidity));
  sprint("\t\t");
  sprint(String(temperature));
  sprint("\t\t");
  sprintln(String(dht.toFahrenheit(temperature)));
}

// END of DHT sensor block


// == Soil Moisture block
void setupSoilMoisture() {
  pinMode(SOIL_HUMIDITY, INPUT);
}

void readSoilMoisture() {
  int moisture = digitalRead(SOIL_HUMIDITY);
  sprintln("Soil moisture: " + String(moisture));
}
// == END of soil moisture block

// == Raindrop block

void setupRaindrop() {
  pinMode(RAINDROP, OUTPUT);
  digitalWrite(RAINDROP, LOW);
}

int raindropAVG = 0;
void readRaindrop() {
  digitalWrite(RAINDROP, HIGH);
  delay(10);
  int dropA = analogRead(A0);
  delay(10);
  int dropB = analogRead(A0);

  raindropAVG = dropA * 0.3 + dropB * 0.7;

  sprintln("Raindrop: " + String(raindropAVG));
  delay(300);  // Delay for test
  digitalWrite(RAINDROP, LOW);
}
// == END of raindrop block

// == Light Intensity block
void setupLightIntensity() {
  pinMode(LIGHT_INTENSITY, OUTPUT);
  digitalWrite(LIGHT_INTENSITY, LOW);
}

void readLightIntensity() {
  digitalWrite(LIGHT_INTENSITY, HIGH);
  int a = analogRead(A0);
  delay(50);
  int b = analogRead(A0);
  int light = a * 0.3 + b * 0.7;
  sprintln("Light Intensity: " + String(light));
  delay(300);  // Delay for test
  digitalWrite(LIGHT_INTENSITY, LOW);
}

// == END of light sensor block

// == WIFI setup block
#define AP_NAME       "Gozi2016"
#define PASSWORD      "pass4share"

/**
 * Convert IP from int
 */
String ipFromInt(unsigned int ip) {
   unsigned int a = int( ip / 16777216 );
   unsigned int b = int( (ip % 16777216) / 65536 );
   unsigned int c = int( ((ip % 16777216) % 65536) / 256 );
   unsigned int d = int( ((ip % 16777216) % 65536) % 256 );
   String ipstr = String(d) + '.' + String(c) + '.' + String(b) + '.' + String(a);

   return ipstr;
}

ESP8266WiFiMulti WiFiMulti;

void setupWIFI() {
  int ledOn = 0;

  WiFi.mode(WIFI_STA);
  WiFiMulti.addAP(AP_NAME, PASSWORD);
  // Wait for connection
  sprintln("Connect to Wifi " + String(AP_NAME));
  while ((WiFiMulti.run() != WL_CONNECTED)) {
    ledOn = ledOn == 0 ? 1 : 0;
    digitalWrite(LED, ledOn);
    sprint(".");
    delay(200);
  }

  String ID = WiFi.macAddress();
  sprintln(" ");
  sprintln("WIFI connect success");
  sprintln(ipFromInt(WiFi.localIP()));
  sprintln(ID);
}

// == END of WIFI setup block

void setup() {

  pinMode(LED, OUTPUT);

  sbegin(115200);

  delay(100);

  // setupWIFI();

  // setupDHT();
  // setupSoilMoisture();
  setupRaindrop();
  setupLightIntensity();

}

int ledState = LOW;

void loop() {

  ledState = ! ledState;
  digitalWrite(LED, ledState);

  // readDHT();
  //
  // delay(1000);

  // readSoilMoisture();
  //
  // delay(1000);

  readRaindrop();

  delay(1000);

  readLightIntensity();

  // int VVV = analogRead(A0);
  //
  // sprintln("ADC Value: " + String(VVV));

  delay(1000);
}
