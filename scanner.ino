// Libraries needed for the file
#include <WiFi.h>
#include <HTTPClient.h>
#include <esp32ARP.h>

// Wifi credentials
const char* ssid = "Your wifi";
const char* password = "Your Password";

// ARP handler used to discover devices on the local network
esp32ARP arp;

// Webserver url
const char* SERVER_URL = "DASHBOARD LINK";

// Defining the max devices, ports, and events the device will store 
#define MAX_DEVICES 20
#define MAX_PORTS 10
#define MAX_EVENTS 30

// Variable that allows the device to scan every 30 sec
const unsigned long SCAN_INTERVAL = 30000;

// Maximum amount of time to wait for a device to respond to a ping.
const uint16_t PING_TIMEOUT = 250;

// Stores the IP address assigned to the ESP32 scanner
IPAddress localIP;

// Maximum number of IPv4 hosts that can exist in a typical /24 network
#define MAX_HOSTS 254

//Common Ports for the device to scan
const uint16_t COMMON_PORTS[] = { 
  21,       // FTP
  22,       // SSH
  23,       // Telnet
  53,       // DNS
  80,       // HTTP
  443,      // HTTPS
  1883,     // MQTT
  8080      // HTTP alternate
};

// Calculate the number of ports in the COMMON_PORTS array.
const int commonPortCount = sizeof(COMMON_PORTS) / sizeof(COMMON_PORTS[0]);

// Device Structure
struct NetworkDevice {
  IPAddress ip;
  String hostname;
  String mac;
  String vendor;

  bool online;

  String deviceType;

  uint16_t openPorts[MAX_PORTS];
  int portCount;

  int fingerprintScore;

  unsigned long firstSeen;
  unsigned long lastSeen;
};

// Security Event
struct SecurityEvent{

  String type;
  String ip;
  String description;
  unsigned long timestamp;

};

// Global Storage 
NetworkDevice currentDevices[MAX_DEVICES];

NetworkDevice previousDevices[MAX_DEVICES];

SecurityEvent events[MAX_EVENTS];

int currentDeviceCount = 0;
int previousDeviceCount = 0;
int eventCount = 0;

unsigned long lastScan = 0;

// Function Declearation
void connectToWiFi();
void  printNetworkInformation();
void runNetworkScan();
void clearCurrentScan();
void discoverDevices();
bool checkHost(IPAddress ip);
void collectDeviceInfo(NetworkDevice &device);
void detectHostname(NetworkDevice &device);
void identifyVendor(NetworkDevice &device);
void detectService(NetworkDevice &device);
bool checkPort(IPAddress ip, uint16_t port);
void fingerprintDevice(NetworkDevice &device);
void compareScans();
void generateNewDeviceEvent(NetworkDevice &device);
void generateOfflineDeviceEvent(NetworkDevice &device);
void generatePortChangeEvent(NetworkDevice &device,uint16_t port,bool opened);
void addEvent(String type,String ip,String description);
void printDevices();
void printEvents();
void sendResultsToServer();
void saveCurrentAsPrevious();
IPAddress calculateNetworkAddress();
void printIPAddress(const char* label,IPAddress address);

// Setup
void setup() {

  Serial.begin(115200);
  // Gives the ESP32 a moment to initialize before starting the scanner
  delay(1000);
  // Display a startup message
  Serial.println("ESP32 Network Watcher V1");
  // Connects to the Wifi
  connectToWiFi();

  // Confirms the esp32 is connected to the wifi and stops it if the esp did not connect
  if(WiFi.status() != WL_CONNECTED){
    Serial.println("[!] Wi-Fi connection failed.");
    return;
  };

  // Store the ESP32's local IP address for network scanning
  localIP = WiFi.localIP();

  // Initialize the ARP functionality used for network device discovery
  if (arp.init() == ERR_OK) {
    Serial.println("ARP initialized");
  } else {
    Serial.println("ARP initialization failed");
  };

  // Display information about the connected network
  printNetworkInformation();
  
  // Indicate that the scanner has completed initialization
  Serial.println("Scanner Ready");

  // Short delay before starting the first network scan
  delay(2000);

  //  Perform the initial network scan
  runNetworkScan();

  // Record when the first scan was completed
  lastScan = millis();
}

// Loop
void loop() {
  // Confirming the Wifi is connected
  if(WiFi.status() != WL_CONNECTED){

    Serial.println("[!] Wi-Fi disconnected.");

    // Connect the device to the network
    connectToWiFi();

    return;
  }

  // Check whether the configured scan interval has elapsed
  if (millis() - lastScan >=SCAN_INTERVAL) {
    // Update the timestamp before starting the scan
    lastScan = millis();
    // Run another network scan.
    runNetworkScan();
  }

}

// Function to connect esp32 to wifi
void connectToWiFi(){
  Serial.println("Wifi connected");
  // Set the ESP32 to station mode so it can connect to an existing Wi-Fi network
  WiFi.mode(WIFI_STA);
  // Begin the connection using the configured Wi-Fi credentials
  WiFi.begin(ssid,password);
  // Tracks the attempts to connect to the wifi
  int attempts = 0;
  // Wait for the ESP32 to connect, with a maximum of 30 attempts
  while(WiFi.status() != WL_CONNECTED && attempts < 30){
    //  Wait 500 milliseconds between connection attempts
    delay(500);
    
    Serial.print(".");
    // Add an attempt
    attempts++;
  }
  // Check whether the ESP32 successfully connected to Wi-Fi
  if(WiFi.status() == WL_CONNECTED){

    Serial.println("Wifi Connected");
    Serial.print("SSID: ");
    Serial.println(ssid);
    // Display the IP address assigned to the ESP32
    Serial.print("IP: ");
    Serial.println(WiFi.localIP());
    // Display the Wi-Fi signal strength in dBm
    Serial.print("RSSI: ");
    Serial.println(WiFi.RSSI());
    
  }else{
    // Report that the ESP32 was unable to connect after the maximum attempts
    Serial.println("Failed to connect to Wi-Fi.");
  }
}
// Network information
void printNetworkInformation(){
  Serial.println("Network Information");

  printIPAddress("IP Address: ",WiFi.localIP());
  printIPAddress("Gateway: ",WiFi.gatewayIP());
  printIPAddress("Subnet: ",WiFi.subnetMask());
  printIPAddress("DNS: ",WiFi.dnsIP());
  
  IPAddress network = calculateNetworkAddress();

  printIPAddress("Network: ",network);
}
// Prints the IP address
void printIPAddress(const char* label, IPAddress address){

  Serial.print(label);

  Serial.println(address);
}
// Calculate the network address from the ESP32's IP address and subnet mask
IPAddress calculateNetworkAddress(){
  // Get the IP address currently assigned to the ESP32
  IPAddress ip = WiFi.localIP();
  // Get the subnet mask of the connected network
  IPAddress subnet = WiFi.subnetMask();
  // Create an IPAddress object to store the calculated network address
  IPAddress network;
  // Calculate the network address by applying a bitwise AND
  // between each IP address octet and the subnet mask.
  for(int i = 0;i < 4;i++){

    network[i] = ip[i] & subnet[i];
  }
  // Return the calculated network address
  return network;
}
// Runs the network scan
void runNetworkScan(){
  Serial.println("Starting Scan");

  // Clears any scans
  clearCurrentScan();

  // Discover Host

  Serial.println("Discovering active hosts...");
  discoverDevices();
  Serial.print("Host Discoverd");

  Serial.println(currentDeviceCount);
  // Finger printing devices
  Serial.println("Fingerprinting devices...");
  for(int i = 0;i < currentDeviceCount;i++){
    collectDeviceInfo(currentDevices[i]);

  }

  //Compare devices
  Serial.println("Comparing with previous scan...");
  compareScans();

  // Scan results
  Serial.print("Scan completed");
  printDevices();

  // Secuirty Events
  Serial.println("Security Events");
  printEvents();

  // Send to ESP32 #3
  Serial.println("Sending to ESP32");
  sendResultsToServer();

  // Save baseline
  saveCurrentAsPrevious();
  Serial.println("Scan Done");
}
// Resets the scan
void clearCurrentScan(){
  // Reset the number of devices found in the current scan
  currentDeviceCount = 0;
  // Reset each device entry in the device list
  for(int i = 0;i < MAX_DEVICES;i++){
    // Mark the device as offline until it is discovered again
    currentDevices[i].online = false;
    // Clear the list of open ports from the previous scan
    currentDevices[i].portCount = 0;
    // Reset the device fingerprint score
    currentDevices[i].fingerprintScore = 0;
  }
}
// Discovering Devices
void discoverDevices(){

  IPAddress ip = WiFi.localIP();
  IPAddress subnet = WiFi.subnetMask();
  IPAddress network = calculateNetworkAddress();

  // Actual Network
  Serial.print("Network: ");
  Serial.println(network);

  // Subnet
  Serial.print("Subnet: ");
  Serial.println(subnet);

  // Most home networks will be /24.
  // We calculate the network address instead of assuming
  // that the network always starts at x.x.x.0.

  if(subnet[0] != 255 || subnet[1] != 255 || subnet[2] != 255){

    Serial.println("Warning: unusual subnet detected.");
    Serial.println("V2 currently scans the final octet");

  }

  // Scan host
  for(int host = 1; host < 255; host++){
    // Don't scan ourselves
    if(host == localIP[3]){
      continue;
    }

    IPAddress target(network[0], network[1], network[2], host);

    Serial.print("[Ping]");
    Serial.print(target);

    if(checkHost(target)){

      Serial.print("Online");

      if(currentDeviceCount >= MAX_DEVICES){

        Serial.print("Mac Devices reached");
        return;
      }

      NetworkDevice &device = currentDevices[currentDeviceCount];

      device.ip = target;

      device.online = true;

      // Checks wheither the device has been scanned before or not

      bool existed = false;

      for(int i = 0; i < previousDeviceCount; i++){

        if(previousDevices[i].ip == target){

          existed = true;

          device.firstSeen = previousDevices[i].firstSeen;
          break;
        }
      }

      if(!existed){
        device.firstSeen = millis();
      }

      device.lastSeen = millis();

      device.hostname = "";
      device.mac = "UNKNOWN";
      device.vendor = "UNKNOWN";
      device.deviceType = "UNKNOWN";

      device.portCount = 0;
      device.fingerprintScore = 0;

      currentDeviceCount++;
      Serial.print("Device Found");
      Serial.println(target);
    }else{

      Serial.print("Offline");

    }
    delay(5);
  }
}
// Checks the host
bool checkHost(IPAddress ip) {

  Serial.print("[ARP] ");
  Serial.print(ip);
  Serial.print(" -> ");

  // Ask the network who owns this IP
  int requestResult = arp.sendRequest(ip.toString());

  if (requestResult != ERR_OK) {
    Serial.println("ARP request failed");
    return false;
  }

  // Give the device a moment to respond
  delay(100);

  // Look for the device in the ARP table
  mac_addr_t mac;

  int result = arp.lookupEntry(ip.toString(), mac);

  if (result == ERR_OK) {

    Serial.print("FOUND | MAC: ");
    printlnMacAddr(mac);

    return true;
  }

  Serial.println("NO RESPONSE");

  return false;
}
// Collects the device info
void collectDeviceInfo(NetworkDevice &device){

  Serial.print("Device: ");
  Serial.println(device.ip);

  // Host name
  detectHostname(device);

  // Mac / Vendor
  identifyVendor(device);

  // TCP Service
  detectService(device);

  // Fingerprint
  fingerprintDevice(device);
}
// Detects Hostname
void detectHostname(NetworkDevice &device){
  device.hostname = "UNKNOWN";


  Serial.print( "Hostname: ");

  Serial.println(device.hostname);
}
// Mac and Vendor detection
void identifyVendor(NetworkDevice &device){

  device.mac = "UNKNOWN";
  device.vendor = "UNKNOWN";
  
  Serial.print("MAC: ");
  Serial.println(device.mac);

  Serial.print("Vendor: ");
  Serial.println(device.vendor);
}
// Detects Services
void detectService(NetworkDevice &device){

  Serial.println("Services:");
  
  for(int i = 0; i < commonPortCount; i++){

    if(device.portCount >= MAX_PORTS){

      break;
    }

    uint16_t port = COMMON_PORTS[i];
    if(checkPort(device.ip,port)){

      device.openPorts[device.portCount] = port;
      device.portCount++;
      Serial.print(port);
    }
    if(device.portCount == 0){
      Serial.println("No common ports detected.");
    }

  }
}
// Port Check
bool checkPort(IPAddress ip, uint16_t port){
  WiFiClient client;
  client.setTimeout(250);

  bool connected = client.connect(ip,port);

  if(connected){

    client.stop();
    return true;
  }

  client.stop();
  return false;
}
// Device Fingerprinting
void fingerprintDevice(NetworkDevice &device){

  int score = 0;
  
  bool hasFTP = false;
  bool hasSSH = false;
  bool hasTelnet = false;
  bool hasDNS = false;
  bool hasHTTP = false;
  bool hasHTTPS = false;
  bool hasMQTT = false;
  bool hasHTTPAlt = false;

  //Examine open ports
  for(int i = 0; i < device.portCount; i++){
    uint16_t port = device.openPorts[i];

    if(port == 21){
      hasFTP = true;

      score += 10;
    }

    if( port == 22){
      hasSSH = true;

      score += 15;
    }

    if(port == 23){
      hasTelnet = true;

      score += 25;
    }

    if(port == 53){
      hasDNS = true;

      score += 10;
    }

    if(port == 80){
      hasHTTP = true;

      score += 20;
    }

    if(port == 443){
      hasHTTPS = true;

      score += 20;
    }

    if(port == 1883){
      hasMQTT = true;

      score += 25;
    }

    if(port == 8080){
      hasHTTPAlt = true;

      score += 15;
    }
  }

  if(hasMQTT && (hasHTTP || hasHTTPAlt)){
    device.deviceType = "IoT Device";
  } else if(hasTelnet){
    device.deviceType = "Network Device";
  }else if (hasSSH && (hasHTTP || hasHTTPS)){
    device.deviceType = "Computer / Server";
  }else if(hasHTTP || hasHTTPS || hasHTTPAlt){
    device.deviceType = "Network Service";
  }else if(hasDNS){
    device.deviceType = "DNS Server";
  }else{
    device.deviceType = "UNKNOWN";
  }

  device.fingerprintScore = score;
  Serial.print("Type: ");
  Serial.println(device.deviceType);

  Serial.print("Fingerprint Score: ");
  Serial.println(device.fingerprintScore);
}
//Compare scans
void compareScans(){
  // First Scan
  if(previousDeviceCount == 0){
    Serial.println("No previous baseline.");
    Serial.println("Current scan becomes baseline.");

  }
  //New Scan
  for(int i = 0; i < currentDeviceCount; i++){
    
    bool foundBefore = false;

    for(int j = 0; j < previousDeviceCount; j++){

      if(currentDevices[i].ip == previousDevices[j].ip){
        foundBefore = true;

        break;
      }
    }
    if(!foundBefore){
      generateNewDeviceEvent(currentDevices[i]);

    }
  }
  //Offline devices
  for(int i = 0; i < previousDeviceCount; i++){

    bool foundNow = false;

    for(int j = 0; j < currentDeviceCount; j++){

      if(previousDevices[i].ip == currentDevices[j].ip){

        foundNow = true;

        break;
      }
    }

    if(!foundNow){
      generateOfflineDeviceEvent(previousDevices[i]);
    }
  }

  // Port Change
  for(int i = 0; i < currentDeviceCount; i++){

    for(int j = 0; j < previousDeviceCount; j++){
      if(currentDevices[i].ip == previousDevices[j].ip){

        // Newly open port
        for(int p = 0; p < currentDevices[i].portCount; p++){

          uint16_t port = currentDevices[i].openPorts[p];
          bool existed = false;

          for(int old = 0; old < previousDevices[j].portCount; old++){

            if(previousDevices[j].openPorts[old] == port){

              existed = true;
              break;
            }
          }
          if(!existed){
            generatePortChangeEvent(currentDevices[i], port, true);
          }
        }
        // Closed ports
        for(int old = 0; old < previousDevices[j].portCount; old++){
          uint16_t port = previousDevices[j].openPorts[old];

          bool stillOpen = false;

          for(int current = 0; current < currentDevices[i].portCount; current++){
            if(currentDevices[i].openPorts[current] == port){
              stillOpen = true;

              break;
            }
          }
          if(!stillOpen){
            generatePortChangeEvent(currentDevices[i],port,false);
          }
        }
      }
    }
  }
}
// New Device Event
void generateNewDeviceEvent(NetworkDevice &device){
  addEvent("DEVICE_NEW",device.ip.toString(),"New device detected");
}

// Offline Device Event
void generateOfflineDeviceEvent(NetworkDevice &device){
  addEvent("DEVICE_OFFLINE",device.ip.toString(),"Previously seen device is offline");
}
//Port Change event
void generatePortChangeEvent(NetworkDevice &device, uint16_t port, bool opened){
  String description;


  if (opened) {

    description = "New service detected on port " + String(port);

  }else {

    description = "Previously open service closed on port " + String(port);
  }

  addEvent(
    opened ? "PORT_OPENED" : "PORT_CLOSED",
    device.ip.toString(),
    description
  );
}
// Add Secuiryt Event
void addEvent(String type, String ip, String description){
  // Event Buffer full
  if(eventCount >= MAX_EVENTS){
    for(int i = 1; i < MAX_EVENTS; i++){
      events[i - 1] = events[i];
    }
    eventCount = MAX_EVENTS - 1;
  }

  events[eventCount].type = type;


  events[eventCount].ip = ip;


  events[eventCount].description = description;


  events[eventCount].timestamp = millis();


  eventCount++;

  Serial.println("Security Event");

  Serial.print( "Type: ");
  Serial.println( type);

  Serial.print("IP: ");

  Serial.println(ip);


  Serial.print("Description: ");
  Serial.println(description);
}
//Print Devices
void printDevices(){
  Serial.println("Network Devices");
  if(currentDeviceCount == 0){

    Serial.println("No devices found.");
    return;
  }
  
  for(int i = 0; i < currentDeviceCount; i++){

    NetworkDevice &device = currentDevices[i];
    Serial.print("Device #");
    Serial.println(i + 1);

    Serial.print("IP: ");
    Serial.println(device.ip);

    Serial.print("Hostname: ");
    Serial.println(device.hostname);

    Serial.print( "MAC: ");
    Serial.println(device.mac);

    Serial.print("Vendor: ");
    Serial.println(device.vendor);

    Serial.print("Type: ");
    Serial.println(device.deviceType);

    Serial.print("Ports: ");

    if(device.portCount == 0){
      Serial.println("None");
    }else{
      for(int p = 0; p < device.portCount; p++){
        Serial.print( device.openPorts[p]);

        if(p < device.portCount - 1){
          Serial.print(", ");
        }
      }
    }
    Serial.print("Fingerprint: ");
    Serial.println(device.fingerprintScore);


    Serial.print("First Seen: ");
    Serial.println(device.firstSeen);


    Serial.print("Last Seen: ");
    Serial.println(device.lastSeen);
  }
}
// Print Events
void printEvents(){

  Serial.print("Security Events");
  if(eventCount == 0){
    Serial.println("No events.");

    return;
  }
  for(int i = 0; i < eventCount; i++){
    Serial.println();

    Serial.print("[");
    Serial.print(events[i].type);
    Serial.println("]");


    Serial.print("IP: ");
    Serial.println(events[i].ip);

    Serial.print("Event: ");
    Serial.println(events[i].description);


    Serial.print("Timestamp: ");
    Serial.println(events[i].timestamp);
  }
}
// Send results to esp #3 webserver
void sendResultsToServer(){
  // Check that the scanner is connected to Wi-Fi before sending data
  if(WiFi.status() != WL_CONNECTED){
    Serial.println("Cannot send Results");
    return;
  }
  // Create an HTTP client to communicate with the web server
  HTTPClient http;
  // Set the destination URL for the scan results
  http.begin(SERVER_URL);
  // Specify that the request body contains JSON data
  http.addHeader("Content-Type", "application/json");

  //Build Json
  String json ="{";

  // Scanner
  json += "\"scanner\":\"ESP32-01\",";

  // Scanner IP
  json += "\"scannerIP\":\"";
  json += WiFi.localIP().toString();
  json += "\",";

  // Network
  json += "\"network\":\"";
  json += calculateNetworkAddress().toString();
  json +=  "\",";

  // Device Count
  json += "\"deviceCount\":";
  json += String(currentDeviceCount);
  json += ",";

  // Devices
  json += "\"devices\":[";
  // Specify that the request body contains JSON data
  for(int i = 0; i < currentDeviceCount; i++){
    NetworkDevice &device = currentDevices[i];

    json += "{";

    // IP
    json += "\"ip\":\"";
    json += device.ip.toString();
    json += "\",";

    //Hostname
    json += "\"hostname\":\"";
    json += device.hostname;
    json += "\",";

    // MAC
    json += "\"mac\":\"";
    json += device.mac;
    json += "\",";

    // Vendor
    json += "\"vendor\":\"";
    json += device.vendor;
    json += "\",";

    // Device Type
    json += "\"type\":\"";
    json += device.deviceType;
    json += "\",";

    // Fingerprint
    json += "\"fingerprint\":";
    json += String(device.fingerprintScore);
    json += ",";

    // First Seen 
    json += "\"firstSeen\":";
    json += String(device.firstSeen);
    json += ",";

    // Last Seen
    json += "\"lastSeen\":";
    json += String(device.lastSeen);
    json += ",";

    // Ports
    json += "\"ports\":[";
    // Add each detected open port to the JSON array
    for(int p = 0;p < device.portCount;p++){
      json += String(device.openPorts[p]);
      // Add a comma between port entries, but not after the final entry
      if(p < device.portCount - 1){
        json += ",";
      }
    }
    // Close the ports array and the current device object
    json +=  "]";
    json += "}";
    // Separate device objects with commas when more devices remain
    if(i < currentDeviceCount - 1){
      json += ",";
    }
  }
  // Close the devices array and add a comma before the next JSON field
  json += "],";

  //Security Events
  json +=  "\"events\":[";
  // Loop through each stored security event
  for( int i = 0; i < eventCount; i++){
    // Start a JSON object for the current event
    json += "{";
    // Add the event type, such as a newly discovered device
    json += "\"type\":\"";
    json += events[i].type;
    json += "\",";
    // Add the IP address associated with the event
    json += "\"ip\":\"";
    json += events[i].ip;
    json += "\",";
    // Include a description of what happened
    json += "\"description\":\"";
    json += events[i].description;
    json += "\",";
    // Add the timestamp associated with the event
    json +=  "\"timestamp\":";
    json += String(events[i].timestamp);
    // Close the current event object
    json += "}";
    // Separate event objects with commas when more events remain
    if( i < eventCount - 1){
      json += ",";
    }
  }
  // Close the events array and the complete JSON object
  json += "]";
  json += "}";

  // Debug JSON
  Serial.println();
  Serial.println("Sending JSON:");
  Serial.println(json);

  // Send the JSON payload to the web server using an HTTP POST request
  int responseCode = http.POST(json);
  // Check whether the server returned a valid HTTP response code
  if(responseCode > 0){
    Serial.print("Server response: ");
    Serial.println(responseCode);
    // Retrieve the response body returned by the server
    String response = http.getString();
    
    Serial.println("Server body:");
    Serial.println(response);
  }else{
    Serial.print("HTTP error: ");

    Serial.println(responseCode);
  }
  // Release the HTTP client's resources after the request completes
  http.end();
}
// Save the Current Scan
void saveCurrentAsPrevious(){
  // Store the number of devices detected in the current scan
  previousDeviceCount = currentDeviceCount;
  // Copy each detected device into the previous scan list
  for(int i = 0; i < currentDeviceCount; i++){
    // Save the current device information for future comparisons
    previousDevices[i] = currentDevices[i];
  }

  Serial.print("Baseline updated. Devices: ");
  Serial.println(previousDeviceCount);
}
