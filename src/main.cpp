#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <Wire.h>
#include <ADS1X15.h>
#include <SHTSensor.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <Adafruit_SSD1306.h>
#include <EEPROM.h>
#include <NTPClient.h>
#include <WiFiUdp.h>

// #define DEBUG 1

#define LED_OFF        HIGH
#define LED_ON         LOW

#define EADDR_TEMPERATURE   0   // Size 4
#define EADDR_HUMIDITY      4   // Size 4
#define EADDR_PM25          8   // Size 4

#define MINIUTE_IN_US     60000000
#define PIN_POWER         D7     // GPIO13
#define PIN_DISPLAY_SDA   D6     // GPIO12
#define PIN_DISPLAY_SCL   D5     // GPIO14
#define PM25_LED          D3     // GPIO00

/* 连接您的WIFI SSID和密码 */
// #define WIFI_SSID "ChinaNGB-wLxf5w"
// #define WIFI_PASSWD "KivikGiK"
#define WIFI_SSID "Gozi2016"
#define WIFI_PASSWD "pass4share"

/* 设备的三元组信息*/
#define PRODUCT_KEY "a1Kj2P1kRYt"
#define DEVICE_NAME "SweetHome901"
#define DEVICE_SECRET "4d03a5cf080e8f48efe5d6e160796d61"
#define REGION_ID "cn-shanghai"

/* 线上环境域名和端口号，不需要改 */
#define MQTT_SERVER PRODUCT_KEY ".iot-as-mqtt." REGION_ID ".aliyuncs.com"
#define MQTT_PORT 1883
#define MQTT_USRNAME DEVICE_NAME "&" PRODUCT_KEY

#define CLIENT_ID "SH103456|securemode=3,timestamp=136580,signmethod=hmacsha256|"
// 算法工具: http://iot-face.oss-cn-shanghai.aliyuncs.com/tools.htm 进行加密生成password
// password教程 https://www.yuque.com/cloud-dev/iot-tech/mebm5g
// clientIdSH103456deviceNameSweetHome901productKeya1Kj2P1kRYttimestamp136580
#define MQTT_PASSWD "d737ad504a9ede7b002dc0a7bb681cba0e1d004360ed01ac36476e7e5a100fbb"

#define ALINK_BODY_FORMAT "{\"id\":\"SH103456\",\"version\":\"1.0\",\"method\":\"thing.event.property.post\",\"params\":%s}"
#define ALINK_TOPIC_PROP_POST "/sys/" PRODUCT_KEY "/" DEVICE_NAME "/thing/event/property/post"

#define REPORT_DATA "{\"CurrentTemperature\":%f,\"CurrentHumidity\":%f,\"PM25\":%f}"
// Report interval, minute
#define REPORT_INTERVAL 5

// #define DEBUG_OFF
#include "SerialHelper.h"

// PM2.5 sample time mesured as micro second
#define PM25_SAMPLE_TIME 280

#define SCREEN_WIDTH     128
#define SCREEN_HEIGHT    32
#define ROW_MSG          24

// Variable define

WiFiClient espClient;
PubSubClient pubClient(espClient);

ADS1115 adc0(ADS1115_ADDRESS);
SHTSensor sht3x = SHTSensor::SHT3X;
// TwoWire displayWire;
Adafruit_SSD1306 display = Adafruit_SSD1306(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire);
WiFiUDP ntpUDP;
NTPClient ntpClient(ntpUDP, 8 * 60 * 60);

uint32_t lastMs = 0;
float dustDensity = 0;
bool wifi_connected = 0;
bool mqtt_connected = 0;
// Sensor data vars
float temperature = 0.0;
float humidity = 0.0;
float pm25 = 0.0;

void blink(int h = 250, int l = 250)
{
  digitalWrite(LED_BUILTIN, LED_ON);
  delay(h);
  digitalWrite(LED_BUILTIN, LED_OFF);
  delay(l);
}

int readPM25FromAdc()
{
  adc0.setGain(0);
  delay(10);

  digitalWrite(PM25_LED, HIGH);
  delayMicroseconds(PM25_SAMPLE_TIME);

  uint16_t data = adc0.readADC(0);

  digitalWrite(PM25_LED, LOW);

  return data;
}

float_t readPM25()
{
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
  float_t pm25 = dustDensity * 1000;

  return pm25;
}

//
// MQTT Callback
//
void callback(char *topic, byte *payload, unsigned int length)
{
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  payload[length] = '\0';
  Serial.println((char *)payload);
}

void readSensor() {
  digitalWrite(PIN_POWER, HIGH);
  // Switch I2C ports to data
  Wire.begin(PIN_WIRE_SDA, PIN_WIRE_SCL);
  delay(100);

  // Read temperature & humidity
  if (sht3x.readSample())
  {
    Serial.print("Temperature: ");
    temperature = sht3x.getTemperature();
    Serial.println(temperature);
    Serial.print("Humidity: ");
    humidity = sht3x.getHumidity();
    Serial.println(humidity);
  } else {
    Serial.println("Temp sensor not ready!");
  }
  // Wait for pm2.5 sensor initial
  delay(1000);
  sprint("PM25: ");
  pm25 = readPM25();
  sprintln(String(pm25) + " ug/m3");

  // Switch I2C ports to display
  Wire.begin(PIN_DISPLAY_SDA, PIN_DISPLAY_SCL);
  delay(100);

  digitalWrite(PIN_POWER, LOW);

}

void mqttIntervalPost()
{
  char param[512];
  char jsonBuf[1024];
  boolean d;

  sprintf(param, REPORT_DATA, temperature, humidity, pm25);
  sprintf(jsonBuf, ALINK_BODY_FORMAT, param);
  Serial.print("Report: ");
  Serial.println(jsonBuf);
  // Demo:
  // {"id":"ESP8266","version":"1.0","method":"thing.event.property.post","params":{"CurrentTemperature":27.870605,"CurrentHumidity":78.672462,"PM25":5.466966}}
  d = pubClient.publish(ALINK_TOPIC_PROP_POST, jsonBuf);
  if (d)
  {
    Serial.println("publish success");
  }
  else
  {
    Serial.println("publish fail");
  }
}

// Load data from EEPROM
void loadData() {
  EEPROM.begin(12 + 1);

  temperature = EEPROM.read(EADDR_TEMPERATURE);
  humidity = EEPROM.read(EADDR_HUMIDITY);
  pm25 = EEPROM.read(EADDR_PM25);

  EEPROM.end();
}

void saveData() {
  EEPROM.begin(12 + 1);

  EEPROM.write(EADDR_TEMPERATURE, temperature);
  EEPROM.write(EADDR_HUMIDITY, humidity);
  EEPROM.write(EADDR_PM25, pm25);

  EEPROM.end();
}

void displayData() {

  char str[24];
  sprintf(str, "T:%0.1f C", temperature);
  Serial.println(str);
  display.setCursor(0, 0);
  display.print(str);
  display.drawCircle(38, 1, 1, WHITE);

  sprintf(str, "H:%0.1f%%", humidity);
  Serial.println(str);
  display.setCursor(64, 0);
  display.print(str);

  sprintf(str, "PM25: %0.1fug/m3", pm25);
  Serial.println(str);
  display.setCursor(0, 10);
  display.print(str);

}

// Main code start here

void setup()
{
  //
  // For the reason of ADS1115 and SSD1306 module both have pull
  // up resisters on SDA and SCL pins, and this is the reason to
  // cause IIC comunication confused.
  // To solve this problem, we need to switch IIC pins before
  // transition. [Multiple I2C ports](https://github.com/esp8266/Arduino/issues/3063)
  // In this project, switch pins to default when read sensor data
  // then swith to display IIC pins.
  // Other solution: [twowire i2c library don't work with multiple instance #7894](https://github.com/esp8266/Arduino/issues/7894)
  //
  // Initial wire as display communication
  Wire.begin(PIN_DISPLAY_SDA, PIN_DISPLAY_SCL);

  sbegin(115200);

  delay(100);

  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.print("Display initial failed!");
  }
  display.clearDisplay();
  display.setTextColor(WHITE);
  display.setCursor(2, 10);
  display.print("Initializing...");
  display.display();

  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(PIN_POWER, OUTPUT);
  pinMode(PM25_LED, OUTPUT);

  digitalWrite(LED_BUILTIN, LED_OFF);
  digitalWrite(PIN_POWER, LOW);
  digitalWrite(PM25_LED, LOW);

  delay(100);

  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);
  WiFi.begin(WIFI_SSID, WIFI_PASSWD);

  delay(500);

  sht3x.init();

  pubClient.setBufferSize(256);
  pubClient.setKeepAlive(30);
  pubClient.setServer(MQTT_SERVER, MQTT_PORT);
  pubClient.setCallback(callback);

  ntpClient.begin();

  blink();
}

void loop()
{
  char msg[128];
  display.clearDisplay();

  if (WiFi.status() != WL_CONNECTED) {
    wifi_connected = 0;
    sprintf(msg, "[%d] Connect WiFi...", WiFi.status());
    Serial.println(msg);

    display.setCursor(0, ROW_MSG);
    display.print(msg);

    display.display();

    blink();
    delay(500);
    return;
  }

  if (!wifi_connected) {
    wifi_connected = 1;

    Serial.println("Connected to AP");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
  }


  display.fillRect(0, ROW_MSG, 128, 32, BLACK);
  display.setCursor(0, ROW_MSG);
  display.print("Reading data...");
  display.display();

  ntpClient.update();

  readSensor();

  delay(50);

  display.clearDisplay();

  displayData();

  if (!pubClient.connected()) {
    mqtt_connected = 0;

    blink();
    blink();

    Serial.println("Connecting to MQTT Server ...");

    display.setCursor(0, ROW_MSG);
    display.print("Connect MQTT...");
    display.display();

    if (pubClient.connect(CLIENT_ID, MQTT_USRNAME, MQTT_PASSWD))
    {
      Serial.println("MQTT Connected!");
    }
    else
    {
      sprintf(msg, "Connect MQTT err: %d", pubClient.state());
      Serial.println(msg);
      display.fillRect(0, ROW_MSG, 128, 32, BLACK);
      display.setCursor(0, ROW_MSG);
      display.print(msg);
      display.display();

      delay(4000);
      return;
    }
  }

  if (!mqtt_connected) {
    mqtt_connected = 1;
  }

  Serial.println("Start report...");
  // mqttCheckConnect();
  /* 上报 */
  display.fillRect(0, ROW_MSG, 128, 32, BLACK);
  display.setCursor(0, ROW_MSG);
  display.print("Report data...");
  display.display();

  mqttIntervalPost();

  delay(100);

  display.fillRect(0, ROW_MSG, 128, 32, BLACK);
  display.setCursor(0, ROW_MSG);
  display.print(ntpClient.getFormattedTime());

  display.display();

  pubClient.disconnect();
  WiFi.disconnect();

  #if defined(DEBUG)
    Serial.println("Delay for next loop...");
    delay(5 * MINIUTE_IN_US / 1000);
  #else
    // DeepSleep
    // 设备睡眠间隔
    uint64_t sleepTime = REPORT_INTERVAL * MINIUTE_IN_US;
    Serial.println("deep sleep start...");
    ESP.deepSleep(sleepTime); // uint64_t time_us
  #endif
}
