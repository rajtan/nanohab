#include "mqtt.h"
#include "radio.h"
#include "web.h"
#include <PubSubClient.h>
#include <ESP8266WiFi.h>

static WiFiClient wifiClient;
static PubSubClient client(wifiClient);
static const GlobalConfig *cfg_ptr = nullptr;

void mqtt_message_callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("MQTT in: ");
  Serial.println(topic);
  // For now we just log. MQTT -> radio forwarding can be implemented later.
}

void mqtt_init(const GlobalConfig *cfg) {
  cfg_ptr = cfg;
  client.setServer(cfg->mqttbroker, 1883);
  client.setCallback(mqtt_message_callback);
}

static void mqtt_subscribe_on_connect() {
  char subtopic[48];
  snprintf(subtopic, sizeof(subtopic), "rfmOut/%d/#", cfg_ptr->networkid);
  client.subscribe(subtopic);
  Serial.printf("Subscribed to %s\n", subtopic);
}

void mqtt_loop() {
  if (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    if (client.connect(WiFi.hostname().c_str())) {
      Serial.println("connected");
      client.publish("rfmIn", "Connect");
      mqtt_subscribe_on_connect();
    } else {
      Serial.print("failed, rc=");
      Serial.println(client.state());
      delay(2000);
      return;
    }
  }
  client.loop();
}

bool mqtt_publish_incoming(uint8_t senderId, const char *payload) {
  char topic[48];
  snprintf(topic, sizeof(topic), "rfmIn/%d/%d", cfg_ptr->networkid, senderId);
  bool ok = client.publish(topic, payload);
  if (!ok) Serial.println("*** mqtt publish failed ***");
  return ok;
}

void handle_incoming_radio(uint8_t senderId, int16_t rssi, const char *message) {
  // Called from radio_loop when a message arrives.
  // Publish to MQTT
  mqtt_publish_incoming(senderId, message);
  // Broadcast to websocket clients if enabled
  web_broadcast(message);
}