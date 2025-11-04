#pragma once
#include <Arduino.h>
#include "config.h"

bool radio_init(const GlobalConfig *cfg);
void radio_loop();

// send function: returns true on success (nonblocking)
bool radio_send(uint8_t to, const void* data, size_t len, bool requestACK);

// returns pointer to small JSON describing radio config (freq omitted; hardware dependent)
const char* radio_get_config_json();

// Handler called by radio when a new message arrives. Implemented in mqtt module.
void handle_incoming_radio(uint8_t senderId, int16_t rssi, const char *message);