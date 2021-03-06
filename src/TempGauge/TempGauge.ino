#include <Adafruit_CC3000.h>
#include <ccspi.h>
#include <SPI.h>
#include <Wire.h>
#include <RTClib.h>
#include <OneWire.h>
#include "WiFiSettings.h"

// CC3000 configuration
#define     ADAFRUIT_CC3000_IRQ    3    // MUST be an interrupt pin!
#define     ADAFRUIT_CC3000_VBAT   5    // VBAT and CS can be any two pins
#define     ADAFRUIT_CC3000_CS     10
// Use hardware SPI for the remaining pins
// On an UNO, SCK = 13, MISO = 12, and MOSI = 11
Adafruit_CC3000 cc3000 = Adafruit_CC3000(ADAFRUIT_CC3000_CS,
                                         ADAFRUIT_CC3000_IRQ,
                                         ADAFRUIT_CC3000_VBAT,
                                         SPI_CLOCK_DIV2);

// Other sketch configuration
#define     READING_DELAY_SECS     10     // Number of seconds to wait between readings.
#define     TIMEOUT_MS             15000  // How long to wait (in milliseconds) for a server connection to respond
#define     SERVER                "fremont"
#define     PORT                  8080

#define MAX_DS1820_SENSORS 1
byte addr[8];
OneWire ds(8);  // on pin 8

// State used to keep track of the current time and time since last temp reading.
unsigned long unixTimeAtStart = 0;   // Last value retrieved from time server
unsigned long sketchTimeAtStartMillis = 0;       // CPU milliseconds since last server query
unsigned long lastReading = 0;      // Time of last temperature reading.

void setup(void) {
  Serial.begin(115200);

  // Initialize and connect to the wireless network
  // This code is adapted from CC3000 example code.
  if (!cc3000.begin()) {
    Serial.println(F("Unable to initialize the CC3000!"));
    while(1);
  }
  if (!cc3000.connectToAP(WLAN_SSID, WLAN_PASS, WLAN_SECURITY)) {
    Serial.println(F("Failed to connect to AP!"));
    while(1);
  }
  // Wait for DHCP to complete
  while (!cc3000.checkDHCP()) {
    delay(100);
  }
  
  // Get an initial time value by querying an NTP server.
  unsigned long t = getTime();
  while (t == 0) {
    // Failed to get time, try again in a minute.
    delay(60*1000);
    t = getTime();
  }
  unixTimeAtStart = t;
  sketchTimeAtStartMillis = millis();
  
  if (!ds.search(addr))  {
    ds.reset_search();
    delay(250);
    return;
  }
  
  Serial.print("R=");
  for(int i = 0; i < 8; i++) {
    Serial.print(addr[i], HEX);
    Serial.print(" ");
  }

  if ( OneWire::crc8( addr, 7) != addr[7]) {
      Serial.print("CRC is not valid!\n");
      return;
  }

  if ( addr[0] == 0x10) {
      Serial.print("Device is a DS18S20 family device.\n");
  } else if ( addr[0] == 0x28) {
      Serial.print("Device is a DS18B20 family device.\n");
  } else {
      Serial.print("Device family is not recognized: 0x");
      Serial.println(addr[0],HEX);
      return;
  }
  
  Serial.println(F("Running..."));
}

void loop(void) {
  // Update the current time.
  // Note: If the sketch will run for more than ~24 hours, you probably want to query the time
  // server again to keep the current time from getting too skewed.
  unsigned long currentTime = unixTimeAtStart + (millis() - sketchTimeAtStartMillis)/1000;

  Serial.print(F("Starting read. Current time: ")); Serial.println(currentTime);
  lastReading = currentTime;

  float currentTemp = readTempF();
  Serial.print(F("Current temp: ")); Serial.println(currentTemp);

  dbWrite(currentTime, currentTemp);
  
  Serial.println(F("\n\rDone writing. Delay till next read.\n\r\n\r"));
  delay(READING_DELAY_SECS * 1000);
}

float readTempF() {
  byte i;
  byte data[12];
  
  ds.reset();
  ds.select(addr);
  
  ds.write(0x44, 1); // Start conversion
  delay(850);
  ds.reset();
  ds.select(addr);
  ds.write(0xBE); // Issue Read scratchpad command
  
  // Receive 9 bytes
  for ( i = 0; i < 9; i++) {
    data[i] = ds.read();
  }
  
  return ((data[1] << 8) + data[0]) * 0.0625;
}

void dbWrite(unsigned long currentTime, float currentTemp) {
  // Resolve server IP address
  Serial.print(SERVER); 
  uint32_t ip = 0;
  Serial.print(F(" I.P. address: "));
  while (ip == 0) {
    if (!cc3000.getHostByName(SERVER, &ip)) {
      Serial.println(F("Couldn't resolve!"));
    }
    delay(500);
  }
  cc3000.printIPdotsRev(ip);
  Serial.println("");
  
  Serial.print(F("Opening TCP connection... "));
  Adafruit_CC3000_Client client = cc3000.connectTCP(ip, 8080);
  if (client.connected()) {
    Serial.println(F("done."));
    
    String body = F("{\"time\":");
    body += currentTime;
    body += F("000,\"temp\":"); // currentTime is in seconds, need milliseconds
    body += currentTemp;
    body += F("}");
    
    String head = F("POST /thermReadings HTTP/1.1\r\n"
    "Host: fremont:8080\r\n"
    "Content-Type: application/json\r\n"
    "Content-Length: ");
    head += body.length();
    head += F("\r\n\r\n");
    
    Serial.println(F("Request:"));
    Serial.print(head.c_str());
    Serial.println(body.c_str());
    
    Serial.print(F("Sending request... "));
    client.fastrprint(head.c_str());
    client.fastrprint(body.c_str());
    Serial.println(F("done."));
  } else {
    Serial.println(F("Connection failed"));    
    client.close();
    return;
  }
  
  // Read data until either the connection is closed, or the idle timeout is reached.
  unsigned long lastRead = millis();
  Serial.println(F("Server response: "));
  while (client.connected() && (millis() - lastRead < TIMEOUT_MS)) {
    while (client.available()) {
      char c = client.read();
      Serial.print(c);
      lastRead = millis();
    }
  }
  client.close();
}

// Convert an array of bytes into a lower case hex string.
// Buffer MUST be two times the length of the input bytes array!
void hexString(uint8_t* bytes, size_t len, char* buffer) {
  for (int i = 0; i < len; ++i) {
    btoa2Padded(bytes[i], &buffer[i*2], 16);
  }
}

// Fill a 16 character buffer with the date in ISO8601 simple format, like '20130101T010101Z'.  
// Buffer MUST be at least 16 characters long!
void dateTime8601(int year, byte month, byte day, byte hour, byte minute, byte seconds, char* buffer) {
  ultoa(year, buffer, 10);
  btoa2Padded(month, buffer+4, 10);
  btoa2Padded(day, buffer+6, 10);
  buffer[8] = 'T';
  btoa2Padded(hour, buffer+9, 10);
  btoa2Padded(minute, buffer+11, 10);
  btoa2Padded(seconds, buffer+13, 10);
  buffer[15] = 'Z';
}

// Print a value from 0-99 to a 2 character 0 padded character buffer.
// Buffer MUST be at least 2 characters long!
void btoa2Padded(uint8_t value, char* buffer, int base) {
  if (value < base) {
    *buffer = '0';
    ultoa(value, buffer+1, base);
  }
  else {
    ultoa(value, buffer, base); 
  }
}

// getTime function adapted from CC3000 ntpTest sketch.
// Minimalist time server query; adapted from Adafruit Gutenbird sketch,
// which in turn has roots in Arduino UdpNTPClient tutorial.
unsigned long getTime(void) {
  Adafruit_CC3000_Client client;
  uint8_t       buf[48];
  unsigned long ip, startTime, t = 0L;

  // Hostname to IP lookup; use NTP pool (rotates through servers)
  if(cc3000.getHostByName("pool.ntp.org", &ip)) {
    static const char PROGMEM
      timeReqA[] = { 227,  0,  6, 236 },
      timeReqB[] = {  49, 78, 49,  52 };

    startTime = millis();
    do {
      client = cc3000.connectUDP(ip, 123);
    } while((!client.connected()) &&
            ((millis() - startTime) < TIMEOUT_MS));

    if(client.connected()) {
      // Assemble and issue request packet
      memset(buf, 0, sizeof(buf));
      memcpy_P( buf    , timeReqA, sizeof(timeReqA));
      memcpy_P(&buf[12], timeReqB, sizeof(timeReqB));
      client.write(buf, sizeof(buf));

      memset(buf, 0, sizeof(buf));
      startTime = millis();
      while((!client.available()) &&
            ((millis() - startTime) < TIMEOUT_MS));
      if(client.available()) {
        client.read(buf, sizeof(buf));
        t = (((unsigned long)buf[40] << 24) |
             ((unsigned long)buf[41] << 16) |
             ((unsigned long)buf[42] <<  8) |
              (unsigned long)buf[43]) - 2208988800UL;
      }
      client.close();
    }
  }
  return t;
}


