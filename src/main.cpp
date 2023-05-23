#include <Arduino.h>
#include <DFRobot_DHT20.h>
#include <Wire.h>
#define ARDUINOJSON_USE_DOUBLE 0
#include <ArduinoJson.h>

/*!
 * @brief Construct the function
 * @param pWire IC bus pointer object and construction device, can both pass or not pass parameters, Wire in default.
 * @param address Chip IIC address, 0x38 in default.
 */

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

DFRobot_DHT20 dht20[NUM_SENSORS];

// funktion protoypen
uint8_t tcaSelect(uint8_t tcaID);
float getTemperature(uint8_t tcaID);
int getHumidity(uint8_t tcaID);
void publishMeasurements();

void setup()
{
  Wire.begin(I2C_SDA, I2C_SCL);

  Serial.begin(115200);

  tcaSelect(DHT20_1_MULTIPLEXER_PORT);
  if (dht20[0].begin())
  {
    Serial.println("Failed to init DHT20[0].\nESP will reboot in 30 seconds.");
    delay(30000);
    ESP.restart();
  }
  tcaSelect(DHT20_2_MULTIPLEXER_PORT);
  if (dht20[1].begin())
  {
    Serial.println("Failed to init DHT20[0].\nESP will reboot in 30 seconds.");
    delay(30000);
    ESP.restart();
  }
  tcaSelect(DHT20_3_MULTIPLEXER_PORT);
  if (dht20[2].begin())
  {
    Serial.println("Failed to init DHT20[0].\nESP will reboot in 30 seconds.");
    delay(30000);
    ESP.restart();
  }
}

void loop()
{
  publishMeasurements();
  delay(1000);
}

float getTemperature(uint8_t tcaID)
{
  tcaSelect(tcaID);
  float value = dht20.getTemperature();
  return ((int)(value * 100 + 0.5) / 100.0);
}

int getHumidity(uint8_t tcaID)
{
  tcaSelect(tcaID);
  float value = dht20.getHumidity();
  return (int)(value * 100.0 + 0.5);
}

void publishMeasurements()
{

  StaticJsonDocument<500> doc;

  JsonArray measurement = doc.createNestedArray("measurement");

  float temperature = getTemperature(DHT20_1_MULTIPLEXER_PORT);
  int humidity = getHumidity(DHT20_1_MULTIPLEXER_PORT);
  JsonObject lab001 = measurement.createNestedObject();
  lab001["sensor"] = "lab001";
  lab001["temperature"] = temperature;
  lab001["humidity"] = humidity;

  temperature = getTemperature(DHT20_2_MULTIPLEXER_PORT);
  humidity = getHumidity(DHT20_2_MULTIPLEXER_PORT);
  JsonObject lab002 = measurement.createNestedObject();
  lab002["sensor"] = "lab002";
  lab002["temperature"] = temperature;
  lab002["humidity"] = humidity;

  // temperature = getTemperature(DHT20_3_MULTIPLEXER_PORT);
  // humidity = getHumidity(DHT20_3_MULTIPLEXER_PORT);
  // JsonObject lab003 = measurement.createNestedObject();
  // lab003["sensor"] = "lab003";
  // lab003["temperature"] = 99.99;
  // lab003["humidity"] = 100;
  serializeJson(doc, Serial);
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
