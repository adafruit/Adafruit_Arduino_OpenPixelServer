// Example using the ESP8266 as an open pixel server.
#include "ESP8266WiFi.h"
#include "ESP8266mDNS.h"
#include "Adafruit_Arduino_OpenPixelServer.h"
#include "Adafruit_ESP8266_OpenPixelServer.h"

// Set your WiFi SSID and password below:
const char* ssid = "...your WiFi SSID...";
const char* password = "...your WiFi password...";

// Set a mDNS name for the server.  The address will be this value + '.local', like 'pixels.local'.
// This should work natively on OSX, but on Linux you need avahi installed and on Windows you need
// Apple's Bonjour print services installed: https://support.apple.com/kb/DL999?locale=en_US
const char* mdnsName = "pixels";

// Create an mDNS responder to give this pixel server an easy to find name.
MDNSResponder mdns;

// Create a listening server on TCP port 7890 (the default for open pixel server).
WiFiServer server(7890);

// Create an open pixel server that listens on the above server.
// Note that this can take an optional maxDataLength parameter which controls
// how much memory is available for the data buffer.  The default is 510 bytes
// which allows for 170 pixels in a single strip/channel (each pixel is 3 bytes).
OpenPixelServer pixelServer(server);


void setup() {
  // Setup serial port.
  Serial.begin(115200);
  Serial.println("");

  // Setup WiFi connection.
  WiFi.begin(ssid, password);

  // Wait for WiFi connection.
  Serial.print("Connecting...");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  // Setup mDNS responder with the name 'pixels.local'.
  if (mdns.begin(mdnsName, WiFi.localIP())) {
    Serial.print("mDNS responder started on address: ");
    Serial.print(mdnsName);
    Serial.println(".local");
  }

  // Setup open pixel server (this will also setup the WiFiServer instance).
  int result = pixelServer.begin();
  if (result != OpenPixelServer::SUCCESS) {
    Serial.print("Error starting pixel server: ");
    Serial.println(result, DEC);
    // Halt gracefully without trigger watchdog resets.
    while (true) {
      delay(100);
    }
  }
}

void loop() {
  // First let the mDNS responder deal with any name queries.
  mdns.update();

  // Now check if an open pixel client connected and sent a packet.
  int result = pixelServer.listen();
  if (result == OpenPixelServer::RECEIVED_PACKET) {
    Serial.println("Received packet!");
    Serial.print("Channel: "); Serial.println(pixelServer.getChannel(), DEC);
    Serial.print("Command: "); Serial.println(pixelServer.getCommand(), DEC);
    Serial.print("Data Length: "); Serial.println(pixelServer.getDataLength(), DEC);
    // Access the packet data with the pixelServer.getData() function.  It returns
    // a uint8_t pointer to the start of the data.
    //uint8_t* data = pixelServer.getData();
  }
  else if (result == OpenPixelServer::IGNORED_PACKET) {
    Serial.println("Ignored a packet that was too long to store!");
  }

  // Small delay to give the EPS8266 RTOS time to do other things.
  delay(10);
}
