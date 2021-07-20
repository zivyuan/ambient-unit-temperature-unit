#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <Wire.h>
#include <ADS1X15.h>
#include <SHTSensor.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

// #define DEBUG 1

#define MINIUTE_IN_US 60000000
#define PIN_POWER 12

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

#define CLIENT_ID "SH123456|securemode=3,timestamp=36580,signmethod=hmacsha256|"
// 算法工具: http://iot-face.oss-cn-shanghai.aliyuncs.com/tools.htm 进行加密生成password
// password教程 https://www.yuque.com/cloud-dev/iot-tech/mebm5g
// clientIdSH123456deviceNameSweetHome901productKeya1Kj2P1kRYttimestamp36580
#define MQTT_PASSWD "0a7a858f1f356472c9f03ff382b016e742a01b91d2923ed4783f0f680fa12f87"

#define ALINK_BODY_FORMAT "{\"id\":\"ESP8266\",\"version\":\"1.0\",\"method\":\"thing.event.property.post\",\"params\":%s}"
#define ALINK_TOPIC_PROP_POST "/sys/" PRODUCT_KEY "/" DEVICE_NAME "/thing/event/property/post"

#define REPORT_DATA "{\"CurrentTemperature\":%f,\"CurrentHumidity\":%f,\"PM25\":%f}"
#define REPORT_INTERVAL (10 * MINIUTE_IN_US)

// #define DEBUG_OFF
#include "SerialHelper.h"

#define PM25_LED     14

// PM2.5 sample time mesured as micro second
#define PM25_SAMPLE_TIME 280

// Variable define

WiFiClient espClient;
PubSubClient pubClient(espClient);

ADS1115 adc0(ADS1115_ADDRESS);
SHTSensor sht3x = SHTSensor::SHT3X;

uint32_t lastMs = 0;
float dustDensity = 0;
bool wifi_connected = 0;
bool mqtt_connected = 0;

void blink(int h = 250, int l = 250)
{
  digitalWrite(LED_BUILTIN, HIGH);
  delay(h);
  digitalWrite(LED_BUILTIN, LOW);
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
  sprint("PM25: ");
  sprintln(String(pm25) + " ug/m3");

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

void wifiInit()
{
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWD); //连接WiFi
  // int count = 0;
  // while (WiFi.status() != WL_CONNECTED)
  // {
  //   delay(1000);
  //   Serial.println("Waiting WiFi connect...");
  //   count ++;
  //   if (count > 30) {
  //     // WiFi 连接失败后休眠重试
  //     uint64_t sleepTime = 1 * MINIUTE_IN_US;
  //     Serial.println("deep sleep start...");
  //     ESP.deepSleep(sleepTime); // uint64_t time_us
  //   }
  // }
  // Serial.println("Connected to AP");
  // Serial.print("IP address: ");
  // Serial.println(WiFi.localIP());

  // pubClient.setServer(MQTT_SERVER, MQTT_PORT); /* 连接WiFi之后，连接MQTT服务器 */
  // pubClient.setCallback(callback);
}

void mqttCheckConnect()
{
  int count = 0;
  while (!pubClient.connected())
  {
    Serial.println("Connecting to MQTT Server ...");
    if (pubClient.connect(CLIENT_ID, MQTT_USRNAME, MQTT_PASSWD))
    {
      Serial.println("MQTT Connected!");
    }
    else
    {
      Serial.print("MQTT Connect err:");
      Serial.println(pubClient.state());
      count++;
      if (count >= 10) {
        return;
      }

      delay(5000);
    }
  }
}

void mqttIntervalPost()
{
  char param[512];
  char jsonBuf[1024];
  boolean d;
  float temperature = 0.0;
  float humidity = 0.0;
  float pm25 = 0.0;

  // Read temperature & humidity
  if (sht3x.readSample())
  {
    temperature = sht3x.getTemperature();
    humidity = sht3x.getHumidity();
  }

  //
  pm25 = readPM25();

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

// Main code start here

void setup()
{

  sbegin(115200);

  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(PIN_POWER, OUTPUT);
  pinMode(PM25_LED, OUTPUT);

  digitalWrite(LED_BUILTIN, LOW);
  digitalWrite(PIN_POWER, LOW);
  digitalWrite(PM25_LED, LOW);

  delay(100);

  // join I2C bus
  Wire.begin();

  wifiInit();

  delay(500);

  sht3x.init();
  pubClient.setBufferSize(256);
  pubClient.setKeepAlive(30);

  blink();
}

void loop()
{
  if (WiFi.status() != WL_CONNECTED) {
    wifi_connected = 0;
    Serial.println("Waiting WiFi ...");

    blink();
    delay(500);
    return;
  }

  if (!wifi_connected) {
    wifi_connected = 1;

    Serial.println("Connected to AP");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());

    /* 连接WiFi之后，连接MQTT服务器 */
    pubClient.setServer(MQTT_SERVER, MQTT_PORT);
    pubClient.setCallback(callback);
  }

  if (!pubClient.connected()) {
    mqtt_connected = 0;

    blink();
    blink();

    Serial.println("Connecting to MQTT Server ...");
    if (pubClient.connect(CLIENT_ID, MQTT_USRNAME, MQTT_PASSWD))
    {
      Serial.println("MQTT Connected!");
    }
    else
    {
      Serial.print("MQTT Connect err:");
      Serial.println(pubClient.state());

      delay(4000);
      return;
    }
  }

  if (!mqtt_connected) {
    mqtt_connected = 1;
  }

  Serial.println("Start report...");

  // Power control
  // digitalWrite(PIN_POWER, HIGH);
  // delay(1050); // Delay at least 1000ms for PM2.5 sensor

  mqttCheckConnect();
  /* 上报 */
  mqttIntervalPost();

  pubClient.loop();


  #if defined(DEBUG)
    Serial.println("Delay for next loop...");
    delay(5 * MINIUTE_IN_US / 1000);
  #else
    // DeepSleep
    // 设备睡眠间隔
    uint64_t sleepTime = 5 * MINIUTE_IN_US;
    Serial.println("deep sleep start...");
    ESP.deepSleep(sleepTime); // uint64_t time_us
  #endif
}
