#include <WiFi.h>
#include <WebServer.h>

// Wifi credentials
const char* WIFI_SSID = "Your WiFi";
const char* WIFI_PASSWORD = "Your Password";

// Webserver
WebServer server(80);

// Configuration
#define MAX_DEVICES 20
#define MAX_EVENTS 50

// Scanner Data
String scannerIP = "";
String scannerNetwork = "";

int deviceCount = 0;

unsigned long lastScannerUpdate = 0;

// Honeypot Data currently not configured
String honeypotIP = "";

unsigned long lastHoneypotUpdate = 0;

int honeypotEventCount = 0;

// Device Data
struct Device{
  String ip;
  String hostname;
  String mac;
  String vendor;
  String type;

  int fingerprint;

  unsigned long firstSeen;
  unsigned long lastSeen;

  String ports;
};

Device devices[MAX_DEVICES];

//  Secuirty Event 
struct SecurityEvent{
  String source;
  String type;

  String sourceIP;

  int port;

  String service;

  String details;

  unsigned long timestamp;
};

SecurityEvent events[MAX_EVENTS];

int eventCount = 0;

// Setup function
void setup() {
  Serial.begin(115200);

  delay(1000);
  Serial.println("DASHBOARD");

  connectWiFi();

  setupRoutes();

  server.begin();
  Serial.println();
  Serial.println("Web server started");

  Serial.print("Dashboard: http://");
  Serial.println(WiFi.localIP());

}

// Loop setup
void loop() {
  server.handleClient();

  updateDeviceStatus();

}

// Connect to Wifi
void connectWiFi(){
   Serial.print("Connecting to WiFi");

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  while (WiFi.status() != WL_CONNECTED) {

    delay(500);

    Serial.print(".");
  }

  Serial.println();

  Serial.println("WiFi connected");

  Serial.print("Dashboard IP: ");
  Serial.println(WiFi.localIP());

  Serial.print("Gateway: ");
  Serial.println(WiFi.gatewayIP());

  Serial.print("Subnet: ");
  Serial.println(WiFi.subnetMask());

  Serial.print("MAC: ");
  Serial.println(WiFi.macAddress());
}

// Dashboard 
void handleDashboard(){
  String html = R"rawliteral(

  <!DOCTYPE html>

  <html>

  <head>

  <meta name="viewport"
  content="width=device-width, initial-scale=1">

  <title>ESP32 NETWATCH</title>

  <style>

  body {

      margin: 0;

      background: #0b0f14;

      color: #ffffff;

      font-family: Arial, sans-serif;

  }

  .header {

      background: #111820;

      padding: 20px;

      border-bottom: 1px solid #26313d;

  }

  .header h1 {

      margin: 0;

      font-size: 26px;

  }

  .header p {

      color: #8b98a7;

      margin-bottom: 0;

  }

  .container {

      padding: 20px;

      max-width: 1200px;

      margin: auto;

  }

  .cards {

      display: grid;

      grid-template-columns:
      repeat(auto-fit, minmax(200px, 1fr));

      gap: 15px;

  }

  .card {

      background: #111820;

      border: 1px solid #26313d;

      border-radius: 10px;

      padding: 20px;

  }

  .card h3 {

      margin-top: 0;

      color: #8b98a7;

  }

  .value {

      font-size: 30px;

      font-weight: bold;

  }

  .online {

      color: #32d583;

  }

  .warning {

      color: #ffb020;

  }

  .danger {

      color: #ff5c5c;

  }

  .section {

      margin-top: 25px;

  }

  .section h2 {

      font-size: 20px;

  }

  table {

      width: 100%;

      border-collapse: collapse;

  }

  th, td {

      padding: 12px;

      text-align: left;

      border-bottom:
      1px solid #26313d;

  }

  th {

      color: #8b98a7;

  }

  .event {

      background: #111820;

      border-left: 4px solid #ff5c5c;

      padding: 15px;

      margin-bottom: 10px;

      border-radius: 5px;

  }

  .small {

      color: #8b98a7;

      font-size: 13px;

  }

  </style>

  </head>

  <body>

  <div class="header">

  <h1>ESP32 NETWATCH</h1>

  <p>Distributed Network Security Monitor</p>

  </div>

  <div class="container">

  <div class="cards">

  <div class="card">

  <h3>SCANNER</h3>

  <div id="scannerStatus"
  class="value warning">

  OFFLINE

  </div>

  </div>

  <div class="card">

  <h3>HONEYPOT</h3>

  <div id="honeypotStatus"
  class="value warning">

  OFFLINE

  </div>

  </div>

  <div class="card">

  <h3>DEVICES</h3>

  <div id="deviceCount"
  class="value">

  0

  </div>

  </div>

  <div class="card">

  <h3>SECURITY EVENTS</h3>

  <div id="eventCount"
  class="value">

  0

  </div>

  </div>

  </div>


  <div class="section">

  <h2>Network Devices</h2>

  <div class="card">

  <table>

  <thead>

  <tr>

  <th>IP</th>

  <th>Hostname</th>

  <th>MAC</th>

  <th>Type</th>

  <th>Ports</th>

  </tr>

  </thead>

  <tbody id="devices">

  </tbody>

  </table>

  </div>

  </div>


  <div class="section">

  <h2>Security Events</h2>

  <div id="events">

  </div>

  </div>

  </div>


  <script>

  async function updateDashboard() {

      try {

          const response = await fetch('/api/data');

          const data = await response.json();

          updateStatus(data);

          updateDevices(data.devices);

          updateEvents(data.events);

      }

      catch(error) {

          console.log(error);

      }

  }


  function updateStatus(data) {

      const scanner =
      document.getElementById(
          'scannerStatus'
      );

      const honeypot =
      document.getElementById(
          'honeypotStatus'
      );

      const deviceCount =
      document.getElementById(
          'deviceCount'
      );

      const eventCount =
      document.getElementById(
          'eventCount'
      );


      const now = Date.now();


      if ((now - data.scannerLastSeen)
          < 90000) {

          scanner.innerText = "ONLINE";

          scanner.className =
          "value online";

      }

      else {

          scanner.innerText = "OFFLINE";

          scanner.className = "value danger";

      }


      if ((now - data.honeypotLastSeen)
          < 90000) {

          honeypot.innerText = "ONLINE";

          honeypot.className = "value online";

      }

      else {

          honeypot.innerText = "OFFLINE";

          honeypot.className = "value danger";

      }


      deviceCount.innerText = data.devices.length;

      eventCount.innerText = data.events.length;if (data.scannerAge < 90000) {

    scanner.innerText = "ONLINE";

    scanner.className =
    "value online";

      }else {

        scanner.innerText = "OFFLINE";

        scanner.className = "value danger";

      }


      if (data.honeypotAge < 90000) {

        honeypot.innerText = "ONLINE";

        honeypot.className = "value online";

      }else {

          honeypot.innerText = "OFFLINE";

          honeypot.className =
          "value danger";

      }

  }


  function updateDevices(devices) {

      const table = document.getElementById('devices');

      table.innerHTML = "";


      devices.forEach(device => {

          const row =
          document.createElement('tr');

          row.innerHTML = `

          <td>${device.ip}</td>

          <td>${device.hostname}</td>

          <td>${device.mac}</td>

          <td>${device.type}</td>

          <td>${device.ports}</td>

          `;

          table.appendChild(row);

      });

  }


  function updateEvents(events) {

      const container = document.getElementById( 'events');

      container.innerHTML = "";


      events.slice().reverse()
      .forEach(event => {

          const div = document.createElement('div');

          div.className = "event";

          div.innerHTML = `

          <strong>
          ${event.type}
          </strong>

          <br>

          ${event.service}

          : ${event.port}

          <br>

          Source:
          ${event.sourceIP}

          <br>

          <span class="small">

          ${event.details}

          </span>

          `;

          container.appendChild(div);

      });

  }


  updateDashboard();

  setInterval(updateDashboard,5000);

  </script>

  </body>

  </html>

  )rawliteral";

  server.send(
    200,
    "text/html",
    html
  );
}

// Handles API status
void handleStatus(){
  String json = "{";

  json += "\"server\":\"ONLINE\",";

  json += "\"ip\":\"";
  json += WiFi.localIP().toString();
  json += "\",";

  json += "\"uptime\":";
  json += String(millis());

  json += "}";

  server.send( 200, "application/json", json);
}

// Recieve Scanner Data
void handleScan(){
  String body =
  server.arg("plain");

  Serial.println("SCAN DATA RECEIVED");

  Serial.println(body);

  parseScannerData(body);

  server.send( 200, "application/json", "{\"status\":\"received\"}");
}

// Handle Honypot
void handleHoneypot(){
  String body =
  server.arg("plain");
  Serial.println("HONEYPOT EVENT RECEIVED");
  Serial.println(body);
  parseHoneypotData(body);
  server.send( 200, "application/json", "{\"status\":\"received\"}");
}

// Parse Scanner Data
void parseScannerData(String body){

  // Scanner information
  scannerIP = extractString(body, "scannerIP");

  scannerNetwork = extractString(body, "network");

  // Number of devices reported by scanner
  int reportedDeviceCount =
    extractNumber(body, "deviceCount");

  // Reset current dashboard devices
  deviceCount = 0;

  for(int i = 0; i < MAX_DEVICES; i++){

    devices[i].ip = "";
    devices[i].hostname = "";
    devices[i].mac = "";
    devices[i].vendor = "";
    devices[i].type = "";
    devices[i].ports = "";
    devices[i].fingerprint = 0;
    devices[i].firstSeen = 0;
    devices[i].lastSeen = 0;
  }

  // Find the devices array
  String devicesKey = "\"devices\":[";

  int devicesStart = body.indexOf(devicesKey);

  if(devicesStart == -1){

    Serial.println("ERROR: Devices array not found.");

    lastScannerUpdate = millis();

    return;
  }

  devicesStart += devicesKey.length();

  // Find the end of the devices array
  int devicesEnd =
    body.indexOf("],\"events\"", devicesStart);

  if(devicesEnd == -1){

    Serial.println("ERROR: End of devices array not found.");

    lastScannerUpdate = millis();

    return;
  }

  // Extract only the devices section
  String devicesJSON =
    body.substring(devicesStart, devicesEnd + 1);

  int searchPosition = 0;

  // Parse each device
  while(
    searchPosition < devicesJSON.length() &&
    deviceCount < MAX_DEVICES
  ){

    int objectStart =
      devicesJSON.indexOf("{", searchPosition);

    if(objectStart == -1){
      break;
    }

    int objectEnd =
      devicesJSON.indexOf("}", objectStart);

    if(objectEnd == -1){
      break;
    }

    String deviceJSON =
      devicesJSON.substring(
        objectStart,
        objectEnd + 1
      );

    // IP
    devices[deviceCount].ip = extractString(deviceJSON, "ip");

    // Hostname
    devices[deviceCount].hostname = extractString(deviceJSON, "hostname");

    // MAC
    devices[deviceCount].mac = extractString(deviceJSON, "mac");

    // Vendor
    devices[deviceCount].vendor = extractString(deviceJSON, "vendor");

    // Device type
    devices[deviceCount].type = extractString(deviceJSON, "type");

    // Fingerprint
    devices[deviceCount].fingerprint = extractNumber(deviceJSON, "fingerprint");

    // First seen
    devices[deviceCount].firstSeen = extractNumber(deviceJSON, "firstSeen");

    // Last seen
    devices[deviceCount].lastSeen = extractNumber(deviceJSON, "lastSeen");

    // Ports
    devices[deviceCount].ports = extractPorts(deviceJSON);

    // Debug information
    Serial.println();
    Serial.println("Device parsed:");

    Serial.print("IP: ");
    Serial.println(devices[deviceCount].ip);

    Serial.print("Hostname: ");
    Serial.println(devices[deviceCount].hostname);

    Serial.print("MAC: ");
    Serial.println(devices[deviceCount].mac);

    Serial.print("Vendor: ");
    Serial.println(devices[deviceCount].vendor);

    Serial.print("Type: ");
    Serial.println(devices[deviceCount].type);

    Serial.print("Ports: ");
    Serial.println(devices[deviceCount].ports);

    deviceCount++;

    searchPosition = objectEnd + 1;
  }

  // Update scanner heartbeat
  lastScannerUpdate = millis();

  Serial.println("SCANNER DATA PARSED");


  Serial.print("Scanner IP: ");
  Serial.println(scannerIP);

  Serial.print("Network: ");
  Serial.println(scannerNetwork);

  Serial.print("Reported Devices: ");
  Serial.println(reportedDeviceCount);

  Serial.print("Parsed Devices: ");
  Serial.println(deviceCount);
}

// Parse Honeypot data
void parseHoneypotData(String body){
  honeypotIP = extractString( body,"sensorIP");

  String type = extractString( body, "type");
  String sourceIP = extractString( body, "sourceIP");

  int port = extractNumber( body, "port");

  String service = extractString( body, "service");
  String details = extractString( body, "details");
  addSecurityEvent( "HONEYPOT", type, sourceIP, port, service, details);

  honeypotEventCount++;
  lastHoneypotUpdate = millis();
}

String extractPorts(String json){

  String search = "\"ports\":[";

  int start = json.indexOf(search);

  if(start == -1){
    return "";
  }

  start += search.length();

  int end =
    json.indexOf("]", start);

  if(end == -1){
    return "";
  }

  String portData =
    json.substring(start, end);

  portData.trim();

  if(portData.length() == 0){
    return "";
  }

  String result = "";

  int position = 0;
  bool firstPort = true;

  while(position < portData.length()){

    int comma =
      portData.indexOf(",", position);

    String port;

    if(comma == -1){

      port =
        portData.substring(position);

      position =
        portData.length();

    }
    else{

      port =
        portData.substring(
          position,
          comma
        );

      position = comma + 1;
    }

    port.trim();

    if(port.length() > 0){

      if(!firstPort){
        result += ", ";
      }

      result += port;

      firstPort = false;
    }
  }

  return result;
}

// Add secuirty event
void addSecurityEvent(String source, String type, String sourceIP, int port, String service, String details){
  if (eventCount >= MAX_EVENTS) {

    for (int i = 1;i < MAX_EVENTS;i++) {

      events[i - 1] = events[i];
    }

    eventCount = MAX_EVENTS - 1;
  }

  events[eventCount].source = source;
  events[eventCount].type = type;

  events[eventCount].sourceIP = sourceIP;
  events[eventCount].port = port;

  events[eventCount].service = service;
  events[eventCount].details = details;
  events[eventCount].timestamp = millis();

  eventCount++;
}

// Simple JSON String Extractor
String extractString( String json, String key) {

  String search = "\"" + key + "\":\"";

  int start = json.indexOf(search);

  if (start == -1) {
    return "";
  }

  start += search.length();

  int end =
  json.indexOf("\"", start);

  if (end == -1) {
    return "";
  }

  return json.substring(start, end);
}

// Simple JSON Numbwe Extractor
int extractNumber(String json, String key) {

  String search = "\"" + key + "\":";

  int start = json.indexOf(search);

  if (start == -1) {
    return 0;
  }

  start += search.length();

  int end = start;

  while (end < json.length() && isDigit(json[end])) {

    end++;
  }

  return json.substring(start, end).toInt();
}

// JSON Escape
String escapeJSON( String input) {

  input.replace("\\", "\\\\");
  input.replace("\"", "\\\"");
  input.replace("\n", "\\n");
  input.replace( "\r", "\\r");

  return input;
}

// 404
void handleNotFound() {

  server.send(404, "text/plain", "NETWATCH: Endpoint not found");
}
