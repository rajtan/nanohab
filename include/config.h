#pragma once
#include <Arduino.h>
#include <EEPROM.h>

// Feature toggles - adjust as needed
#ifndef ENABLE_WEBSOCKET
#define ENABLE_WEBSOCKET    0
#endif
#ifndef ENABLE_WIFI_MANAGER
#define ENABLE_WIFI_MANAGER 1
#endif
#ifndef ENABLE_NODE_STATS
#define ENABLE_NODE_STATS   0
#endif
#ifndef ENABLE_MQTT_CONFIG
#define ENABLE_MQTT_CONFIG  1
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
#define DEFAULT_MQTT_PORT   1883
#define DEFAULT_MQTT_USER   ""
#define DEFAULT_MQTT_PASS   ""
#define DEFAULT_ENCRYPTKEY  "sampleEncryptKey"

#ifndef RFMAPNAME
#define RFMAPNAME "RFM69-AP"
#endif

struct GlobalConfig {
  uint32_t checksum;
  // Network configuration
  bool     use_dhcp;
  char     ip[16];
  char     netmask[16];
  char     gateway[16];
  char     dns1[16];
  char     dns2[16];

  // MQTT configuration
  char     mqttbroker[32];
  uint16_t mqtt_port;
#if ENABLE_MQTT_CONFIG
  char     mqtt_user[32];
  char     mqtt_pass[32];
#endif

  // Radio configuration
  char     encryptkey[17]; // 16 + NUL
  uint8_t  networkid;
  uint8_t  nodeid;
  uint8_t  powerlevel;
};

extern GlobalConfig gConfig;

bool config_init();
bool config_save();
uint32_t config_calc_checksum(const GlobalConfig *c);