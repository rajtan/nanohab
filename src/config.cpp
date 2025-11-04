#include "config.h"

GlobalConfig gConfig;

static void apply_defaults(GlobalConfig *c) {
  memset(c, 0, sizeof(*c));
  strncpy(c->mqttbroker, DEFAULT_MQTT_BROKER, sizeof(c->mqttbroker)-1);
  strncpy(c->encryptkey, DEFAULT_ENCRYPTKEY, sizeof(c->encryptkey)-1);
  c->networkid = DEFAULT_NETWORKID;
  c->nodeid = DEFAULT_NODEID;
  c->powerlevel = DEFAULT_POWERLEVEL;
  c->checksum = config_calc_checksum(c);
}

uint32_t config_calc_checksum(const GlobalConfig *c) {
  const uint8_t *p = (const uint8_t *)c;
  uint32_t sum = 0;
  // skip checksum field itself (first 4 bytes)
  p += sizeof(c->checksum);
  for (size_t i = 0; i < (sizeof(*c) - sizeof(c->checksum)); i++) sum += p[i];
  return sum;
}

bool config_init() {
  EEPROM.begin(EEPROM_SIZE);
  // read into RAM copy
  EEPROM.get(0, gConfig);
  uint32_t calc = config_calc_checksum(&gConfig);
  if (gConfig.checksum != calc || gConfig.checksum == 0xFFFFFFFF) {
    Serial.println("Config checksum mismatch or uninitialized; applying defaults");
    apply_defaults(&gConfig);
    config_save();
    return false;
  }
  return true;
}

bool config_save() {
  gConfig.checksum = config_calc_checksum(&gConfig);
  EEPROM.put(0, gConfig);
  return EEPROM.commit();
}