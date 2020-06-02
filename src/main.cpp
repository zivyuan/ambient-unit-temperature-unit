#include <Arduino.h>
#include "Wire.h"
#include "ADS1115.h"
#include "BME280_t.h"


// #define DEBUG_OFF
#include "SerialHelper.h"

#define LED                  13

#define ADS1115_ALERT        15

#define PM25_LED             3

// PM2.5 sample time mesured as micro second
#define PM25_SAMPLE_TIME     280
#define PM25_DELTA_TIME      40

#define ASCII_ESC 27

#define MYALTITUDE  150.50

char bufout[10];

BME280<> BMESensor;



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


void initAdc();
void pollAlertReadyPin();
void readPM25();
void blink();


ADS1115 adc0(ADS1115_DEFAULT_ADDRESS);

void setup() {

  sbegin(115200);

  pinMode(LED, OUTPUT);
  pinMode(PM25_LED, OUTPUT);

  digitalWrite(LED, LOW);
  digitalWrite(PM25_LED, LOW);

  delay(100);

  // join I2C bus
  Wire.begin();

  if (!Wire.available()) {
    sprint("Wire not available.\n");
  }

  // Wait I2C prepare
  delay(100);

  BMESensor.begin();

  initAdc();


  // To get output from this method, you'll need to turn on the
  // #ifdef ADS1115_SERIAL_DEBUG
  // adc0.showConfigRegister();
  // sprint("HighThreshold=");
  // sprintln(adc0.getHighThreshold(),BIN);
  // sprint("LowThreshold=");
  // sprintln(adc0.getLowThreshold(),BIN);
  // #endif

  delay(500);


  blink();
}

int ledState = LOW;
float dustDensity = 0;

void loop() {

  // readPM25();
  // delay(5);

  // BME280
  //
    BMESensor.refresh();                                                  // read current sensor data
  // sprintf(bufout,"%c[1;0H",ASCII_ESC);
  // Serial.println(bufout);
  Serial.println("BME 280:");

  Serial.print("  Temperature: ");
  Serial.print(BMESensor.temperature);                                  // display temperature in Celsius
  Serial.println("C");

  Serial.print("  Humidity:    ");
  Serial.print(BMESensor.humidity);                                     // display humidity in %
  Serial.println("%");

  Serial.print("  Pressure:    ");
  Serial.print(BMESensor.pressure  / 100.0F);                           // display pressure in hPa
  Serial.println("hPa");

  float relativepressure = BMESensor.seaLevelForAltitude(MYALTITUDE);
  Serial.print("  RelPress:    ");
  Serial.print(relativepressure  / 100.0F);                             // display relative pressure in hPa for given altitude
  Serial.println("hPa");

  Serial.print("  Altitude:    ");
  Serial.print(BMESensor.pressureToAltitude(relativepressure));         // display altitude in m for given pressure
  Serial.println("m");
  //
  // End of BME280


  sprintln();

  blink();
  blink();


  delay(2000);

}



void blink() {
  digitalWrite(LED, HIGH);
  delay(300);
  digitalWrite(LED, LOW);
  delay(200);
}

/** Poll the assigned pin for conversion status
 */
void pollAlertReadyPin() {
  for (uint32_t i = 0; i<100000; i++)
    if (!digitalRead(ADS1115_ALERT)) return;
   sprintln("Failed to wait for AlertReadyPin, it's stuck high!");
}


int readFromAdc() {
  digitalWrite(PM25_LED, HIGH);
  delayMicroseconds(PM25_SAMPLE_TIME);
  adc0.setMultiplexer(ADS1115_MUX_P0_NG);
  adc0.triggerConversion();
  int data = adc0.getConversionP0GND();
  digitalWrite(PM25_LED, LOW);

  return data;
}

void readPM25() {
  int densityRead = 0;
  float newDensity = 0;
  densityRead += readFromAdc();
  delay(1);
  densityRead += readFromAdc();
  delay(1);
  densityRead += readFromAdc();
  delay(1);
  densityRead += readFromAdc();
  delay(1);
  densityRead += readFromAdc();

  newDensity = (densityRead / 5) * (3.3 / 32767);
  dustDensity = dustDensity * 0.17 + newDensity * 0.83;
  float pm25 = dustDensity * 1000;
  sprint("PM25: ");
  sprintln(String(pm25) + " ug/m3");
}



void initAdc() {

  sprintln("Testing device connections...");
  sprintln(adc0.testConnection() ? "ADS1115 connection successful" : "ADS1115 connection failed");

  adc0.initialize(); // initialize ADS1115 16 bit A/D chip

  // We're going to do single shot sampling
  adc0.setMode(ADS1115_MODE_SINGLESHOT);

  // Slow things down so that we can see that the "poll for conversion" code works
  adc0.setRate(ADS1115_RATE_860);

  // Set the gain (PGA) +/- 6.144V
  // Note that any analog input must be higher than –0.3V and less than VDD +0.3
  adc0.setGain(ADS1115_PGA_2P048);
  // ALERT/RDY pin will indicate when conversion is ready

  // pinMode(ADS1115_ALERT, INPUT_PULLUP);
  // adc0.setConversionReadyPinMode();

}