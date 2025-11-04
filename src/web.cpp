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
static WiFiManagerParameter *wm_param_user = nullptr;
static WiFiManagerParameter *wm_param_pass = nullptr;
static WiFiManagerParameter *wm_param_ip = nullptr;
static WiFiManagerParameter *wm_param_gw = nullptr;
static WiFiManagerParameter *wm_param_sn = nullptr;
static WiFiManagerParameter *wm_param_dns1 = nullptr;
static WiFiManagerParameter *wm_param_dns2 = nullptr;

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

// Blocking call that starts the captive portal and saves params via config_save()
void wifi_start_config_portal() {
  WiFiManager wm;
  wm.setAPCallback(configModeCallback);
  wm.setSaveConfigCallback(saveConfigCallback);

  // create parameters for WiFiManager portal
  char tmp[48];

  // MQTT broker
  snprintf(tmp, sizeof(tmp), "%s", cfg_ptr->mqttbroker);
  wm_param_mqtt = new WiFiManagerParameter("mqtt", "MQTT broker", tmp, sizeof(tmp));
  wm.addParameter(wm_param_mqtt);

#if ENABLE_MQTT_CONFIG
  // MQTT credentials
  snprintf(tmp, sizeof(tmp), "%s", ((GlobalConfig*)cfg_ptr)->mqtt_user);
  wm_param_user = new WiFiManagerParameter("mquser", "MQTT user", tmp, sizeof(tmp));
  snprintf(tmp, sizeof(tmp), "%s", ((GlobalConfig*)cfg_ptr)->mqtt_pass);
  wm_param_pass = new WiFiManagerParameter("mqpass", "MQTT pass", tmp, sizeof(tmp));
  wm.addParameter(wm_param_user);
  wm.addParameter(wm_param_pass);
#endif

  // Static IP params
  snprintf(tmp, sizeof(tmp), "%s", cfg_ptr->ip);
  wm_param_ip = new WiFiManagerParameter("ip", "Static IP (leave blank for DHCP)", tmp, sizeof(tmp));
  snprintf(tmp, sizeof(tmp), "%s", cfg_ptr->gateway);
  wm_param_gw = new WiFiManagerParameter("gw", "Gateway", tmp, sizeof(tmp));
  snprintf(tmp, sizeof(tmp), "%s", cfg_ptr->netmask);
  wm_param_sn = new WiFiManagerParameter("sn", "Netmask", tmp, sizeof(tmp));
  snprintf(tmp, sizeof(tmp), "%s", cfg_ptr->dns1);
  wm_param_dns1 = new WiFiManagerParameter("dns1", "DNS1", tmp, sizeof(tmp));
  snprintf(tmp, sizeof(tmp), "%s", cfg_ptr->dns2);
  wm_param_dns2 = new WiFiManagerParameter("dns2", "DNS2", tmp, sizeof(tmp));
  wm.addParameter(wm_param_ip);
  wm.addParameter(wm_param_gw);
  wm.addParameter(wm_param_sn);
  wm.addParameter(wm_param_dns1);
  wm.addParameter(wm_param_dns2);

  // blocking call: starts AP and portal until credentials provided
  if (!wm.autoConnect(RFMAPNAME)) {
    Serial.println("Failed to connect or portal aborted; restarting...");
    delay(2000);
    ESP.restart();
    return;
  }

  // if user changed parameters, save to EEPROM via config_save()
  if (shouldSaveConfig) {
#if ENABLE_MQTT_CONFIG
    strncpy(((GlobalConfig*)cfg_ptr)->mqtt_user, wm_param_user->getValue(), sizeof(((GlobalConfig*)cfg_ptr)->mqtt_user)-1);
    strncpy(((GlobalConfig*)cfg_ptr)->mqtt_pass, wm_param_pass->getValue(), sizeof(((GlobalConfig*)cfg_ptr)->mqtt_pass)-1);
#endif
    strncpy(((GlobalConfig*)cfg_ptr)->mqttbroker, wm_param_mqtt->getValue(), sizeof(((GlobalConfig*)cfg_ptr)->mqttbroker)-1);
    strncpy(((GlobalConfig*)cfg_ptr)->ip, wm_param_ip->getValue(), sizeof(((GlobalConfig*)cfg_ptr)->ip)-1);
    strncpy(((GlobalConfig*)cfg_ptr)->gateway, wm_param_gw->getValue(), sizeof(((GlobalConfig*)cfg_ptr)->gateway)-1);
    strncpy(((GlobalConfig*)cfg_ptr)->netmask, wm_param_sn->getValue(), sizeof(((GlobalConfig*)cfg_ptr)->netmask)-1);
    strncpy(((GlobalConfig*)cfg_ptr)->dns1, wm_param_dns1->getValue(), sizeof(((GlobalConfig*)cfg_ptr)->dns1)-1);
    strncpy(((GlobalConfig*)cfg_ptr)->dns2, wm_param_dns2->getValue(), sizeof(((GlobalConfig*)cfg_ptr)->dns2)-1);

    // Determine use_dhcp: if ip field empty -> DHCP
    ((GlobalConfig*)cfg_ptr)->use_dhcp = (strlen(wm_param_ip->getValue()) == 0);

    // persist
    config_save();
    delay(500);
    ESP.restart();
  }
}
#endif // ENABLE_WIFI_MANAGER

// Root page
void handleRoot() {
  static const char INDEX_PAGE[] PROGMEM = R"rawliteral(
<!DOCTYPE html><html><head><meta name="viewport" content="width=device-width,initial-scale=1">
<title>RFM69 Gateway</title>
</head><body>
<h3>RFM69 Gateway</h3>
<ul>
  <li><a href="/config/network">Network</a></li>
  <li><a href="/config/radio">Radio</a></li>
#if ENABLE_MQTT_CONFIG
  <li><a href="/config/mqtt">MQTT</a></li>
#endif
</ul>
</body></html>
)rawliteral";
  server.send_P(200, "text/html", INDEX_PAGE);
}

// Network config page (GET)
void handleConfigNetworkGet() {
  String page = "<html><body><h3>Network Configuration</h3>";
  page += "<form method='POST' action='/config/network'>";
  page += "<label><input type='checkbox' name='use_dhcp' " + String(cfg_ptr->use_dhcp ? "checked" : "") + "> Use DHCP</label><br>";
  page += "Static IP: <input name='ip' value='" + String(cfg_ptr->ip) + "'><br>";
  page += "Netmask: <input name='netmask' value='" + String(cfg_ptr->netmask) + "'><br>";
  page += "Gateway: <input name='gateway' value='" + String(cfg_ptr->gateway) + "'><br>";
  page += "DNS1: <input name='dns1' value='" + String(cfg_ptr->dns1) + "'><br>";
  page += "DNS2: <input name='dns2' value='" + String(cfg_ptr->dns2) + "'><br>";
  page += "<input type='submit' value='Save'>";
  page += "</form>";
#if ENABLE_WIFI_MANAGER
  page += "<p><a href='/startAP'>Start WiFi Config Portal</a> (captive portal)</p>";
#endif
  page += "</body></html>";
  server.send(200, "text/html", page);
}

// Network config page (POST)
void handleConfigNetworkPost() {
  GlobalConfig *wcfg = (GlobalConfig*)cfg_ptr;
  bool useDhcp = server.hasArg("use_dhcp");
  wcfg->use_dhcp = useDhcp;
  if (!useDhcp) {
    if (server.hasArg("ip")) strncpy(wcfg->ip, server.arg("ip").c_str(), sizeof(wcfg->ip)-1);
    if (server.hasArg("netmask")) strncpy(wcfg->netmask, server.arg("netmask").c_str(), sizeof(wcfg->netmask)-1);
    if (server.hasArg("gateway")) strncpy(wcfg->gateway, server.arg("gateway").c_str(), sizeof(wcfg->gateway)-1);
    if (server.hasArg("dns1")) strncpy(wcfg->dns1, server.arg("dns1").c_str(), sizeof(wcfg->dns1)-1);
    if (server.hasArg("dns2")) strncpy(wcfg->dns2, server.arg("dns2").c_str(), sizeof(wcfg->dns2)-1);
  } else {
    // clear static fields when DHCP selected
    wcfg->ip[0] = '\0';
    wcfg->netmask[0] = '\0';
    wcfg->gateway[0] = '\0';
    wcfg->dns1[0] = '\0';
    wcfg->dns2[0] = '\0';
  }
  config_save();
  server.send(200, "text/html", "<html><body>Saved. Rebooting...<script>setTimeout(function(){location='/';},2000);</script></body></html>");
  delay(500);
  ESP.restart();
}

// Radio config (GET)
void handleConfigRadioGet() {
  String page = "<html><body><h3>Radio Configuration</h3>";
  page += "<form method='POST' action='/config/radio'>";
  page += "Network ID: <input name='networkid' value='" + String(cfg_ptr->networkid) + "'><br>";
  page += "Node ID: <input name='nodeid' value='" + String(cfg_ptr->nodeid) + "'><br>";
  page += "Encrypt key: <input name='encryptkey' value='" + String(cfg_ptr->encryptkey) + "' maxlength='16'><br>";
  page += "Power level: <input name='powerlevel' value='" + String(cfg_ptr->powerlevel & 0x1F) + "'><br>";
  page += "<input type='submit' value='Save'>";
  page += "</form>";
  page += "</body></html>";
  server.send(200, "text/html", page);
}

// Radio config (POST)
void handleConfigRadioPost() {
  GlobalConfig *wcfg = (GlobalConfig*)cfg_ptr;
  if (server.hasArg("networkid")) wcfg->networkid = (uint8_t)atoi(server.arg("networkid").c_str());
  if (server.hasArg("nodeid")) wcfg->nodeid = (uint8_t)atoi(server.arg("nodeid").c_str());
  if (server.hasArg("encryptkey")) strncpy(wcfg->encryptkey, server.arg("encryptkey").c_str(), sizeof(wcfg->encryptkey)-1);
  if (server.hasArg("powerlevel")) wcfg->powerlevel = (uint8_t)atoi(server.arg("powerlevel").c_str()) & 0x1F;
  config_save();
  server.send(200, "text/html", "<html><body>Saved. Rebooting...<script>setTimeout(function(){location='/';},2000);</script></body></html>");
  delay(500);
  ESP.restart();
}

#if ENABLE_MQTT_CONFIG
void handleConfigMqttGet() {
  String page = "<html><body><h3>MQTT Configuration</h3>";
  page += "<form method='POST' action='/config/mqtt'>";
  page += "MQTT broker: <input name='mqttbroker' value='" + String(cfg_ptr->mqttbroker) + "'><br>";
  page += "MQTT port: <input name='mqttport' value='" + String(cfg_ptr->mqtt_port) + "'><br>";
  page += "MQTT user: <input name='mqttuser' value='" + String(((GlobalConfig*)cfg_ptr)->mqtt_user) + "'><br>";
  page += "MQTT pass: <input name='mqttpass' type='password' value='" + String(((GlobalConfig*)cfg_ptr)->mqtt_pass) + "'><br>";
  page += "<input type='submit' value='Save'>";
  page += "</form>";
  page += "</body></html>";
  server.send(200, "text/html", page);
}

void handleConfigMqttPost() {
  GlobalConfig *wcfg = (GlobalConfig*)cfg_ptr;
  if (server.hasArg("mqttbroker")) strncpy(wcfg->mqttbroker, server.arg("mqttbroker").c_str(), sizeof(wcfg->mqttbroker)-1);
  if (server.hasArg("mqttport")) wcfg->mqtt_port = (uint16_t)atoi(server.arg("mqttport").c_str());
  if (server.hasArg("mqttuser")) strncpy(wcfg->mqtt_user, server.arg("mqttuser").c_str(), sizeof(wcfg->mqtt_user)-1);
  if (server.hasArg("mqttpass")) strncpy(wcfg->mqtt_pass, server.arg("mqttpass").c_str(), sizeof(wcfg->mqtt_pass)-1);
  config_save();
  server.send(200, "text/html", "<html><body>Saved. Rebooting...<script>setTimeout(function(){location='/';},2000);</script></body></html>");
  delay(500);
  ESP.restart();
}
#endif // ENABLE_MQTT_CONFIG

void handleStartAP() {
  server.send(200, "text/plain", "Starting config portal...");
  delay(200);
#if ENABLE_WIFI_MANAGER
  wifi_start_config_portal();
#else
  server.send(500, "text/plain", "WiFiManager not available");
#endif
}

void web_init(const GlobalConfig *cfg) {
  cfg_ptr = cfg;
  server.on("/", HTTP_GET, handleRoot);
  server.on("/config/network", HTTP_GET, handleConfigNetworkGet);
  server.on("/config/network", HTTP_POST, handleConfigNetworkPost);
  server.on("/config/radio", HTTP_GET, handleConfigRadioGet);
  server.on("/config/radio", HTTP_POST, handleConfigRadioPost);
#if ENABLE_MQTT_CONFIG
  server.on("/config/mqtt", HTTP_GET, handleConfigMqttGet);
  server.on("/config/mqtt", HTTP_POST, handleConfigMqttPost);
#endif
#if ENABLE_WIFI_MANAGER
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
