Brew Monitor Arduino Client
===========================

This is the Arduino client for [Brew Monitor](https://github.com/AlarmingCow/brew-monitor). It's WiFi enabled and periodically
sends updates from a digital temperature probe to a server over a RESTful web interface.

# Usage
Deploy using the [Arduino IDE](http://arduino.cc/en/main/software).

## WiFi Setup
You'll have to configure your wireless network. Add a file called 
`WiFiSettings.h` in the same directory as `TempGauge.ino` with the following 
definitions:

    #define     WLAN_SSID              "<network_SSID>"     // cannot be longer than 32 characters!
    #define     WLAN_PASS              "<network_password>" 
    #define     WLAN_SECURITY          <security_type>      // Security type can be WLAN_SEC_UNSEC, WLAN_SEC_WEP, WLAN_SEC_WPA or WLAN_SEC_WPA2

## Dependency Management
Newer versions of the Arduino IDE include library management. You'll need to 
download the following extras (via Sketch -> Include Library):

* Adafruit CC3000 Library
* OneWire (a contributed library)

# Hardware
This is written for an Arduino UNO hooked up to a CC3000 WiFI breakout board or 
shield. It also requires a OneWire thermometer (currently known to work with the 
DS18B20 sensor).

# Acknowledgements
Some of the source code here is mangled from Tony DiCola's [CloudThermometer](https://github.com/tdicola/CloudThermometer), 
also distributed under the MIT license.
