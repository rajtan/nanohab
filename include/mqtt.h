#pragma once
#ifndef NANOHAB_MQTT_H
#define NANOHAB_MQTT_H

#include "config.h"
#include <Arduino.h>

// Initialize MQTT subsystem with pointer to global configuration.
// Must be called after WiFi is (or will be) available.
void mqtt_init(const GlobalConfig *cfg);

// Call regularly from main loop to maintain MQTT connection and process messages.
void mqtt_loop(void);

// Publish an incoming radio message to MQTT.
// Topic used: rfmIn/<networkid>/<senderId>
// Returns true if publish was attempted/succeeded (PubSubClient return).
bool mqtt_publish_incoming(uint8_t senderId, const char *payload);

// MQTT callback invoked by the MQTT client when a message arrives.
// Implemented in mqtt.cpp. Topic and payload (raw bytes + length) are provided.
void mqtt_message_callback(char* topic, byte* payload, unsigned int length);

// Handler called by radio module when a new radio message is received.
// Implemented in mqtt.cpp to forward radio -> MQTT (and optional web broadcast).
void handle_incoming_radio(uint8_t senderId, int16_t rssi, const char *message);

#endif // NANOHAB_MQTT_H
