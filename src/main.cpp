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

  // Initialize web first so wifi_start_config_portal() has cfg_ptr available
  web_init(&gConfig);

#if ENABLE_WIFI_MANAGER
  // Boot-time config button: if held for CONFIG_GPIO_HOLD_MS at boot, start config portal
  pinMode(CONFIG_GPIO_NUM, (CONFIG_GPIO_STATE)==0 ? INPUT_PULLUP : INPUT);
  int val = digitalRead(CONFIG_GPIO_NUM);
  bool active = (val == CONFIG_GPIO_STATE);
  if (active) {
    unsigned long start = millis();
    bool stillActive = true;
    while ((millis() - start) < CONFIG_GPIO_HOLD_MS) {
      int v = digitalRead(CONFIG_GPIO_NUM);
      if (v != CONFIG_GPIO_STATE) { stillActive = false; break; }
      delay(10);
    }
    if (stillActive) {
      Serial.println("Config button held at boot - starting WiFiManager config portal");
      // This blocking call will start the AP and captive portal. It will restart the device after save.
      wifi_start_config_portal();
    }
  }
#endif

  // Initialize other modules
  radio_init(&gConfig);    // initializes radio and builds radio config JSON
  mqtt_init(&gConfig);     // requires WiFi; reconnect is handled inside mqtt_loop
  // web_init already called above to make wifi_start_config_portal available
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