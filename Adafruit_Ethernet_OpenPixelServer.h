#ifndef ADAFRUIT_ETHERNET_OPENPIXELSERVER_H
#define ADAFRUIT_ETHERNET_OPENPIXELSERVER_H

#include "Adafruit_Arduino_OpenPixelServer.h"
#include "Ethernet.h"

// Define an OpenPixelServer class that uses the EthernetServer and EthernetClient class.
typedef OpenPixelServerBase<EthernetServer, EthernetClient> OpenPixelServer;

#endif
