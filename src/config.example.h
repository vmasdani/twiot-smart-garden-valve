#include <Arduino.h>

const char *ssid PROGMEM = "ssid";
const char *password PROGMEM = "password";
const char *serverUrl = "http://url-or.ip:port";
const char *mqttBroker = "url-or.ip";

#define ACTIVE_LOW
// #define ACTIVE_HIGH // <- Set this instead for active high relay

#ifdef ACTIVE_LOW

#define RELAY_ON LOW
#define RELAY_OFF HIGH

#elif defined ACTIVE_HIGH

#define RELAY_ON HIGH
#define RELAY_OFF LOW

#endif
