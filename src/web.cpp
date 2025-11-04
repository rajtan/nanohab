#include "web.h"
#include "config.h"
#include "radio.h"
#include <ESP8266WebServer.h>
#include <ESP8266WiFi.h>

static ESP8266WebServer server(80);
#if ENABLE_WEBSOCKET
#include <WebSocketsServer.h>
static WebSocketsServer webSocket(81);
#endif

static const GlobalConfig *cfg_ptr = nullptr;

#if ENABLE_WIFI_MANAGER
#include <WiFiManager.h>
static WiFiManagerParameter *wm_param_mqtt = nullptr;
static WiFiManagerParameter *wm_param_enc = nullptr;
static WiFiManagerParameter *wm_param_netid = nullptr;
static WiFiManagerParameter *wm_param_nodeid = nullptr;
static WiFiManagerParameter *wm_param_power = nullptr;
static bool shouldSaveConfig = false;

static void saveConfigCallback () {
  Serial.println("Should save config (WiFiManager)");
  shouldSaveConfig = true;
}

static void configModeCallback (WiFiManager *myWiFiManager) {
  Serial.println("Entered WiFiManager config mode");
  Serial.println(WiFi.softAPIP());
  Serial.println(myWiFiManager->getConfigPortalSSID());
}

void wifi_start_config_portal() {
  WiFiManager wm;
  wm.setAPCallback(configModeCallback);
  wm.setSaveConfigCallback(saveConfigCallback);

  // create parameters
  char tmp[32];
  snprintf(tmp, sizeof(tmp), "%s", cfg_ptr->mqttbroker);
  wm_param_mqtt = new WiFiManagerParameter("mqtt", "MQTT broker", tmp, sizeof(tmp));

  snprintf(tmp, sizeof(tmp), "%s", cfg_ptr->encryptkey);
  wm_param_enc = new WiFiManagerParameter("enc", "Encrypt key", tmp, sizeof(tmp));

  snprintf(tmp, sizeof(tmp), "%u", cfg_ptr->networkid);
  wm_param_netid = new WiFiManagerParameter("netid", "Network ID", tmp, 4);

  snprintf(tmp, sizeof(tmp), "%u", cfg_ptr->nodeid);
  wm_param_nodeid = new WiFiManagerParameter("nodeid", "Node ID", tmp, 4);

  snprintf(tmp, sizeof(tmp), "%u", cfg_ptr->powerlevel & 0x1F);
  wm_param_power = new WiFiManagerParameter("power", "Power Level", tmp, 4);

  wm.addParameter(wm_param_mqtt);
  wm.addParameter(wm_param_enc);
  wm.addParameter(wm_param_netid);
  wm.addParameter(wm_param_nodeid);
  wm.addParameter(wm_param_power);

  // blocking call: starts AP and portal until credentials provided
  if (!wm.autoConnect(RFMAPNAME)) {
    Serial.println("Failed to connect or portal aborted; restarting...");
    delay(2000);
    ESP.restart();
    return;
  }

  // if user changed parameters, save to EEPROM via config_save()
  if (shouldSaveConfig) {
    // read new parameters back into gConfig
    strncpy(((GlobalConfig*)cfg_ptr)->mqttbroker, wm_param_mqtt->getValue(), sizeof(((GlobalConfig*)cfg_ptr)->mqttbroker)-1);
    strncpy(((GlobalConfig*)cfg_ptr)->encryptkey, wm_param_enc->getValue(), sizeof(((GlobalConfig*)cfg_ptr)->encryptkey)-1);
    ((GlobalConfig*)cfg_ptr)->networkid = (uint8_t)atoi(wm_param_netid->getValue());
    ((GlobalConfig*)cfg_ptr)->nodeid = (uint8_t)atoi(wm_param_nodeid->getValue());
    ((GlobalConfig*)cfg_ptr)->powerlevel = (uint8_t)atoi(wm_param_power->getValue()) & 0x1F;
    // persist
    config_save();
    delay(500);
    ESP.restart();
  }
}
#endif // ENABLE_WIFI_MANAGER

void handleRoot() {
  static const char INDEX_PAGE[] PROGMEM = R"rawliteral(
<!DOCTYPE html><html><head><meta name=\"viewport\" content=\"width=device-width,initial-scale=1\"> 
<title>RFM69 Gateway</title>
</head><body>
<h3>RFM69 Gateway</h3>
<div id=\"status\">Status</div>
<p><a href=\"/config\">Configure</a></p>
</body></html>
)rawliteral";
  server.send_P(200, "text/html", INDEX_PAGE);
}

// Minimal config page (GET) and handler (POST) -- only enabled when compile-time directive present.
#if ENABLE_WIFI_MANAGER
void handleConfigGet() {
  String page = "<html><body><h3>Gateway Config</h3>";
  page += "<form method='POST' action='/config'>";
  page += "MQTT broker: <input name='mqttbroker' value='" + String(cfg_ptr->mqttbroker) + "'><br>";
  page += "Encrypt key: <input name='encryptkey' value='" + String(cfg_ptr->encryptkey) + "'><br>";
  page += "Network ID: <input name='networkid' value='" + String(cfg_ptr->networkid) + "'><br>";
  page += "Node ID: <input name='nodeid' value='" + String(cfg_ptr->nodeid) + "'><br>";
  page += "Power Level: <input name='powerlevel' value='" + String(cfg_ptr->powerlevel & 0x1F) + "'><br>";
  page += "<input type='submit' value='Save'>";
  page += "</form>";
  page += "<p><a href='/startAP'>Start WiFi Config Portal</a> (starts AP & portal)</p>";
  server.send(200, "text/html", page);
}

void handleConfigPost() {
  if (server.hasArg("mqttbroker")) {
    strncpy(((GlobalConfig*)cfg_ptr)->mqttbroker, server.arg("mqttbroker").c_str(), sizeof(((GlobalConfig*)cfg_ptr)->mqttbroker)-1);
  }
  if (server.hasArg("encryptkey")) {
    strncpy(((GlobalConfig*)cfg_ptr)->encryptkey, server.arg("encryptkey").c_str(), sizeof(((GlobalConfig*)cfg_ptr)->encryptkey)-1);
  }
  if (server.hasArg("networkid")) {
    ((GlobalConfig*)cfg_ptr)->networkid = (uint8_t)atoi(server.arg("networkid").c_str());
  }
  if (server.hasArg("nodeid")) {
    ((GlobalConfig*)cfg_ptr)->nodeid = (uint8_t)atoi(server.arg("nodeid").c_str());
  }
  if (server.hasArg("powerlevel")) {
    ((GlobalConfig*)cfg_ptr)->powerlevel = (uint8_t)atoi(server.arg("powerlevel").c_str()) & 0x1F;
  }
  config_save();
  server.send(200, "text/html", "<html><body>Saved. Rebooting...<script>setTimeout(function(){location='/';},2000);</script></body></html>");
  delay(500);
  ESP.restart();
}

void handleStartAP() {
  // Start the WiFiManager portal (blocking). Better to call wifi_start_config_portal from loop or button handler.
  server.send(200, "text/plain", "Starting config portal...");
  delay(200);
  wifi_start_config_portal();
}
#endif // ENABLE_WIFI_MANAGER

void web_init(const GlobalConfig *cfg) {
  cfg_ptr = cfg;
  server.on("/", HTTP_GET, handleRoot);
#if ENABLE_WIFI_MANAGER
  server.on("/config", HTTP_GET, handleConfigGet);
  server.on("/config", HTTP_POST, handleConfigPost);
  server.on("/startAP", HTTP_GET, handleStartAP);
#endif
  server.begin();

#if ENABLE_WEBSOCKET
  webSocket.begin();
  webSocket.onEvent([](uint8_t num, WStype_t type, uint8_t * payload, size_t length){
    if (type == WStype_CONNECTED) {
      webSocket.sendTXT(num, radio_get_config_json());
    }
  });
#endif
}

void web_loop() {
  server.handleClient();
#if ENABLE_WEBSOCKET
  webSocket.loop();
#endif
}

void web_broadcast(const char *msg) {
#if ENABLE_WEBSOCKET
  webSocket.broadcastTXT(msg);
#else
  (void)msg;
#endif
}