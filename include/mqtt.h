#pragma once
#include "config.h"
#include <Arduino.h>

void mqtt_init(const GlobalConfig *cfg);
void mqtt_loop();
bool mqtt_publish_incoming(uint8_t senderId, const char *payload);

// Called when inbound MQTT message arrives on subscribed topic; forward to radio if desired
void mqtt_message_callback(char* topic, byte* payload, unsigned int length);

// The radio message handler will call handle_incoming_radio which is implemented below:
void handle_incoming_radio(uint8_t senderId, int16_t rssi, const char *message);