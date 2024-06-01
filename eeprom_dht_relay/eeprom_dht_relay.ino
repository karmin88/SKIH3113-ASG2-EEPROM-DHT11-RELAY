// Libraries
#include <ESP8266WiFi.h>
#include <EEPROM.h>
#include <ESP8266WebServer.h>
#include <DHT.h>

#define RELAY_PIN D1    // Define the relay pin
#define DHT_PIN D4      // Define the DHT sensor pin
#define DHT_TYPE DHT11  // Define the type of DHT sensor

ESP8266WebServer server(80);                     // Create a web server object on port 80
String ssid = "", password = "", deviceID = "";  // Variables to store WiFi credentials and device ID
bool relayState = false;                         // Variable to store relay state
DHT dht(DHT_PIN, DHT_TYPE);                      // Create a DHT sensor object
float temp = 0.0, hum = 0.0;                     // Variables to store real-time temperature and humidity readings
float tempOld = 0.0, humOld = 0.0;               // Variables to store saved temperature and humidity readings

void setup() {
  Serial.begin(115200);        // Initialize serial communication at 115200 baud
  pinMode(RELAY_PIN, OUTPUT);  // Set relay pin as output
  EEPROM.begin(512);           // Initialize EEPROM with a size of 512 bytes
  delay(100);
  readData();   // Read saved data from EEPROM
  dht.begin();  // Initialize DHT sensor

  Serial.println("Finding WiFi...");
  if (testWiFi()) {  // Try connecting to WiFi
    Serial.println("Web Type 0 - Wifi Mode");
    launchWeb(0);  // Launch web server in client mode
  } else {
    const char* ssidap = "nodemcu-AP";  // Define SSID for AP mode
    const char* passap = "";            // Define password for AP mode (empty)
    WiFi.mode(WIFI_AP);                 // Set WiFi mode to AP
    WiFi.softAP(ssidap, passap);        // Start AP with defined SSID and password
    Serial.println("Web Type 1 - AP Mode");
    Serial.print("Please connect to nodemcu-AP and access to http://");
    Serial.println(WiFi.softAPIP());
    launchWeb(1);  // Launch web server in AP mode
  }

  digitalWrite(RELAY_PIN, relayState ? HIGH : LOW);  // Apply initial relay state
}

void launchWeb(int webtype) {
  createWebServer(webtype);  // Create the web server based on the type (AP or client mode)
  server.begin();            // Start the web server
}

void loop() {
  server.handleClient();  // Handle incoming client requests
  readDHTSensor();        // Read the DHT sensor data
}

void readDHTSensor() {
  hum = dht.readHumidity();      // Read humidity from DHT sensor
  temp = dht.readTemperature();  // Read temperature from DHT sensor
}

void createWebServer(int webtype) {
  server.on("/", [webtype]() {
    String content = "<!DOCTYPE HTML>\r\n<html><head><style>";
    content += "body { font-family: Arial; text-align: center; }";
    content += "table { margin: 0 auto; }";
    content += "</style></head><body>";

    if (webtype == 0) {  // Client mode
      content += "<h1>WiFi Mode</h1><h2>Welcome to Your NodeMCU</h2>";
      content += "<p><b>Saved Settings:</b></p>";
      content += "<table border='1'><tr><td>SSID</td><td>" + ssid + "</td></tr>";
      content += "<tr><td>Device ID</td><td>" + deviceID + "</td></tr>";
      content += "<tr><td>Temperature</td><td>" + String(tempOld, 1) + " C</td></tr>";  // Display saved temperature with 1 decimal place
      content += "<tr><td>Humidity</td><td>" + String(humOld, 1) + " %</td></tr>";      // Display saved humidity with 1 decimal place
      content += "<tr><td>Relay State</td><td>" + String(relayState ? "On" : "Off") + "</td></tr></table>";
    } else {  // AP mode
      content += "<h1>Access Point Mode</h1><h2>Welcome to NodeMCU WiFi Configuration</h2>";
      content += "<p><b>Current Configuration:</b></p>";
      content += "<table border='1'><tr><td>SSID</td><td>" + ssid + "</td></tr>";
      content += "<tr><td>Password</td><td>" + password + "</td></tr>";
      content += "<tr><td>Device ID</td><td>" + deviceID + "</td></tr>";
      content += "<tr><td>Temperature</td><td>" + String(tempOld, 1) + " C</td></tr>";  // Display saved temperature with 1 decimal place
      content += "<tr><td>Humidity</td><td>" + String(humOld, 1) + " %</td></tr>";      // Display saved humidity with 1 decimal place
      content += "<tr><td>Relay State</td><td>" + String(relayState ? "On" : "Off") + "</td></tr></table>";

      content += "<p><b>New Configuration:</b></p>";
      content += "<form method='get' action='setting'><table border='1'>";
      content += "<tr><td>SSID</td><td><input name='SSID' length=32></td></tr>";
      content += "<tr><td>Password</td><td><input name='password' length=32></td></tr>";
      content += "<tr><td>Device ID</td><td><input name='deviceID' length=32></td></tr>";
      content += "<tr><td>Relay State</td><td>";
      content += "<input type='radio' name='relayState' value='on'" + String(relayState ? " checked" : "") + "> On";
      content += "<input type='radio' name='relayState' value='off'" + String(!relayState ? " checked" : "") + "> Off";
      content += "</td></tr>";
      content += "<tr><td>Temperature</td><td><span id='temp'>" + String(temp, 1) + " C</span></td></tr>";  // Real temperature
      content += "<tr><td>Humidity</td><td><span id='hum'>" + String(hum, 1) + " %</span></td></tr>";       // Real humidity
      content += "</table><br><input type='submit'></form>";
      content += "<script>";
      content += "setInterval(function() {";
      content += "  fetch('/getTempHum').then(function(response) { return response.json(); }).then(function(data) {";
      content += "    document.getElementById('temp').innerText = data.temp.toFixed(1) + ' C';";
      content += "    document.getElementById('hum').innerText = data.hum.toFixed(1) + ' %';";
      content += "  });";
      content += "}, 2000);";  // Update every 2 seconds
      content += "</script>";
    }
    content += "</body></html>";
    server.send(200, "text/html", content);
  });

  server.on("/setting", []() {
    String newSSID = server.arg("SSID");                    // Retrieve new SSID from the form
    String newPassword = server.arg("password");            // Retrieve new password from the form
    String newDeviceID = server.arg("deviceID");            // Retrieve new device ID from the form
    bool newRelayState = server.arg("relayState") == "on";  // Retrieve new relay state from the form
    float newTemp = temp;                                   // Use current temperature value
    float newHum = hum;                                     // Use current humidity value

    // Check if any of the fields are empty and send an error message if they are
    if (newSSID.length() == 0 || newPassword.length() == 0 || newDeviceID.length() == 0) {
      server.send(200, "text/html", "Error: SSID, Password, and Device ID cannot be empty. <a href='/'> Back </a>");
      return;
    }

    // Update the global variables with the new values
    ssid = newSSID;
    password = newPassword;
    deviceID = newDeviceID;
    relayState = newRelayState;
    tempOld = newTemp;
    humOld = newHum;

    // Save the new configuration to EEPROM
    writeData(ssid, password, deviceID, relayState, tempOld, humOld);

    // Create a response page to indicate the settings have been saved and the esp will reboot
    String content = "<!DOCTYPE HTML>\r\n<html><head><style>";
    content += "body { font-family: Arial; text-align: center; }";
    content += "</style></head><body>";
    content += "Success! Settings are saved!<br>";
    content += "Rebooting... Please wait for awhile...<br>";
    content += "</body></html>";

    // Send the response page to the client
    server.send(200, "text/html", content);

    // Delay for a short period to allow the client to see the message, then restart the esp
    delay(1000);
    ESP.restart();
  });

  server.on("/getTempHum", []() {  // Handle requests for temperature and humidity data
    String json = "{\"temp\":" + String(temp, 1) + ", \"hum\":" + String(hum, 1) + "}";
    server.send(200, "application/json", json);
  });
}

// Function to test WiFi connection
boolean testWiFi() {
  WiFi.begin(ssid.c_str(), password.c_str());  // Begin WiFi connection with stored credentials
  int c = 0;
  while (c < 10) {                        // Attempt to connect up to 10 times
    if (WiFi.status() == WL_CONNECTED) {  // Check if the WiFi is connected
      Serial.println(WiFi.status());
      Serial.print("Wifi ip: ");
      Serial.println(WiFi.localIP());
      return true;
    }
    delay(1000);  // Wait for 1 second before the next attempt
    Serial.print(".");
    c++;
  }
  Serial.println("");
  Serial.println("Connect timed out.");  // Print a timeout message
  return false;
}

// Function to write data to EEPROM
void writeData(String a, String b, String c, bool d, float e, float f) {
  Serial.println("Writing to EEPROM...");

  // Write SSID to EEPROM
  for (int i = 0; i < 20; i++) {
    if (i < a.length()) {
      EEPROM.write(i, a[i]);  // Write character of SSID to EEPROM
    } else {
      EEPROM.write(i, 0);  // Write null character to pad remaining space
    }
  }

  // Write Password to EEPROM
  for (int i = 20; i < 40; i++) {
    if (i - 20 < b.length()) {
      EEPROM.write(i, b[i - 20]);  // Write character of password to EEPROM
    } else {
      EEPROM.write(i, 0);  // Write null character to pad remaining space
    }
  }

  // Write Device ID to EEPROM
  for (int i = 40; i < 60; i++) {
    if (i - 40 < c.length()) {
      EEPROM.write(i, c[i - 40]);  // Write character of device ID to EEPROM
    } else {
      EEPROM.write(i, 0);  // Write null character to pad remaining space
    }
  }

  // Write relay state to EEPROM
  EEPROM.write(60, d);  // Write relay state to EEPROM

  // Write Temperature to EEPROM
  writeFloatToEEPROM(61, e);  // Write temperature to EEPROM starting at address 61

  // Write Humidity to EEPROM
  writeFloatToEEPROM(65, f);  // Write humidity to EEPROM starting at address 65

  EEPROM.commit();                     // Commit changes to EEPROM
  Serial.println("Write successful");  // Print message to indicate write was successful
}

void writeFloatToEEPROM(int address, float value) {
  // Cast the float value to a byte pointer
  byte* p = (byte*)(void*)&value;
  // Write each byte of the float value to EEPROM
  for (int i = 0; i < sizeof(value); i++) {
    EEPROM.write(address + i, *p++);  // Write byte to EEPROM
  }
  EEPROM.commit();  // Commit changes to EEPROM
}

void readFloatFromEEPROM(int address, float& value) {
  // Cast the byte pointer to a float pointer
  byte* p = (byte*)(void*)&value;
  // Read each byte of the float value from EEPROM
  for (int i = 0; i < sizeof(value); i++) {
    *p++ = EEPROM.read(address + i);  // Read byte from EEPROM
  }
}

void readData() {
  Serial.println("Reading from EEPROM...");  // Print message to indicate start of reading from EEPROM

  char ssidArr[21];      // Array to hold the SSID
  char passwordArr[21];  // Array to hold the Password
  char deviceIDArr[21];  // Array to hold the Device ID
  // Read SSID from EEPROM
  for (int i = 0; i < 20; i++) {
    ssidArr[i] = EEPROM.read(i);  // Read character from EEPROM
  }
  ssidArr[20] = '\0';      // Add null terminator
  ssid = String(ssidArr);  // Convert char array to String

  // Read Password from EEPROM
  for (int i = 20; i < 40; i++) {
    passwordArr[i - 20] = EEPROM.read(i);  // Read character from EEPROM
  }
  passwordArr[20] = '\0';          // Add null terminator
  password = String(passwordArr);  // Convert char array to String

  // Read Device ID from EEPROM
  for (int i = 40; i < 60; i++) {
    deviceIDArr[i - 40] = EEPROM.read(i);  // Read character from EEPROM
  }
  deviceIDArr[20] = '\0';          // Add null terminator
  deviceID = String(deviceIDArr);  // Convert char array to String

  // Read relay state from EEPROM
  relayState = EEPROM.read(60);  // Read relay state from EEPROM

  // Read Temperature from EEPROM
  readFloatFromEEPROM(61, tempOld);  // Read saved temperature from EEPROM

  // Read Humidity from EEPROM
  readFloatFromEEPROM(65, humOld);  // Read saved humidity from EEPROM

  // Print values read from EEPROM to serial monitor
  Serial.println("WiFi SSID from EEPROM: " + ssid);
  Serial.println("WiFi password from EEPROM: " + password);
  Serial.println("Device ID from EEPROM: " + deviceID);
  Serial.println("Relay state from EEPROM: " + String(relayState ? "On" : "Off"));
  Serial.print("Saved Temperature: ");
  Serial.print(tempOld);
  Serial.println(" C");
  Serial.print("Saved Humidity: ");
  Serial.print(humOld);
  Serial.println(" %");
  Serial.println("Reading successful...");  // Print message to indicate read was successful
}
