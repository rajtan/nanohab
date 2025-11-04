#include "config.h"
#include "platform_pins.h"
#include "radio.h"
#include "mqtt.h"
#include "web.h"
#include <Arduino.h>
#include <ESP8266WiFi.h>

void apply_saved_network_config() {
  if (!gConfig.use_dhcp) {
    IPAddress ip, gw, sn, d1, d2;
    if (ip.fromString(gConfig.ip) && gw.fromString(gConfig.gateway) && sn.fromString(gConfig.netmask)) {
      if (strlen(gConfig.dns1) && d1.fromString(gConfig.dns1)) {
        if (strlen(gConfig.dns2) && d2.fromString(gConfig.dns2)) {
#if defined(ESP8266)
          WiFi.config(ip, gw, sn, d1, d2);
#else
          WiFi.config(ip, gw, sn);
#endif
        } else {
#if defined(ESP8266)
          WiFi.config(ip, gw, sn, d1);
#else
          WiFi.config(ip, gw, sn);
#endif
        }
      } else {
        WiFi.config(ip, gw, sn);
      }
      Serial.println("Applied static IP configuration");
    } else {
      Serial.println("Invalid saved static IP configuration; falling back to DHCP");
    }
  }
}

void setup() {
  Serial.begin(SERIAL_BAUD);
  Serial.println("\nRFM69 WiFi Gateway - rt-refact");

  if (!config_init()) {
    Serial.println("Config init failed, continuing with defaults");
  }

  web_init(&gConfig);

#if ENABLE_WIFI_MANAGER
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
      wifi_start_config_portal();
    }
  }
#endif

  // Apply saved network configuration (static IP) before attempting connection
  apply_saved_network_config();

  // Try to connect using stored WiFi credentials (SDK stores last credentials)
  WiFi.begin();
  Serial.print("Attempting WiFi connect...");
  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED && (millis() - start) < 10000) {
    delay(200);
    Serial.print('.');
  }
  Serial.println();
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("WiFi connected");
  } else {
    Serial.println("WiFi not connected (will continue, config portal available on request)");
  }

  radio_init(&gConfig);
  mqtt_init(&gConfig);
}

void loop() {
  radio_loop();
  mqtt_loop();
  web_loop();
}