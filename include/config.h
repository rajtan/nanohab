#pragma once
#include <Arduino.h>
#include <EEPROM.h>

// Feature toggles - adjust as needed
// When ENABLE_WIFI_MANAGER == 1, WiFiManager + web parameter editing endpoints are compiled in.
// Websocket and node-stats are optional.
#ifndef ENABLE_WEBSOCKET
#define ENABLE_WEBSOCKET    0  // set to 1 to include websocket server
#endif
#ifndef ENABLE_WIFI_MANAGER
#define ENABLE_WIFI_MANAGER 1  // set to 1 to include WiFiManager (captive portal + config endpoints)
#endif
#ifndef ENABLE_NODE_STATS
#define ENABLE_NODE_STATS   0  // per-node advanced stats
#endif

// Boot-time configuration button settings (optional)
#ifndef CONFIG_GPIO_HOLD_MS
#define CONFIG_GPIO_HOLD_MS 3000 // milliseconds to hold button at boot to start config portal
#endif
#ifndef CONFIG_GPIO_NUM
#define CONFIG_GPIO_NUM 0        // GPIO pin number for config button (board-specific)
#endif
#ifndef CONFIG_GPIO_STATE
#define CONFIG_GPIO_STATE 0      // Active state: 0 = LOW active (use INPUT_PULLUP), 1 = HIGH active
#endif

#define SERIAL_BAUD 115200
#define EEPROM_SIZE 4096

// Default values
#define DEFAULT_NETWORKID   200
#define DEFAULT_NODEID       1
#define DEFAULT_POWERLEVEL  31
#define DEFAULT_MQTT_BROKER "raspi2"
#define DEFAULT_ENCRYPTKEY  "sampleEncryptKey"

// RFM AP name can be compile-time define as you requested
#ifndef RFMAPNAME
#define RFMAPNAME "RFM69-AP"
#endif

struct GlobalConfig {
  uint32_t checksum;
  char mqttbroker[32];
  char encryptkey[17]; // 16 + NUL
  uint8_t networkid;
  uint8_t nodeid;
  uint8_t powerlevel;
};

extern GlobalConfig gConfig;

// config API
bool config_init(); // loads EEPROM into gConfig (returns true if checksum OK)
bool config_save(); // saves gConfig into EEPROM (updates checksum)
uint32_t config_calc_checksum(const GlobalConfig *c);