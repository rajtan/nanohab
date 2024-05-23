//////////////////////////////////////////////////////////////////////////
// MDNS Part
void mdns_setup(void) {
  if (pGC->mdnsname[0] == '\0') return;

  if (mdns.begin(pGC->mdnsname)) {
    Serial.println("MDNS responder started");
    mdns.addService("http", "tcp", 80);
    //mdns.addService("ws", "tcp", 81);
  }
  else {
    Serial.println("MDNS.begin failed");
  }
  Serial.printf("Connect to http://%s.local or http://", pGC->mdnsname);
  Serial.println(WiFi.localIP());
}

//////////////////////////////////////////////////////////////////////////
// Web Server Part
//------------------------------------------------------------------------
void handleRoot()
{
  Serial.print("Free heap="); Serial.println(ESP.getFreeHeap());

  webServer.send_P(200, "text/html", INDEX_HTML);
}

//------------------------------------------------------------------------
void handleNotFound()
{
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += webServer.uri();
  message += "\nMethod: ";
  message += (webServer.method() == HTTP_GET)?"GET":"POST";
  message += "\nArguments: ";
  message += webServer.args();
  message += "\n";
  for (uint8_t i=0; i<webServer.args(); i++){
    message += " " + webServer.argName(i) + ": " + webServer.arg(i) + "\n";
  }
  webServer.send(404, "text/plain", message);
}

//------------------------------------------------------------------------
// Reset global config back to factory defaults
void handleConfigReset()
{
  pGC->checksum++;
  EEPROM.commit();
  ESP.restart();
  delay(1000);
}

//------------------------------------------------------------------------

void handleConfigDev()
{
  size_t formFinal_len = strlen_P(CONFIGUREDEV_HTML) + sizeof(*pGC);
  Serial.println(sizeof(*pGC));
  Serial.println(strlen_P(CONFIGUREDEV_HTML));
  Serial.println(formFinal_len);
  char *formFinal = (char *)malloc(formFinal_len);
  if (formFinal == NULL) {
    Serial.println("formFinal malloc failed");
    return;
  }
  snprintf_P(formFinal, formFinal_len, CONFIGUREDEV_HTML,
      pGC->encryptkey, pGC->networkid, pGC->nodeid,  pGC->devdesc, pGC->devapname, pGC->devappass,
      (GC_IS_RFM69HCW)?"checked":"", (GC_IS_RFM69HCW)?"":"checked", GC_POWER_LEVEL,
      SELECTED_FREQ(RF69_315MHZ), SELECTED_FREQ(RF69_433MHZ),
      SELECTED_FREQ(RF69_868MHZ), SELECTED_FREQ(RF69_915MHZ)
      );

  webServer.send(200, "text/html", formFinal);
  free(formFinal);
}

//------------------------------------------------------------------------
void handleConfigDevWrite()
{
  bool commit_required = false;
  String argi, argNamei;

  for (uint8_t i=0; i<webServer.args(); i++) {
    Serial.print(webServer.argName(i));
    Serial.print('=');
    Serial.println(webServer.arg(i));
    argi = webServer.arg(i);
    argNamei = webServer.argName(i);

    if (argNamei == "encryptkey") {
      const char *enckey = argi.c_str();
      if (strcmp(enckey, pGC->encryptkey) != 0) {
        commit_required = true;
        strcpy(pGC->encryptkey, enckey);
      }
    }
    else if (argNamei == "networkid") {
      uint8_t formnetworkid = argi.toInt();
      if (formnetworkid != pGC->networkid) {
        commit_required = true;
        pGC->networkid = formnetworkid;
      }
    }
    else if (argNamei == "nodeid") {
      uint8_t formnodeid = argi.toInt();
      if (formnodeid != pGC->nodeid) {
        commit_required = true;
        pGC->nodeid = formnodeid;
      }
    }
    else if (argNamei == "devdesc") {
      const char *devdesc = argi.c_str();
      if (strcmp(devdesc, pGC->devdesc) != 0) {
        commit_required = true;
        strcpy(pGC->devdesc, devdesc);
      }
    }
    else if (argNamei == "devapname") {
      const char *apname = argi.c_str();
      if (strcmp(apname, pGC->devapname) != 0) {
        commit_required = true;
        strcpy(pGC->devapname, apname);
      }
    }
    else if (argNamei == "devappass") {
      const char *appass = argi.c_str();
      if (strcmp(appass, pGC->devappass) != 0) {
        commit_required = true;
        strcpy(pGC->devappass, appass);
      }
    }
    else if (argNamei == "mdnsname") {
      const char *mdnsname = argi.c_str();
      if (strcmp(mdnsname, pGC->mdnsname) != 0) {
        commit_required = true;
        strcpy(pGC->mdnsname, mdnsname);
      }
    }
    else if (argNamei == "rfm69hcw") {
      uint8_t hcw = argi.toInt();
      if (hcw != GC_IS_RFM69HCW) {
        commit_required = true;
        pGC->powerlevel = (hcw << 7) | GC_POWER_LEVEL;
      }
    }
    else if (argNamei == "powerlevel") {
      uint8_t powlev = argi.toInt();
      if (powlev != GC_POWER_LEVEL) {
        commit_required = true;
        pGC->powerlevel = (GC_IS_RFM69HCW << 7) | powlev;
      }
    }
    else if (argNamei == "rfmfrequency") {
      uint8_t freq = argi.toInt();
      if (freq != pGC->rfmfrequency) {
        commit_required = true;
        pGC->rfmfrequency = freq;
      }
    }
  }
  handleRoot();
  if (commit_required) {
    pGC->checksum = gc_checksum();
    EEPROM.commit();
    ESP.restart();
    delay(1000);
  }
}

//------------------------------------------------------------------------
void webserver_setup() {
  webServer.on("/", handleRoot);
  webServer.on("/configDev", HTTP_GET, handleConfigDev);
  webServer.on("/configDev", HTTP_POST, handleConfigDevWrite);
  webServer.on("/configReset", HTTP_GET, handleConfigReset);
  webServer.onNotFound(handleNotFound);
  webServer.begin();
}


