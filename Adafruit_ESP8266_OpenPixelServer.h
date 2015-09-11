#ifndef ADAFRUIT_ESP8266_OPENPIXELSERVER_H
#define ADAFRUIT_ESP8266_OPENPIXELSERVER_H

#include "Adafruit_Arduino_OpenPixelServer.h"
#include "ESP8266WiFi.h"

// Define an OpenPixelServer class that uses the ESP8266 WiFiServer and WiFiClient.
typedef OpenPixelServerBase<WiFiServer, WiFiClient> OpenPixelServer;

#endif
