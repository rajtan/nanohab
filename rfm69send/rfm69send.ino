// ref: https://github.com/bbx10/nanohab/blob/master/rfm69send/rfm69send.ino
// heavily modified by rt@mps.in 
#if defined(ESP8266)
#include <ESP8266WiFi.h>
#include <pgmspace.h>
#include <EEPROM.h>
//#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
ESP8266WebServer webServer(80);
#endif

#if defined(ESP32)
#include <WiFi.h>
#include <pgmspace.h>
#include <EEPROM.h>
//#include <DNSServer.h>
#include <WebServer.h>
#include <ESPmDNS.h>
WebServer webServer(80);
#endif

// define for testing without actual h/w setup
#define TESTING_CONFIG 0

#include "enums_structs.h"

//////////////////////////////////////////////////////////////////////////
// Web Server Part
#include "html_pages.h"

MDNSResponder mdns;

#include <RFM69.h>    //get it here: https://www.github.com/lowpowerlab/rfm69
#include <SPI.h>

RFM69 radio;
char RadioConfig[128];

//*********************************************************************************************
// *********** IMPORTANT SETTINGS - YOU MUST CHANGE/ONFIGURE TO FIT YOUR HARDWARE *************
//*********************************************************************************************
#define GW_NODEID     1
#define IS_RFM69HCW   true // set to 'true' if you are using an RFM69HCW module
#define POWER_LEVEL   16
#define NETWORKID     200  // The same on all nodes that talk to each other
// for gw server NODEID is not required in configuration, wherever needed it will be derived in code
//#define NODEID        201  // The unique identifier of this node
//Match frequency to the hardware version of the radio on your Feather
//#define FREQUENCY     RF69_433MHZ
#define FREQUENCY     RF69_868MHZ
//#define FREQUENCY     RF69_915MHZ

//*********************************************************************************************
#define SERIAL_BAUD   19200

#if defined(__AVR_ATmega168__) || defined(__AVR_ATmega328P__) || defined(__AVR_ATmega88) || defined(__AVR_ATmega8__) || defined(__AVR_ATmega88__)
#define RFM69_CS      10
#define RFM69_IRQ     2
#define RFM69_IRQN    digitalPinToInterrupt(RFM69_IRQ)
#define RFM69_RST     9
#define LED           13  // onboard blinky
#elif defined(__arm__)//Use pin 10 or any pin you want
// Arduino Zero works
#define RFM69_CS      10
#define RFM69_IRQ     5
#define RFM69_IRQN    digitalPinToInterrupt(RFM69_IRQ)
#define RFM69_RST     6
#define LED           13  // onboard blinky
#elif defined(ESP8266)
// ESP8266
#define RFM69_CS      15  // GPIO15/D8
#define RFM69_IRQ     4   // GPIO04/D2
#define RFM69_IRQN    digitalPinToInterrupt(RFM69_IRQ)
#define RFM69_RST     2   // GPIO02/D4
#define LED           0   // GPIO00/D3, onboard blinky for Adafruit Huzzah
#elif defined (ESP32)
#define RFM69_CS      5
#define RFM69_IRQ     15
#define RFM69_IRQN    digitalPinToInterrupt(RFM69_IRQ)
#define RFM69_RST     2
#define LED           13  // onboard blinky
#endif

// Default values
const char PROGMEM ENCRYPTKEY[]   = "sampleKey123";
const char PROGMEM DEV_DESC[]      = "Descriptive Sens Name 12";
const char PROGMEM DEV_AP_NAME[]  = "SENSDEV";
const char PROGMEM DEV_AP_PASS[]  = "SensPass";
const char PROGMEM MDNS_NAME[]    = "devconf";

struct _GLOBAL_CONFIG {
  uint32_t    checksum;

  char        encryptkey[16+1];
  uint8_t     networkid;
  uint8_t     nodeid;
  char        devdesc[24+1];
  char        devapname[12+1];
  char        devappass[12+1];
  char        mdnsname[12+1];

  uint8_t     powerlevel; // bits 0..4 power leve, bit 7 RFM69HCW 1=true
  uint8_t     rfmfrequency;
  uint32_t    ipaddress;  // if 0, use DHCP
  uint32_t    ipnetmask;
  uint32_t    ipgateway;
  uint32_t    ipdns1;
  uint32_t    ipdns2;
};

#define GC_POWER_LEVEL    (pGC->powerlevel & 0x1F)
#define GC_IS_RFM69HCW  ((pGC->powerlevel & 0x80) != 0)

struct _GLOBAL_CONFIG *pGC;

// used in stress testing of sending consecutive messaages
uint32_t packetnum = 0;  // packet counter, we increment per xmission

#define SELECTED_FREQ(f)  ((pGC->rfmfrequency==f)?"selected":"")

struct _SENS_INFO sens_list[] = {
  {.s_num = 1, .s_type=BINARY_SENSOR, .s_subtype=MOTION_SENSOR, .s_val=0.0, .s_val_type=VT_NULL},
  {.s_num = 2, .s_type=BINARY_SENSOR, .s_subtype=DOOR_SENSOR, .s_val=1.0, .s_val_type=VT_ONOFF},
};

//////////////////////////////////////////////////////////////////////////
// EEPROM Part
uint32_t gc_checksum() {
  uint8_t *p = (uint8_t *)pGC;
  uint32_t checksum = 0;
  p += sizeof(pGC->checksum);
  for (size_t i = 0; i < (sizeof(*pGC) - 4); i++) {
    checksum += *p++;
  }
  return checksum;
}

//------------------------------------------------------------------------
void eeprom_setup() {
  EEPROM.begin(4096);
  pGC = (struct _GLOBAL_CONFIG *)EEPROM.getDataPtr();
  // if checksum bad init GC else use GC values
  if (gc_checksum() != pGC->checksum) {
    Serial.println("Factory reset");
    memset(pGC, 0, sizeof(*pGC));

    strcpy_P(pGC->encryptkey, ENCRYPTKEY);
    pGC->networkid = NETWORKID;
    //for gw server nodeid (mynodeid) is alwaays 1
    //pGC->nodeid = NODEID;
    pGC->nodeid = GW_NODEID;
    strcpy_P(pGC->devdesc, DEV_DESC);
    strcpy_P(pGC->devapname, DEV_AP_NAME);
    strcpy_P(pGC->devappass, DEV_AP_PASS);
    strcpy_P(pGC->mdnsname, MDNS_NAME);

    pGC->powerlevel = ((IS_RFM69HCW)?0x80:0x00) | POWER_LEVEL;
    pGC->rfmfrequency = FREQUENCY;

    pGC->checksum = gc_checksum();
    EEPROM.commit();
  }
}

#include "webconf_server_code.h"
#include "recv_send_code.h"


//////////////////////////////////////////////////////////////////////////
// webconf setup
void webconf_setup(){
  WiFi.softAP(pGC->devapname, pGC->devappass);
  //mdns_setup();
  Serial.print("Access Point \"");Serial.print(pGC->devapname);
  Serial.println("\" started");
  Serial.print("IP address:\t");
  Serial.println(WiFi.softAPIP());         // Send the IP address of the ESP8266 to the computer
  webserver_setup();
}

//////////////////////////////////////////////////////////////////////////
// webconf loop
void webconf_loop() {
  webServer.handleClient();
}

//////////////////////////////////////////////////////////////////////////
// Main Setup
void setup() {

  // Shut Down WiFi connection
  // WiFi.disconnect( true, false ); // disconnect but dont erase credentials (or save time of it)
  // delay( 1 );
  // WiFi.mode( WIFI_OFF );

  // //Bring up the WiFi connection
  // WiFi.forceSleepWake();
  // delay( 1 );

  Serial.begin(SERIAL_BAUD);
  eeprom_setup();
  delay(100);

  if (TESTING_CONFIG == 1) {
    webconf_setup();
  } else {
    Serial.println("Switching to Radio Mode");
    radio_gw_setup();
  }
}

//////////////////////////////////////////////////////////////////////////
// loop
void loop() {
  if (TESTING_CONFIG == 1) {
    webconf_loop();
  } else {
    radio_gw_loop();
  }
}