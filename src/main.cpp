#include <Arduino.h>
#include <ESP8266WiFi.h>
// #include <ESP8266WiFiMulti.h>
#include <ESP8266HTTPClient.h>
#include "ADS1115.h"
#include "SHTSensor.h"

// #define DEBUG_OFF
#include "SerialHelper.h"

// ESP8266 work mode configuretion
// ADC_MODE(ADC_TOUT_3V3);

#define API_BASE     String("http://gemdesign.cn/milk/dailylog.php")

#define LED                  2

#define ADS1115_ALERT        15

#define PM25_LED             14
// PM2.5 sample time mesured as micro second
#define PM25_SAMPLE_TIME     280
#define PM25_DELTA_TIME      40


// == WIFI setup block
#define AP_NAME       "Gozi2016"
#define PASSWORD      "pass4share"

// #define NO_REPORT      1

#ifdef NO_REPORT

// Report interval 10 minute
#define REPORT_INTERVAL        5000

#else

// Report interval 10 minute
#define REPORT_INTERVAL        600000

#endif

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
#ifndef NO_REPORT

  HTTPClient http;

  event.replace(" ", "%20");
  data.replace(" ", "%20");

  String request = API_BASE + "?id=" + ID + "&e=" + event + "&d=" + data;

  http.begin(request);
  http.GET();
  http.end();

#endif
}


ADS1115 adc0(ADS1115_DEFAULT_ADDRESS);
SHTSensor sht;

void setup() {

  sbegin(115200);

  pinMode(LED, OUTPUT);
  pinMode(PM25_LED, OUTPUT);

  digitalWrite(LED, LOW);
  digitalWrite(PM25_LED, LOW);

  delay(100);

  // setupWIFI();
  int ledOn = 0;

  WiFi.mode(WIFI_STA);
  WiFi.begin("Gozi2016", "pass4share");
  // WiFi.begin("ChinaNGB-wLxf5w", "KivikGiK");

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    ledOn = ledOn == 0 ? 1 : 0;
    digitalWrite(LED, ledOn);
    sprint(".");
  }

  ID = WiFi.macAddress();
  sprintln(" ");
  sprintln("WIFI connect success");
  sprintln(ipFromInt(WiFi.localIP()));
  sprintln(ID);

  // Wait for wifi prepare
  delay(100);

  //I2Cdev::begin();  // join I2C bus
  Wire.begin();

  // Wait I2C prepare
  delay(100);

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

  // To get output from this method, you'll need to turn on the
  //#define ADS1115_SERIAL_DEBUG // in the ADS1115.h file
  #ifdef ADS1115_SERIAL_DEBUG
  adc0.showConfigRegister();
  sprint("HighThreshold="); sprintln(adc0.getHighThreshold(),BIN);
  sprint("LowThreshold="); sprintln(adc0.getLowThreshold(),BIN);
  #endif

  if (sht.init()) {
    sprint("init(): success\n");
  } else {
    sprint("init(): failed\n");
  }
  sht.setAccuracy(SHTSensor::SHT_ACCURACY_MEDIUM); // only supported by SHT3x

  report("ENV Monitor ONLINE");

  delay(500);


  digitalWrite(LED, LOW);
  delay(2000);
  digitalWrite(LED, HIGH);
}

/** Poll the assigned pin for conversion status
 */
void pollAlertReadyPin() {
  for (uint32_t i = 0; i<100000; i++)
    if (!digitalRead(ADS1115_ALERT)) return;
   sprintln("Failed to wait for AlertReadyPin, it's stuck high!");
}

int readPM25() {
  digitalWrite(PM25_LED, HIGH);
  delayMicroseconds(PM25_SAMPLE_TIME);
  adc0.setMultiplexer(ADS1115_MUX_P2_NG);
  adc0.triggerConversion();
  int data = adc0.getConversionP2GND();
  digitalWrite(PM25_LED, LOW);

  return data;
}

int ledState = LOW;
float dustDensity = 0;

void loop() {

  // ledState = ! ledState;
  // digitalWrite(LED, ledState);
  digitalWrite(LED, LOW);

  // Read soil humidity
  //
  adc0.setMultiplexer(ADS1115_MUX_P0_NG);
  adc0.triggerConversion();
  float soilHumidity = (float)adc0.getConversionP0GND();
  float shum = (soilHumidity / 32767.0) * 100.0;
  shum = shum > 100 ? 100 : 0;
  sprint("Soil Humidity: ");
  sprintln(String(shum) + "%");
  report("Soil Humidity", String(shum));
  //
  // === END of soil humidity


  delay(5);


  // Read water level
  //
  adc0.setMultiplexer(ADS1115_MUX_P1_NG);
  adc0.triggerConversion();
  float rainv = (float)adc0.getConversionP1GND();
  float rain = (rainv / 32767.0) * 100.0;
  rain = rain > 100 ? 100 : 0;
  sprint("Raining: ");
  sprint(rain);
  sprintln("%");
  report("Raining Level", String(rain));
  //
  // === END of water level


  // Read PM2.5 sensor
  //
  int densityRead = 0;
  float newDensity = 0;
  densityRead += readPM25();
  delay(1);
  densityRead += readPM25();
  delay(1);
  densityRead += readPM25();
  delay(1);
  densityRead += readPM25();
  delay(1);
  densityRead += readPM25();

  newDensity = (densityRead / 5) * (3.3 / 32767);
  dustDensity = dustDensity * 0.17 + newDensity * 0.83;
  float pm25 = dustDensity * 1000;
  sprint("PM25: ");
  sprintln(String(pm25) + " ug/m3");
  report("PM25", String(pm25) + " ug/m3");
  //
  // === END of PM2.5 sensor

  delay(5);


  // SHT3x sensor
  //
  if (sht.readSample()) {
    float humidity = sht.getHumidity();
    sprint("SHT:\n");
    sprint("  RH: ");
    sprint(humidity);
    sprint("\n");
    float temperature = sht.getTemperature();
    sprint("  T:  ");
    sprint(temperature);
    sprint("\n");

    report("Humidity", String(humidity));
    report("Temperature", String(temperature));
  } else {
    sprint("Error in readSample()\n");
  }
  //
  // === END of SHT3x sensor

  sprintln();

  digitalWrite(LED, HIGH);

  delay(REPORT_INTERVAL);

}
