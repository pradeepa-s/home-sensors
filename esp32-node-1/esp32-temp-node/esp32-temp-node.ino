#include <WiFi.h>
#include <PubSubClient.h>
#include <Wire.h>
#include "esp_adc_cal.h"

const uint8_t ath10_address = 0x38;
#define LED       (2)
#define I2C_SDA   (21)
#define I2C_SCL   (22)

bool ledOn = false;
const char* ssid = "P&C_Printer";
const char* password = "0424676158";

const char* mqtt_server = "rpimain.local";
const int mqtt_port = 1883;
const uint32_t id = 2;
const char* cmd_topic = "esp32/2/cmd";
const char* data_topic = "esp32/2/data";
const char* event_topic = "esp32/2/event";

const int BatteryPin = 34;

bool sync_measurements = false;
WiFiClient wifi;
PubSubClient mqtt(wifi);

esp_adc_cal_characteristics_t adc_chars;

void setup_battery_mon() {
  analogReadResolution(12);
  analogSetAttenuation(ADC_11db);
  esp_adc_cal_characterize(ADC_UNIT_1, ADC_ATTEN_DB_11,
                           ADC_WIDTH_BIT_12, 1100, &adc_chars);
}

void setup_wifi() {
  Serial.println("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while(WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    digitalWrite(LED, ledOn ? HIGH : LOW);
    ledOn = !ledOn;
  }

  Serial.println("\nWiFi connected");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}

void setup_mqtt() {
  mqtt.setServer(mqtt_server, mqtt_port);
  mqtt.setCallback(mqtt_rx_callback);
}

void mqtt_rx_callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message received [");
  Serial.print(topic);
  Serial.print("]: ");
  String topic_str(topic);
  if (topic_str.equals(cmd_topic)) {    
    for (int i = 0; i < length; i++) {
      Serial.print("0x");
      Serial.print(payload[i], HEX);
      if (i < length - 1) {
        Serial.print(", ");
      }      
    }

    Serial.println();

    if (length >= 4) {
      const uint16_t cmd_length = (payload[0] << 8) | payload[1];
      const uint16_t cmd = (payload[2] << 8) | payload[3];
      Serial.print("Decoded length: 0x");
      Serial.println(cmd_length, HEX);
      Serial.print("Decoded cmd: 0x");
      Serial.println(cmd, HEX);

      if (cmd == 0x01) {
        sync_measurements = true;
      }
    }
  }
  Serial.println();
}

void setup_tsensor() {
  Wire.begin(I2C_SDA, I2C_SCL);

  // AHT10 needs 20 ms to startup
  delay(20);
  Wire.beginTransmission(ath10_address);
  Wire.write(0xE1);
  Wire.write(0x08);
  Wire.write(0x00);
  Wire.endTransmission();
  delay(20);
}

bool read_tsensor(float& temperature, float& humidity) {
    // Trigger measurement: 0xAC, 0x33, 0x00
    Wire.beginTransmission(ath10_address);
    Wire.write(0xAC);
    Wire.write(0x33);
    Wire.write(0x00);
    Wire.endTransmission();
    delay(80);

    Wire.requestFrom(ath10_address, (uint8_t)6);
    if (Wire.available() != 6) {
      return false;
    }

    uint8_t data[6];
    for (int i = 0; i < 6; ++i) {
      data[i] = Wire.read();
    }

    const uint32_t rawHumidity = ((uint32_t)(data[1]) << 12) | ((uint32_t)(data[2]) << 4) | (data[3] >> 4);
    const uint32_t rawTemp = ((uint32_t)(data[3] & 0x0F) << 16) | ((uint32_t)(data[4]) << 8) | data[5];

    humidity = (rawHumidity * 100.0) / 1048576.0;
    temperature = ((rawTemp * 200.0) / 1048576.0) - 50.0;

    return true;
}

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  pinMode(LED,OUTPUT);
  delay(1000);
  setup_wifi();
  setup_mqtt();
  //setup_tsensor();
  //setup_battery_mon();
}

void reconnect() {
  while (!mqtt.connected()) {
    Serial.print("Attempting MQTT connection...");

    String clientId = "esp32-";
    clientId += String(id, HEX);

    if (mqtt.connect(clientId.c_str())) {
      mqtt.subscribe(cmd_topic);
      Serial.println("Connected");
    }
    else {
      Serial.print("Failed, rc=");
      Serial.print(mqtt.state());
      Serial.println(" try again in 5 seconds.");
      for (int i = 0; i < 10; i++) {
        delay(250);
        digitalWrite(LED, HIGH);
        delay(250);
        digitalWrite(LED, LOW);
      }
    }
  }
}


void loop() {
  digitalWrite(LED, ledOn ? HIGH : LOW);

  reconnect();

  mqtt.loop();

  if (sync_measurements) {
    float t = 0;
    float h = 0;
    if (read_tsensor(t, h)) {
      const uint16_t value = analogRead(BatteryPin);
      const uint32_t voltage = esp_adc_cal_raw_to_voltage(value, &adc_chars);
      const float bat_voltage = (float)voltage * 57.0 / 10.0;

      sync_measurements = false;
      // Serial.print("Temperature: ");
      // Serial.println(t);
      // Serial.print("Humidity: ");
      // Serial.println(h);
      ledOn = !ledOn;
      String msg = "{\"id\":";
      msg += id;
      msg += ",\"temp_v1\":";
      msg += t;
      msg += ",\"humidity_v1\":";
      msg += h;
      msg += ",\"bat_v1\":";
      msg += bat_voltage;
      msg += "}";
      mqtt.publish(data_topic, msg.c_str());
      // Serial.println("Message published!");
    }
  }
}