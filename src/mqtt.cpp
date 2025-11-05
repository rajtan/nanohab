#include "mqtt.h"
#include "radio.h"
#include "web.h"
#include "config.h"
#include <PubSubClient.h>
#include <ESP8266WiFi.h>
#include <Arduino.h>

static WiFiClient wifiClient;
static PubSubClient client(wifiClient);
static const GlobalConfig *cfg_ptr = nullptr;

// Helper to safely copy payload into a null-terminated buffer
static void payload_to_cstr(char *buf, size_t bufLen, const byte *payload, unsigned int length) {
  size_t copyLen = (length < (unsigned int)(bufLen - 1)) ? length : (unsigned int)(bufLen - 1);
  if (copyLen > 0) memcpy(buf, payload, copyLen);
  buf[copyLen] = '\0';
}

// MQTT inbound callback
// Expected inbound topics: rfmOut/<netid>/<nodeid> (additional path components ignored)
// The payload is forwarded to the RFM69 radio as-is.
void mqtt_message_callback(char* topic, byte* payload, unsigned int length) {
  Serial.printf("MQTT in: %s (len=%u)\n", topic, length);

  // copy payload to safe buffer
  char msg[512];
  payload_to_cstr(msg, sizeof(msg), payload, length);

  // Parse topic tokens
  // We expect topics like: rfmOut/<netid>/<nodeid>
  char topic_copy[128];
  strncpy(topic_copy, topic, sizeof(topic_copy)-1);
  topic_copy[sizeof(topic_copy)-1] = '\0';

  char *saveptr = NULL;
  char *token = strtok_r(topic_copy, "/", &saveptr);
  if (!token) return;

  if (strcmp(token, "rfmOut") != 0) {
    // Not an rfmOut topic we handle
    Serial.println("MQTT topic not rfmOut, ignoring");
    return;
  }

  token = strtok_r(NULL, "/", &saveptr); // network id
  if (!token) {
    Serial.println("MQTT topic missing network id");
    return;
  }
  int netid = atoi(token);

  token = strtok_r(NULL, "/", &saveptr); // node id
  if (!token) {
    Serial.println("MQTT topic missing node id");
    return;
  }
  int nodeid = atoi(token);

  if ((uint8_t)netid != cfg_ptr->networkid) {
    Serial.printf("MQTT netid %d != local netid %d, ignoring\n", netid, cfg_ptr->networkid);
    return;
  }

  if (nodeid < 1 || nodeid > 255) {
    Serial.printf("Invalid destination node id %d\n", nodeid);
    return;
  }

  // Forward to radio (no ACK requested)
  bool ok = radio_send((uint8_t)nodeid, msg, strlen(msg), false);
  if (ok) {
    Serial.printf("Forwarded MQTT -> radio to node %d\n", nodeid);
  } else {
    Serial.printf("Failed to forward MQTT -> radio to node %d\n", nodeid);
  }
}

void mqtt_init(const GlobalConfig *cfg) {
  cfg_ptr = cfg;
  client.setServer(cfg->mqttbroker, cfg->mqtt_port);
  client.setCallback(mqtt_message_callback);
}

static void mqtt_subscribe_on_connect() {
  char subtopic[48];
  snprintf(subtopic, sizeof(subtopic), "rfmOut/%d/#", cfg_ptr->networkid);
  if (client.subscribe(subtopic)) {
    Serial.printf("Subscribed to %s\n", subtopic);
  } else {
    Serial.printf("Failed to subscribe to %s\n", subtopic);
  }
}

static bool mqtt_do_connect() {
  char clientId[48];
  // Build client id from hostname; add random suffix to avoid collisions
  snprintf(clientId, sizeof(clientId), "%s-%lu", WiFi.hostname().c_str(), (unsigned long)random(0xffff));

#if ENABLE_MQTT_CONFIG
  // If user/pass configured, use them
  if (strlen(((GlobalConfig*)cfg_ptr)->mqtt_user) > 0) {
    Serial.printf("Connecting to MQTT %s:%u as user '%s'\n", cfg_ptr->mqttbroker, cfg_ptr->mqtt_port, ((GlobalConfig*)cfg_ptr)->mqtt_user);
    return client.connect(clientId, ((GlobalConfig*)cfg_ptr)->mqtt_user, ((GlobalConfig*)cfg_ptr)->mqtt_pass);
  }
#endif

  Serial.printf("Connecting to MQTT %s:%u (no auth)\n", cfg_ptr->mqttbroker, cfg_ptr->mqtt_port);
  return client.connect(clientId);
}

void mqtt_loop() {
  if (WiFi.status() != WL_CONNECTED) {
    // No WiFi; nothing to do
    return;
  }

  if (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    if (mqtt_do_connect()) {
      Serial.println("connected");
      // announce and subscribe
      client.publish("rfmIn", "Connect");
      mqtt_subscribe_on_connect();
    } else {
      Serial.print("failed, rc=");
      Serial.println(client.state());
      // back off briefly
      delay(2000);
      return;
    }
  }

  client.loop();
}

// Publish incoming radio message to MQTT topic: rfmIn/<netid>/<senderId>
bool mqtt_publish_incoming(uint8_t senderId, const char *payload) {
  if (!client.connected()) {
    Serial.println("MQTT publish skipped: client not connected");
    return false;
  }
  char topic[48];
  snprintf(topic, sizeof(topic), "rfmIn/%d/%d", cfg_ptr->networkid, senderId);
  bool ok = client.publish(topic, payload);
  if (!ok) {
    Serial.println("*** mqtt publish failed ***");
  } else {
    Serial.printf("Published to %s\n", topic);
  }
  return ok;
}
