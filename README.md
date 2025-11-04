# nanohab - RFM69 Gateway (refactor)
Inspired from : https://github.com/bbx10/nanohab

This repository contains a refactored, modular version of the RFM69 WiFi gateway from the original nanohab project. The changes split responsibilities into modules (config, radio, mqtt, web) and add an optional boot-time configuration button and PlatformIO support.

Features retained and options
- RFM69 receive + ACK behavior
- MQTT publish (rfmIn/<networkid>/<nodeid>) and subscribe (rfmOut/<networkid>/#) on connect
- EEPROM-backed configuration (networkid, nodeid, encryptkey, mqttbroker, powerlevel). Config is RAM-backed at runtime and persisted explicitly.
- WiFiManager captive-portal and HTTP config endpoints (compiled in when ENABLE_WIFI_MANAGER = 1)
- Optional WebSocket support (ENABLE_WEBSOCKET)
- Optional node stats (ENABLE_NODE_STATS)
- OTA has been removed in this refactor

Boot-time configuration button
- The refactor adds a boot-time hold-to-config feature. If the configured GPIO is held for the configured time during boot, the WiFiManager config portal will be started.
- Configure these defines in include/config.h before building:
  - CONFIG_GPIO_HOLD_MS - milliseconds to hold the button (default 3000)
  - CONFIG_GPIO_NUM - GPIO pin number to sample at boot (default 0)
  - CONFIG_GPIO_STATE - active state (0 = LOW active, default; 1 = HIGH active)

Usage
- Build with PlatformIO (recommended) or Arduino IDE.
- Edit include/config.h to set feature flags and defaults as desired.
- If using PlatformIO, the provided platformio.ini targets the nodemcuv2 board (ESP8266).

Building with PlatformIO
1. Install PlatformIO for VSCode or use the CLI.
2. From repository root run:
   pio run -e nodemcuv2
3. Upload with:
   pio run -e nodemcuv2 -t upload

WiFiManager usage
- The captive portal is available on-demand. Either:
  - Press and hold the configured button during boot to start the portal (if compiled-in), or
  - Visit the device HTTP page "/startAP" to start the portal (if web module compiled-in)

Next work you may want
- Implement MQTT -> radio forwarding for `rfmOut/<netid>/<nodeid>` messages
- Add a non-blocking long-press detection that runs from loop() if you prefer not to block at boot
- Implement node-stats behind ENABLE_NODE_STATS

License
- Original project is GPLv3; retain license notices from original source when merging or redistributing.
