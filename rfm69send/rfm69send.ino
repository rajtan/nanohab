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
#define POWER_LEVEL   31
#define NETWORKID     200  // The same on all nodes that talk to each other
#define NODEID        201  // The unique identifier of this node
#define RECEIVER      GW_NODEID    // The recipient of packets
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
#else
#define RFM69_CS      10
#define RFM69_IRQ     2
#define RFM69_IRQN    digitalPinToInterrupt(RFM69_IRQ)
#define RFM69_RST     9
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
    pGC->nodeid = NODEID;
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

//------------------------------------------------------------------------
void radio_setup() {
  int freq;
  static const char PROGMEM JSONtemplate[] =
    R"({"msgType":"config","freq":%d,"rfm69hcw":%d,"netid":%d,"power":%d})";
  char payload[128];
 
  //PRE-CHECK RADIO
  //radio = RFM69(RFM69_CS, RFM69_IRQ, GC_IS_RFM69HCW, RFM69_IRQN);
  Serial.printf("RFM69_CS=%d, RFM69_IRQ=%d, GC_IS_RFM69HCW=%d, RFM69_IRQN=%d \n", RFM69_CS, RFM69_IRQ, GC_IS_RFM69HCW, RFM69_IRQN);

  // Hard Reset the RFM module
  pinMode(RFM69_RST, OUTPUT);
  digitalWrite(RFM69_RST, HIGH);
  delay(100);
  digitalWrite(RFM69_RST, LOW);
  delay(100);

  // Initialize radio
  //PRE-CHECK RADIO
  //radio.initialize(pGC->rfmfrequency, pGC->nodeid, pGC->networkid);
  Serial.printf("rfmfreq=%d, nodeid=%d, networkid=%d \n");
  if (GC_IS_RFM69HCW) {
  //PRE-CHECK RADIO
    //radio.setHighPower();    // Only for RFM69HCW & HW!
    Serial.println("HighPower");
  }
  //PRE-CHECK RADIO
  //radio.setPowerLevel(GC_POWER_LEVEL); // power output ranges from 0 (5dBm) to 31 (20dBm)
  Serial.printf("Poer level = %d\n",GC_POWER_LEVEL);
  //PRE-CHECK RADIO
  if (pGC->encryptkey[0] != '\0') radio.encrypt(pGC->encryptkey);
  Serial.print(" ENc Key : "); Serial.println(pGC->encryptkey);
  pinMode(LED, OUTPUT);

  Serial.print("\nSending at ");
  switch (pGC->rfmfrequency) {
    case RF69_433MHZ:
      freq = 433;
      break;
    case RF69_868MHZ:
      freq = 868;
      break;
    case RF69_915MHZ:
      freq = 915;
      break;
    case RF69_315MHZ:
      freq = 315;
      break;
    default:
      freq = -1;
      break;
  }
  Serial.print(freq); Serial.print(' ');
  Serial.print(pGC->rfmfrequency); Serial.println(" MHz");

  size_t len = snprintf_P(RadioConfig, sizeof(RadioConfig), JSONtemplate,
      freq, GC_IS_RFM69HCW, pGC->networkid, GC_POWER_LEVEL);
  if (len >= sizeof(RadioConfig)) {
    Serial.println("\n\n*** RFM69 config truncated ***\n");
  }
}

//////////////////////////////////////////////////////////////////////////
// Normal Setup and Loop
void normal_setup() {
  //while (!Serial); // wait until serial console is open, remove if not tethered to computer
  Serial.begin(SERIAL_BAUD);
  Serial.println();

  Serial.println("Arduino RFM69HCW Transmitter");

  // Hard Reset the RFM module
  pinMode(RFM69_RST, OUTPUT);
  digitalWrite(RFM69_RST, HIGH);
  delay(100);
  digitalWrite(RFM69_RST, LOW);
  delay(100);

  // Initialize radio
  if (!radio.initialize(FREQUENCY,NODEID,NETWORKID)) {
    Serial.println("radio.initialize failed!");
  }
  if (IS_RFM69HCW) {
    radio.setHighPower();    // Only for RFM69HCW & HW!
  }
  radio.setPowerLevel(31); // power output ranges from 0 (5dBm) to 31 (20dBm)

  radio.encrypt(ENCRYPTKEY);

  pinMode(LED, OUTPUT);
  Serial.print("\nTransmitting at ");
  Serial.print(FREQUENCY==RF69_433MHZ ? 433 : FREQUENCY==RF69_868MHZ ? 868 : 915);
  Serial.println(" MHz");
  Serial.print("Network "); Serial.print(NETWORKID);
  Serial.print(" Node "); Serial.println(NODEID); Serial.println();
}

//------------------------------------------------------------------------
void normal_loop() {
  int loops;
  uint32_t startMillis;
  static uint32_t deltaMillis = 0;

  delay(1000);  // Wait 1 second between transmits, could also 'sleep' here!

  char radioPerf[32];
  ultoa(packetnum++, radioPerf, 10);
  strcat(radioPerf, ",");
  ltoa(radio.readRSSI(false), radioPerf+strlen(radioPerf), 10);
  strcat(radioPerf, ",");
  ultoa(deltaMillis, radioPerf+strlen(radioPerf), 10);
  Serial.print("Sending "); Serial.print(radioPerf); Serial.print(' ');

  loops = 10;
  startMillis = millis();
  while (loops--) {
    if (radio.sendWithRetry(RECEIVER, radioPerf, strlen(radioPerf)+1)) {
      deltaMillis = millis() - startMillis;
      Serial.print(" OK ");
      Serial.println(deltaMillis);
      break;
    }
    else {
      Serial.print("!");
    }
    delay(50);
  }
  if (loops <= 0) {
    Serial.println(" Fail");
    deltaMillis = 0;
  }

  radio.receiveDone(); //put radio in RX mode
}

//////////////////////////////////////////////////////////////////////////
// Main
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
  
  WiFi.softAP(pGC->devapname, pGC->devappass);
  //mdns_setup();
  Serial.print("Access Point \"");Serial.print(pGC->devapname);
  Serial.println("\" started");
  Serial.print("IP address:\t");
  Serial.println(WiFi.softAPIP());         // Send the IP address of the ESP8266 to the computer
  webserver_setup();
}

//////////////////////////////////////////////////////////////////////////
// loop
void loop() {
  webServer.handleClient();
}