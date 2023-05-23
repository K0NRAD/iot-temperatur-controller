#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <DFRobot_DHT20.h>
#include <Wire.h>
#define ARDUINOJSON_USE_DOUBLE 0
#include <ArduinoJson.h>
#include "credentials.h"

#define NO_ERR 0
#define TCA_SELECT_FAILED_TO_INIT_12C 1
#define TCA_SELECT_FAILED_TO_SELECT_I2C_PORT 2

#define TCA 0x70
#define I2C_SDA 0
#define I2C_SCL 4

#define NUM_SENSORS 3
#define DHT20_1_MULTIPLEXER_PORT 0
#define DHT20_2_MULTIPLEXER_PORT 3
#define DHT20_3_MULTIPLEXER_PORT 6

#define CONTROLLER_GAUGE_MULTIPLEXER_PORT 5
#define POWERBANK_GAUGE_MULTIPLEXER_PORT 2

// WiFi credentials
// const char *ssid = "SSID";
// const char *password = "PASSWORD";

// MQTT Broker settings
const char *mqtt_server = "broker.hivemq.com";
const int mqtt_port = 1883;
// const char *mqtt_user = "USER";
// const char *mqtt_password = "PASSWORD";
const char *topic = "sit/lws/lab";

WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);
DFRobot_DHT20 dht20[NUM_SENSORS];
int multiplexerPorts[] = {DHT20_1_MULTIPLEXER_PORT, DHT20_2_MULTIPLEXER_PORT, DHT20_3_MULTIPLEXER_PORT};

// funktion protoypen
uint8_t tcaSelect(uint8_t tcaID);
float getTemperature(uint8_t tcaID);
int getHumidity(uint8_t tcaID);
void setup_wifi();
void mqttReconnect();
void publishMeasurements();

void setup()
{
  Serial.begin(115200);

  setup_wifi();
  mqttClient.setServer(mqtt_server, mqtt_port);

  Wire.begin(I2C_SDA, I2C_SCL);

  for (size_t inx = 0; inx < NUM_SENSORS; inx++)
  {
    tcaSelect(multiplexerPorts[inx]);
    if (dht20[inx].begin())
    {
      Serial.printf("Failed to init DHT20[%d].\nESP will reboot in 30 seconds.\n", inx + 1);
      delay(30000);
      ESP.restart();
    }
  }
}

void loop()
{
  publishMeasurements();
  delay(10000);
}

float getTemperature(uint8_t inx)
{
  tcaSelect(multiplexerPorts[inx]);
  float value = dht20[inx].getTemperature();
  return ((int)(value * 100 + 0.5) / 100.0);
}

int getHumidity(uint8_t inx)
{
  tcaSelect(multiplexerPorts[inx]);
  float value = dht20[inx].getHumidity();
  return (int)(value * 100.0 + 0.5);
}

void publishMeasurements()
{
  if (!mqttClient.connected())
  {
    mqttReconnect();
  }

  mqttClient.loop();

  StaticJsonDocument<500> doc;

  JsonArray measurement = doc.createNestedArray("measurement");

  for (size_t inx = 0; inx < NUM_SENSORS; inx++)
  {
    float temperature = getTemperature(inx);
    int humidity = getHumidity(inx);
    JsonObject lab = measurement.createNestedObject();
    lab["sensor"] = inx + 1;
    lab["temperature"] = temperature;
    lab["humidity"] = humidity;
  }

  // serializeJsonPretty(doc, Serial);
  // Serial.println();

  char buffer[500] = {0};

  serializeJson(doc, buffer, 500);
  mqttClient.publish(topic, buffer);
}

uint8_t tcaSelect(uint8_t tcaID)
{
  Wire.beginTransmission(TCA);
  Wire.write(1 << tcaID);
  if (Wire.endTransmission())
  {
    log_e("Failed to select i2c port: %d", tcaID);
    return TCA_SELECT_FAILED_TO_SELECT_I2C_PORT;
  }
  return NO_ERR;
}

void setup_wifi()
{
  delay(10);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }

  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void mqttReconnect()
{
  while (!mqttClient.connected())
  {
    Serial.print("Attempting MQTT connection...");
    if (mqttClient.connect("SitLwsLab", mqtt_user, mqtt_password))
    {
      Serial.println("connected");
    }
    else
    {
      Serial.print("failed, rc=");
      Serial.print(mqttClient.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}
