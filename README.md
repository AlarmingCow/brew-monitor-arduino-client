Brew Monitor Arduino Client
===========================

This is the Arduino client for Brew Monitor. It's WiFi enabled and periodically sends updates from a digital temperature probe to a server over a RESTful web interface.

Currently this thing is pretty dumb: it POSTs an index request to an Elasticsearch node running at `192.168.1.108:9200`. If that's your setup, congrats, you're good to go! If not, you'll have to dig into my nasty Wiring code.

# Usage
Deploy using the [Arduino IDE](http://arduino.cc/en/main/software).

## WiFi Setup
You'll have to configure your wireless network. Add the following lines in the header for `TempGauge.ino`:

    #define     WLAN_SSID              "<network_SSID>"     // cannot be longer than 32 characters!
    #define     WLAN_PASS              "<network_password>" 
    #define     WLAN_SECURITY          <security_type>      // Security type can be WLAN_SEC_UNSEC, WLAN_SEC_WEP, WLAN_SEC_WPA or WLAN_SEC_WPA2

Maybe don't commit that stuff though. TODO: make configurable.

# Hardware
This is written for an Arduino UNO hooked up to a CC3000 WiFI breakout board or shield. It also requires a OneWire thermometer (currently known to work with the DS18B20 sensor).
