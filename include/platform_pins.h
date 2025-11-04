#pragma once

// Platform-specific pin map; adjust as needed.
// Default targets ESP8266 as in original project.
#if defined(ESP8266)
#define RFM69_CS   15
#define RFM69_IRQ  4
#define RFM69_RST  2
#define LED_PIN    0
#else
// generic Arduino defaults (change if required)
#define RFM69_CS   10
#define RFM69_IRQ  2
#define RFM69_RST  9
#define LED_PIN    13
#endif