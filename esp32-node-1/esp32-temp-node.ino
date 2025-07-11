#include <WiFi.h>
#include <PubSubClient.h>

const char* ssid = "P&C_Printer";
const char* password = "0424676158";

const char* mqtt_server = "192.168.20.42";
const int mqtt_port = 1883;
const char* mqtt_topic = "esp32/topic";

WiFiClient wifi;
PubSubClient mqtt(wifi);

void setup_wifi() {
  Serial.println("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while(WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nWiFi connected");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}

void setup_mqtt() {
  mqtt.setServer(mqtt_server, mqtt_port);
  mqtt.setCallback(mqtt_callback);
}

void mqtt_callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message received [");
  Serial.print(topic);
  Serial.print("]: ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();
}

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  delay(1000);
  setup_wifi();
  setup_mqtt();
}

void reconnect() {
  while (!mqtt.connected()) {
    Serial.print("Attempting MQTT connection...");

    String clientId = "ESP32Client-";
    clientId += String(random(0xffff), HEX);

    if (mqtt.connect(clientId.c_str())) {
      Serial.println("Connected");
      mqtt.subscribe(mqtt_topic);
    }
    else {
      Serial.print("Failed, rc=");
      Serial.print(mqtt.state());
      Serial.println(" try again in 5 seconds.");
      delay(5000);
    }
  }
}

void loop() {
  if (!mqtt.connected()) {
    reconnect();
  }
  mqtt.loop();

  static unsigned long lastMessageTime = 0;
  if (millis() - lastMessageTime > 5000) {
    lastMessageTime = millis();
    String msg = "Hello from ESP32";
    mqtt.publish(mqtt_topic, msg.c_str());
    Serial.println("Message published!");
  }
}