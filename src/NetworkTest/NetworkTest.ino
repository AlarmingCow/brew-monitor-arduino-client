#include <Adafruit_CC3000.h>
#include <ccspi.h>
#include <SPI.h>
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

#define     SERVER                "fremont"

void setup(void) {
  Serial.begin(115200);

  Serial.print(F("Initializing CC3000 network device... "));
  if (!cc3000.begin()) {
    Serial.println(F("Unable to initialize the CC3000"));
    while(1);
  }
  Serial.println(F("done."));

  Serial.print(F("Connecting to WiFi access point. SSID: "));
  Serial.print(WLAN_SSID);
  Serial.print(F(", Password: "));
  Serial.print(WLAN_PASS);
  Serial.print(F(", Security Type: "));
  Serial.print(WLAN_SECURITY);
  Serial.print(F(" ... "));
  if (!cc3000.connectToAP(WLAN_SSID, WLAN_PASS, WLAN_SECURITY)) {
    Serial.println(F("Failed to connect to AP"));
    while(1);
  }
  Serial.println(F("done."));
  
  Serial.print(F("Waiting for DHCP resolution... "));
  while (!cc3000.checkDHCP()) {
    delay(100);
  }
  Serial.println(F("done."));
}

void loop(void) {
  Serial.print(F("Resolving IP address for server "));
  Serial.println(SERVER);
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
    
    Serial.print(F("Sending HTTP request..."));
    client.fastrprint(F("POST / HTTP/1.1\r\n"));
    client.fastrprint(F("Host: fremont:8080\r\n"));
    client.fastrprint(F("\r\n"));
    Serial.println(F("done."));
  } else {
    Serial.println(F("failed."));
    client.close();
    return;
  }
  
  Serial.println(F("HTTP response: "));
  while (client.connected()) {
    while (client.available()) {
      char c = client.read();
      Serial.print(c);
    }
  }
  
  client.close();
  
  Serial.println(F("\r\nTest Finished.\r\n\r\n"));
  
  delay(5000);
}
