#include "radio.h"
#include <RFM69.h>
#include <SPI.h>

static RFM69 radio;
static char radioConfig[128];
static const GlobalConfig *cfg_ptr = nullptr;

bool radio_init(const GlobalConfig *cfg) {
  cfg_ptr = cfg;
  // initialize & reset radio hardware
  radio = RFM69(RFM69_CS, RFM69_IRQ, true, digitalPinToInterrupt(RFM69_IRQ));
  pinMode(RFM69_RST, OUTPUT);
  digitalWrite(RFM69_RST, HIGH);
  delay(100);
  digitalWrite(RFM69_RST, LOW);
  delay(100);

  radio.initialize(FREQUENCY /* from RFM69 lib */, cfg->nodeid, cfg->networkid);
  if ((cfg->powerlevel & 0x80) != 0) radio.setHighPower();
  radio.setPowerLevel(cfg->powerlevel & 0x1F);
  if (cfg->encryptkey[0] != '\0') radio.encrypt(cfg->encryptkey);

  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  // Build a small JSON config (frequency omitted; hardware-specific)
  snprintf(radioConfig, sizeof(radioConfig),
           "{\"msgType\":\"config\",\"netid\":%d,\"nodeid\":%d,\"power\":%d}",
           cfg->networkid, cfg->nodeid, cfg->powerlevel & 0x1F);

  return true;
}

const char* radio_get_config_json() {
  return radioConfig;
}

void radio_loop() {
  if (radio.receiveDone()) {
    uint8_t sender = radio.SENDERID;
    int16_t rssi = radio.RSSI;
    char data[RF69_MAX_DATA_LEN + 1];
    memcpy(data, (void*)radio.DATA, radio.DATALEN);
    data[radio.DATALEN] = '\0';
    if (radio.ACKRequested()) {
      radio.sendACK();
    }
    radio.receiveDone(); // ensure RX mode
    // forward to handler (publishes to MQTT, optional websocket)
    handle_incoming_radio(sender, rssi, data);
  } else {
    radio.receiveDone(); // keep in RX
  }
}

bool radio_send(uint8_t to, const void* data, size_t len, bool requestACK) {
  if (len > RF69_MAX_DATA_LEN) return false;
  return radio.sendWithRetry(to, (const uint8_t*)data, len);
}