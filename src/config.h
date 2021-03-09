#include <Arduino.h>

const char *ssid PROGMEM = "Who am i";
const char *password PROGMEM = "12345678";
const char *serverUrl = "http://192.168.100.34";
const char *mqttBroker = "192.168.100.34";

#define ACTIVE_LOW
// #define ACTIVE_HIGH // <- Set this instead for active high relay

#ifdef ACTIVE_LOW

#define RELAY_ON LOW
#define RELAY_OFF HIGH

#elif defined ACTIVE_HIGH

#define RELAY_ON HIGH
#define RELAY_OFF LOW

#endif
