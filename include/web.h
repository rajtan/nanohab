#pragma once
#include "config.h"

void web_init(const GlobalConfig *cfg);
void web_loop();
void web_broadcast(const char *msg); // optional, no-op if websockets disabled

// If WiFiManager compiled-in, this triggers the captive portal (blocking autoConnect).
// Intended to be called on-demand (e.g., by button), not at every boot.
#if ENABLE_WIFI_MANAGER
void wifi_start_config_portal();
#endif