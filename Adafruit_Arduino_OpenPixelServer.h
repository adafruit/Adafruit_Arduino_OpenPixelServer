#ifndef ADAFRUIT_ARDUINO_OPENPIXELSERVER_H
#define ADAFRUIT_ARDUINO_OPENPIXELSERVER_H

// Uncomment the below to enable debug output.
//#define DEBUG_MODE 1

// Set where debug output will be printed.
#define DEBUG_PRINTER Serial

// Define debug print macros that disappear when not enabled.
#ifdef DEBUG_MODE
  #define DEBUG_PRINT(...) { DEBUG_PRINTER.print(__VA_ARGS__); }
  #define DEBUG_PRINTLN(...) { DEBUG_PRINTER.println(__VA_ARGS__); }
#else
  #define DEBUG_PRINT(...) { }
  #define DEBUG_PRINTLN(...) { }
#endif

// Make the base OpenPixelServer class templated on a server and client type.
// Normally we could just work with generic Service and Client pointers, but
// the generic Server class doesn't implement the actual server interface.
// It appears this part of the Arduino core is somewhat unimplemented, see:
//   https://github.com/arduino/Arduino/issues/616
// As a workaround define a base OpenPixelServer class that takes as template
// parameters the explicit type of the server and client type to expect.
// Then specialized headers like Adafruit_ESP8266_OpenPixelServer.h can typedef
// the definition of an OpenPixelServerBase class that fills in the appropriate
// server and client types (to hide the template usage from users).
template <class TServer, class TClient>
class OpenPixelServerBase {
public:
  // Create an open pixel server instance that listens on the specified Arduino
  // server instance (like a ESP8266 WiFiServer class instance).  Can also set a
  // maximum received data buffer length (default is 510), and a timeout time
  // in milliseconds (default is 1 second).
  // An open pixel server command can have at most 64k of data, however this is
  // a significantly large chunk of memory for most embedded processors (the ESP8266
  // only has ~80k of memory available for example) so care should be taken to
  // balance the max data size vs. memory usage.  For lighting pixels each pixel
  // requires 3 bytes of memory so a size of 510 can support up to 170 pixels in
  // a single channel.
  OpenPixelServerBase(const TServer& server, const uint16_t& maxDataLength=510,
                      const uint32_t timeoutMS=1000):
    _server(server),
    _maxDataLen(maxDataLength),
    _timeoutMS(timeoutMS),
    _data(NULL),
    _receivedLen(0),
    _expectedLen(0),
    _readTimeout(0)
  {
    // Try to allocate memory for the data buffer plus header data.  If this fails
    // and null is returned it will be handled in the start() function (an ERROR_MEMORY
    // error is returned).
    _data = (uint8_t*) malloc(_maxDataLen+4);
    // Expect a packet.
    _expectNewPacket();
  }

  // Copy constructor will create a pixel server instance that has the same
  // server and data buffer size as the provided copy.
  OpenPixelServerBase(const OpenPixelServerBase& copy):
    OpenPixelServerBase(copy._server, copy._maxDataLen, copy._timeoutMS)
  {}

  // Destructor frees buffer memory.
  ~OpenPixelServerBase() {
    if (_data != NULL) {
      free(_data);
      _data = NULL;
    }
  }

  // Return codes for the begin() function.
  enum BeginResult {
    SUCCESS = 0,          // Server was successfully started.

    ERROR_NO_MEMORY = -1, // Not enough memory available to create the data
                          // buffer.
  };

  // Initialize the open pixel server and start listening for packets.
  // Will return SUCCESS if it succeeded in listening or a negative error code
  // if it failed.  See the BeginResult enum above for error codes.  Note that
  // this will call begin on the server instance passed to the constructor so
  // you don't need to call it yourself.
  BeginResult begin() {
    // Check that memory for the data buffer was allocated.
    if (_data == NULL) {
      return ERROR_NO_MEMORY;
    }
    // Start the listening server and return success.
    _server.begin();
    return SUCCESS;
  }

  // Return codes for the listen() function.
  enum ListenResult {
    WORKING = 0,          // Server is in a good state and listening for
                          // open pixel packets.  Keep calling listen() and wait
                          // for the RECEIVED_PACKET response.

    RECEIVED_PACKET = 1,  // Received a full packet, now read it using the get
                          // functions like getChannel, getCommand, etc.

    IGNORED_PACKET = -1,  // Received a packet which was too big to fit in the
                          // data buffer and it was ignored.  Try increasing
                          // the maxDataLength value in the constructor call.
  };

  // Listen for an open pixel packet to be received.  Should be called in every
  // loop iteration to make sure data isn't missed.  Will return a result code
  // that indicates if a packet was received (in which case you can call the
  // get functions like getChannel, getCommand, etc. to read the packet data) or
  // if there was an error or other problem (see the ListenResult enum values).
  ListenResult listen() {
    // Check if we've timed out trying to read a packet.
    if (millis() >= _readTimeout) {
      DEBUG_PRINTLN("Timeout waiting for pixel packet!");
      // Throw away any in-progress packet and start over reading a new packet.
      _expectNewPacket();
    }
    // Check for available data from a connected client.
    TClient client = _server.available();
    if (client) {
      // Read data from the client until a full packet is received.
      while (client.available() > 0) {
        // Read data and bump the timeout time since data was received.
        uint8_t d = client.read();
        _readTimeout = millis() + _timeoutMS;
        DEBUG_PRINT("Received: 0x");
        DEBUG_PRINTLN(d, HEX);
        // Add byte to the buffer if there is space.
        if (_receivedLen < _maxDataLen) {
          _data[_receivedLen++] = d;
        }
        // Decrease expected byte count.
        _expectedLen--;
        // Check if exactly 4 bytes have been received (a full header) and
        // validate it.
        if (_receivedLen == 4) {
          // Check the command is either 0 (light pixels) or 255 (system exclusive).
          uint8_t command = getCommand();
          if (command != 0 && command != 255) {
            // Unknown command, start over parsing a new packet.
            DEBUG_PRINT("Unknown command: ");
            DEBUG_PRINTLN(command, DEC);
            _expectNewPacket();
          }
          else {
            // Command is good (as far as we know), parse out the expected length.
            _expectedLen = getDataLength();
            DEBUG_PRINT("Reading packet data of size: ");
            DEBUG_PRINTLN(_expectedLen);
          }
        }
        // Finally check if a full packet has been received (expected length is
        // now zero).
        if (_expectedLen == 0) {
          // Reset to expect a new packet.
          _expectNewPacket();
          // Check if there wasn't enough space to store the packet and that it
          // should be ignored.
          if (getDataLength() > _maxDataLen) {
            // Not enough room, ignore the packet.
            return IGNORED_PACKET;
          }
          else {
            // Woo hoo! Successfully parsed the packet.
            return RECEIVED_PACKET;
          }
        }
      }
    }
    // No data to read, return that we're still working on getting a packet.
    return WORKING;
  }

  // Return the channel value for the received packet.  Only call this function
  // after isPacketReceived returns true!
  uint8_t getChannel() {
    // Grab the channel from the buffer of received data.
    return _data[0];
  }

  // Return the command value for the received packet.  Only call this function
  // after isPacketReceived returns true!
  uint8_t getCommand() {
    // Grab the command from the buffer of received data.
    return _data[1];
  }

  // Return the length of data for the received packet.  Only call this function
  // after isPacketReceived returns true!
  uint16_t getDataLength() {
    // Grab the length from the buffer of received data.
    uint16_t dataLen = (_data[2] << 8) | _data[3];
    return dataLen;
  }

  // Return a point to the data for the received packet.  Only call this function
  // after isPacketReceived returns true!  Also do not take ownership of this
  // data buffer, i.e. don't call free/delete on it to try to clean it up.  The
  // pixel server class owns the lifetime and contents of this buffer.  Copy out
  // any important data before you call isPacketReceived again as the data can
  // and will change!
  uint8_t* getData() {
    // Return the start of the data from the packet.
    return _data+4;
  }

private:
  TServer _server;        // The server instance to listen for connections.
  uint16_t _maxDataLen;   // Maximum data this server can receive.
  uint32_t _timeoutMS;    // How long to wait (in millisconds) for a packet to
                          // be received.
  uint8_t* _data;         // Data that has been received.
  uint16_t _receivedLen;  // Amount of data received in the _data buffer.
  uint16_t _expectedLen;  // Amount of data expected to read.
  uint32_t _readTimeout;  // Millis() time to give up reading any in progress
                          // packet and expect a new one.  Useful to get back
                          // into a good state if junk data is received.

  void _expectNewPacket() {
    // Reset the internal state to start expecting a new packet (throwing out any
    // packet in the process of being received).
    _expectedLen = 4;
    _receivedLen = 0;
    // Bump the timeout to allow time to read a new packet.
    _readTimeout = millis() + _timeoutMS;
  }
};

#endif
