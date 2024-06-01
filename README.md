# SKIH3113 (ASG2) Sensor-Based System
**ESP8266 with EEPROM**
<br>
<br>This project is a sensor-based system utilizing an ESP8266 microcontroller, DHT11 sensor, and relay. The system is designed to read and display temperature and humidity data, as well as control a relay based on these readings. Additionally, it features EEPROM for storing Wi-Fi configuration data, device ID, temperature, humidity, and the last output status of the relay. 
<br><br>**_NOTE:_** The system operates in two modes: **Access Point (AP) mode** and **Wi-Fi mode**. 
<br><br>In Access Point (AP) mode, the ESP8266 creates its own Wi-Fi network, allowing users to connect and configure Wi-Fi credentials and other parameters via a web interface. Once the configuration is saved, the ESP8266 restarts and attempts to connect to the specified Wi-Fi network using the stored credentials. 
<br><br>In Wi-Fi mode, the ESP8266 connects to the configured Wi-Fi network and displays the saved settings, including the device ID, temperature, humidity, and relay state.
