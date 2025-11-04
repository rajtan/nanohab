#include "config.h"
#include "platform_pins.h"
#include "radio.h"
#include "mqtt.h"
#include "web.h"
#include <Arduino.h>

void setup() {
  Serial.begin(SERIAL_BAUD);
  Serial.println("\nRFM69 WiFi Gateway - rt-refact");

  if (!config_init()) {
    Serial.println("Config init failed, continuing with defaults");
  }

  // Initialize modules. WiFiManager captive portal is available on-demand.
  radio_init(&gConfig);    // initializes radio and builds radio config JSON
  mqtt_init(&gConfig);     // requires WiFi; reconnect is handled inside mqtt_loop
  web_init(&gConfig);      // registers HTTP routes; WiFiManager config portal support is compiled in only if enabled
}

void loop() {
  radio_loop();   // handles receiveDone(), ACKs, and calls a callback for messages
  mqtt_loop();
#if defined(ENABLE_WEBSOCKET) && (ENABLE_WEBSOCKET != 0)
  web_loop();
#else
  web_loop(); // still call web_loop to handle config endpoints when enabled
#endif
}