#include <Arduino.h>
#include "Wire.h"
#include <ADS1X15.h>
#include <SHTSensor.h>


// #define DEBUG_OFF
#include "SerialHelper.h"

#define PM25_LED             3

// PM2.5 sample time mesured as micro second
#define PM25_SAMPLE_TIME     280



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

String ID;
// Data report handle
void report(String event, String data = "") {

}

void pollAlertReadyPin();
void readPM25();
void blink();


ADS1115 adc0(ADS1115_ADDRESS);
SHTSensor sht3x = SHTSensor::SHT3X;

void setup() {

  sbegin(115200);

  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(PM25_LED, OUTPUT);

  digitalWrite(LED_BUILTIN, LOW);
  digitalWrite(PM25_LED, LOW);

  delay(100);

  // join I2C bus
  sprint("Start I2C...\n");
  Wire.begin();

  delay(500);

  sht3x.init();

  blink();
}

int ledState = LOW;
float dustDensity = 0;

void loop() {

  if (sht3x.readSample()) {
    Serial.print("  RH: ");
    Serial.println(sht3x.getHumidity());
    Serial.print("   T: ");
    Serial.println(sht3x.getTemperature());
  }


  readPM25();


  sprintln();

  blink();
  blink();


  delay(2000);

}



void blink() {
  digitalWrite(LED_BUILTIN, HIGH);
  delay(300);
  digitalWrite(LED_BUILTIN, LOW);
  delay(200);
}


int readPM25FromAdc() {
  adc0.setGain(0);
  delay(10);

  digitalWrite(PM25_LED, HIGH);
  delayMicroseconds(PM25_SAMPLE_TIME);

  uint16_t data = adc0.readADC(0);

  digitalWrite(PM25_LED, LOW);

  return data;
}

void readPM25() {
  int densityRead = 0;
  float newDensity = 0;
  densityRead += readPM25FromAdc();
  delay(1);
  densityRead += readPM25FromAdc();
  delay(1);
  densityRead += readPM25FromAdc();
  delay(1);
  densityRead += readPM25FromAdc();
  delay(1);
  densityRead += readPM25FromAdc();

  newDensity = (densityRead / 5) * (3.3 / 32767);
  dustDensity = dustDensity * 0.17 + newDensity * 0.83;
  float pm25 = dustDensity;// * 1000;
  sprint("PM25: ");
  sprintln(String(pm25) + " ug/m3");
}