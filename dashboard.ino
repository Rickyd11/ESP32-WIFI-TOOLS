// Libraries: WiFi, WebServer
#include <WiFi.h>
#include <WebServer.h>
#include <ArduinoJson.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>

#include "dashboard.h"
#include "your-nfty-cred.h"
#include "your-nfty-cert.h"

// Wifi credentials
const char* WIFI_SSID = "Your WiFi";
const char* WIFI_PASSWORD = "Your Password";

// Notification Variable
const char* NTFY_TOPIC = "Name of your notification";

// Server Configuration
WebServer server(80);

constexpr int MAX_DEVICES = 20;
constexpr int MAX_PORTS = 10;
constexpr int MAX_EVENTS = 50;

constexpr unsigned long WIFI_TIMEOUT = 20000UL;

constexpr unsigned long ONLINE_TIMEOUT = 300000UL;

constexpr size_t MAX_REQUEST_SIZE = 16000;

// Device Structure
struct Device{
  String ip;
  String hostname;
  String mac;
  String vendor;
  String type;

  uint16_t ports[MAX_PORTS] = {};
  int portCount = 0;

  int fingerprint = 0;

  unsigned long firstSeen = 0;
  unsigned long lastSeen = 0;

  bool online = false;
};

// Security Event
struct SecurityEvent{
  String source;
  String type;
  String sourceIP;
  String severity;

  int port = 0;
  String service;
  String details;

  unsigned long timestamp = 0;
};

// Global Data

Device devices[MAX_DEVICES];
SecurityEvent events[MAX_EVENTS];

int deviceCount = 0;
int eventCount = 0;

String scannerIP;
String scannerNetwork;
String honeypotIP;

unsigned long lastScannerUpdate = 0;
unsigned long lastHoneypotUpdate = 0;
unsigned long lastWiFiAttempt = 0;

bool serverStarted = false;

bool scannerHealthKnown = false;
bool scannerScanComplete = false;
bool scannerScanTruncated = false;

uint32_t scannerScanSequence = 0;
uint32_t scannerReportedDeviceCount = 0;
uint32_t scannerReportedDurationMs = 0;
uint32_t scannerCapacity = MAX_DEVICES;

// Function Declaration
void handleRoot();
void handleStatus();
void handleData();
void handleScan();
void handleHoneypot();

void sendDocument(JsonDocument& doc);
void sendError(int statusCode, const char* message);

void updateDeviceStatus();
void connectWiFi();

bool sendNotification(const String& title, const String& message);

void addSecurityEvent(String source, String type, String severity, String sourceIP, int port, String service, String details);
bool eventAlreadyStored(const String& source, const String& type, const String& sourceIP, int port, const String& details, unsigned long timestamp);

String determineSeverity(const String& type);

void readIncomingPorts(JsonVariantConst value, Device& device);

bool isValidIPv4(const String& ip);
int findDeviceByIP(const String& ip);

// Notification funtion
bool sendNotification(const String& title, const String& message){

  if(WiFi.status() != WL_CONNECTED){
    Serial.println(F("ntfy: Wi-Fi disconnected."));
    return false;
  }
  
  WiFiClientSecure client;
  client.setInsecure();

  HTTPClient http;

  String url = "https://ntfy.sh/";
  url += NTFY_TOPIC;

  if (!http.begin(client, url)){
    Serial.println(F("ntfy: Could not start HTTPS request."));
    return false;
  }

  http.setConnectTimeout(5000);
  http.setTimeout(5000);

  http.addHeader("Title", title);
  http.addHeader("Content-Type", "text/plain");

  int statusCode = http.POST(message);

  ool success = statusCode >= 200 && statusCode < 300;

  Serial.printf("ntfy HTTP status: %d\n", statusCode);

  if (!success) {
    Serial.println(http.getString());
  }

  http.end();

  return success;
}
// Json Response Helper
void sendDocument(JsonDocument& doc){
  
  String output;
  output.reserve(measureJson(doc));
  serializeJson(doc, output);

  server.sendHeader("Cache-Control", "no-store");
  server.send(200, "application/json; charset=utf-8", output);
}

// Error handler
void sendError(int statusCode, const char* message){
  JsonDocument doc;

  doc["success"] = false;
  doc["error"] = message;

  String output;
  serializeJson(doc, output);

  server.sendHeader("Cache-Control", "no-store");
  server.send(statusCode, "application/json; charset=utf-8", output);
}

// Device Lookup
int findDeviceByIP(const String& ip){
  for (int i = 0; i < deviceCount; i++){
    if (devices[i].ip == ip){
      return i;
    }
  }

  return -1;
}

bool isValidIPv4(const String& ip){
  IPAddress parsed;
  return parsed.fromString(ip);
}

// Device Status
void updateDeviceStatus(){
  unsigned long now = millis();

  bool scannerIsStale = lastScannerUpdate == 0 || now - lastScannerUpdate >= ONLINE_TIMEOUT;
      
  if (scannerHealthKnown && !scannerScanComplete && !scannerIsStale){
    return;
  }

  for (int i = 0; i < deviceCount; i++){
    unsigned long age = now - devices[i].lastSeen;

    devices[i].online = age < ONLINE_TIMEOUT;
  }
}

// API: Server Status
void handleStatus(){
  JsonDocument doc;

  doc["server"] = WiFi.status() == WL_CONNECTED ? "ONLINE" : "OFFLINE";

  doc["ip"] = WiFi.localIP().toString();
  doc["uptime"] = millis() / 1000;

  doc["deviceCount"] = deviceCount;
  doc["eventCount"] = eventCount;

  doc["freeHeap"] = ESP.getFreeHeap();
  doc["minimumFreeHeap"] = ESP.getMinFreeHeap();

  doc["scannerIP"] = scannerIP;
  doc["network"] = scannerNetwork;

  doc["scannerAge"] = lastScannerUpdate == 0 ? 999999 : (millis() - lastScannerUpdate) / 1000;

  doc["honeypotAge"] = lastHoneypotUpdate == 0 ? 999999 : (millis() - lastHoneypotUpdate) / 1000;

  doc["scannerHealthKnown"] = scannerHealthKnown;
  doc["scanComplete"] = scannerScanComplete;
  doc["scanTruncated"] = scannerScanTruncated;
  doc["scanSequence"] = scannerScanSequence;
  doc["scanDurationMs"] = scannerReportedDurationMs;
  doc["scannerCapacity"] = scannerCapacity;

  doc["scannerReportedDeviceCount"] = scannerReportedDeviceCount;

  doc["onlineTimeoutMs"] = ONLINE_TIMEOUT;

  sendDocument(doc);
}

// API: Network Data
void handleData() {
  updateDeviceStatus();

  JsonDocument doc;

  doc["scannerIP"] = scannerIP;
  doc["network"] = scannerNetwork;
  doc["honeypotIP"] = honeypotIP;

  doc["scanComplete"] = scannerScanComplete;
  doc["scanTruncated"] = scannerScanTruncated;
  doc["scanSequence"] = scannerScanSequence;
  doc["scanDurationMs"] = scannerReportedDurationMs;
  doc["scannerCapacity"] = scannerCapacity;
  doc["scannerHealthKnown"] = scannerHealthKnown;

  doc["scannerAge"] = lastScannerUpdate == 0 ? 999999 : (millis() - lastScannerUpdate) / 1000;

  doc["onlineTimeoutMs"] = ONLINE_TIMEOUT;

  JsonArray deviceArray = doc["devices"].to<JsonArray>();

  for (int i = 0; i < deviceCount; i++){
    JsonObject obj = deviceArray.add<JsonObject>();

    obj["ip"] = devices[i].ip;
    obj["hostname"] = devices[i].hostname;
    obj["mac"] = devices[i].mac;
    obj["vendor"] = devices[i].vendor;
    obj["type"] = devices[i].type;

    JsonArray ports = obj["ports"].to<JsonArray>();

    for (int p = 0; p < devices[i].portCount; p++){
      ports.add(devices[i].ports[p]);
    }

    obj["fingerprint"] = devices[i].fingerprint;
    obj["firstSeen"] = devices[i].firstSeen;
    obj["lastSeen"] = devices[i].lastSeen;
    obj["online"] = devices[i].online;
  }

  JsonArray eventArray = doc["events"].to<JsonArray>();

  for (int i = 0; i < eventCount; i++) {
    JsonObject obj = eventArray.add<JsonObject>();

    obj["source"] = events[i].source;
    obj["type"] = events[i].type;
    obj["sourceIP"] = events[i].sourceIP;
    obj["port"] = events[i].port;
    obj["service"] = events[i].service;
    obj["details"] = events[i].details;
    obj["timestamp"] = events[i].timestamp;
    obj["severity"] = events[i].severity;
  }

  sendDocument(doc);
}

// Event Security
String determineSeverity(const String& type){
  if (type == "DEVICE_OFFLINE" || type == "DEVICE_RETURNED"){
    return "WARNING";
  }

  if (type == "PORT_OPENED" || type == "HONEYPOT_ACTIVITY" || type == "HONEYPOT_LOGIN" || type == "HONEYPOT_ACCESS"){
    return "WARNING";
  }

  if (type == "DEVICE_NEW" || type == "PORT_CLOSED"){
    return "INFO";
  }

  return "INFO";
}

// Event Duplication
bool eventAlreadyStored(const String& source, const String& type, const String& sourceIP, int port, const String& details, unsigned long timestamp){
  for (int i = 0; i < eventCount; i++){
    if (events[i].source == source && events[i].type == type && events[i].sourceIP == sourceIP && events[i].port == port && events[i].details == details && events[i].timestamp == timestamp){
      return true;
    }
  }

  return false;
}

// Security Event Manager
void addSecurityEvent(String source, String type, String severity, String sourceIP, int port, String service, String details){
  if (eventCount >= MAX_EVENTS){
    for (int i = 1; i < MAX_EVENTS; i++){
      events[i - 1] = events[i];
    }

    eventCount = MAX_EVENTS - 1;
  }

  SecurityEvent& e = events[eventCount++];

  e.source = source;
  e.type = type;
  e.severity = severity;
  e.sourceIP = sourceIP;
  e.port = port;
  e.service = service;
  e.details = details;
  e.timestamp = millis() / 1000;

  Serial.printf("[EVENT] %s | %s | %s\n", type.c_str(), sourceIP.c_str(), details.c_str());
}

// Port Parsing
void readIncomingPorts( JsonVariantConst value, Device& device){
  // Missing port field: preserve previously recorded ports.
  if (value.isUnbound()){
    return;
  }

  // Standard scanner format: [22, 80, 443]
  if (value.is<JsonArrayConst>()){
    JsonArrayConst ports = value.as<JsonArrayConst>();

    device.portCount = 0;

    for (JsonVariantConst portValue : ports){
      if (device.portCount >= MAX_PORTS){
        break;
      }

      int port = -1;

      if (portValue.is<int>() || portValue.is<unsigned int>()){
        port = portValue.as<int>();
      } else if (portValue.is<const char*>()){
        port = portValue.as<String>().toInt();
      }

      if (port < 1 || port > 65535) {
        continue;
      }

      bool duplicate = false;

      for (int i = 0; i < device.portCount; i++){
        if (device.ports[i] == port){
          duplicate = true;
          break;
        }
      }

      if (!duplicate){
        device.ports[device.portCount++] = static_cast<uint16_t>(port);
      }
    }

    return;
  }

  // Backward compatibility: "22,80,443" or "[\"22\",\"80\"]"
  if (value.is<const char*>() || value.is<String>()){
    String text = value.as<String>();
    text.trim();

    if (text.isEmpty()) {
      device.portCount = 0;
      return;
    }

    // Remove JSON array brackets if present.
    text.replace("[", "");
    text.replace("]", "");
    text.replace("\"", "");
    text.replace("'", "");

    device.portCount = 0;

    int start = 0;

    while (start < text.length() && device.portCount < MAX_PORTS){
      int comma = text.indexOf(',', start);

      if (comma < 0){
        comma = text.length();
      }

      String part = text.substring(start, comma);
      part.trim();

      bool digitsOnly = !part.isEmpty();

      for (unsigned int i = 0; i < part.length(); i++){
        if (!isDigit(part[i])) {
          digitsOnly = false;
          break;
        }
      }

      if (digitsOnly){
        long port = part.toInt();

        if (port >= 1 && port <= 65535){
          bool duplicate = false;

          for (int i = 0; i < device.portCount; i++){
            if (device.ports[i] == port){
              duplicate = true;
              break;
            }
          }

          if (!duplicate){
            device.ports[device.portCount++] = static_cast<uint16_t>(port);
          }
        }
      }

      start = comma + 1;
    }
  }
}

// API: Scanner Ingestion
void handleScan(){
  if (server.method() != HTTP_POST){
    sendError(405, "Method not allowed");
    return;
  }

  if (!server.hasArg("plain")){
    sendError(400, "Missing JSON body");
    return;
  }

  String body = server.arg("plain");

  if (body.length() > MAX_REQUEST_SIZE){
    sendError(413, "Payload too large");
    return;
  }

  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, body);

  if (error){
    Serial.print(F("[ERROR] Invalid scanner JSON: "));
    Serial.println(error.c_str());

    sendError(400, "Invalid JSON");
    return;
  }

  if (!doc["devices"].is<JsonArray>()){
    sendError(400, "Missing or invalid devices array");
    return;
  }

  bool hasCompleteField =  doc["scanComplete"].is<bool>();

  bool incomingComplete = hasCompleteField ? doc["scanComplete"].as<bool>() : true;

  bool incomingTruncated = doc["scanTruncated"] | false;

  uint32_t incomingSequence = doc["scanSequence"] | 0;

  uint32_t incomingDuration = doc["scanDurationMs"] | 0;

  uint32_t incomingCapacity = doc["scannerCapacity"] | MAX_DEVICES;

  if (incomingTruncated){
    incomingComplete = false;
  }

  scannerIP = doc["scannerIP"] | scannerIP;
  scannerNetwork = doc["network"] | scannerNetwork;

  scannerHealthKnown = hasCompleteField;
  scannerScanComplete = incomingComplete;
  scannerScanTruncated = incomingTruncated;

  scannerScanSequence = incomingSequence;
  scannerReportedDurationMs = incomingDuration;
  scannerCapacity = incomingCapacity;

  lastScannerUpdate = millis();

  JsonArray incomingDevices =
      doc["devices"].as<JsonArray>();

  scannerReportedDeviceCount = incomingDevices.size();

  for (JsonObject incoming : incomingDevices){
    String ip = incoming["ip"] | "";

    if (ip.isEmpty() || !isValidIPv4(ip)){
      continue;
    }

    int index = findDeviceByIP(ip);

    bool isNewDevice = index < 0;

    if (isNewDevice){
      if (deviceCount >= MAX_DEVICES){
        Serial.println(F("[WARN] Server inventory capacity reached."));
        continue;
      }

      index = deviceCount++;

      devices[index].ip = ip;
      devices[index].firstSeen = millis();
      devices[index].lastSeen = millis();
      devices[index].online = true;

      devices[index].hostname = "";
      devices[index].mac = "";
      devices[index].vendor = "";
      devices[index].type = "";
      devices[index].fingerprint = 0;
      devices[index].portCount = 0;

      Serial.print(F("[INVENTORY] New device: "));
      Serial.println(ip);
    }

    Device& d = devices[index];

    String hostname = incoming["hostname"] | "";
    String mac = incoming["mac"] | "";
    String vendor = incoming["vendor"] | "";
    String type = incoming["type"] | "";

    if (!hostname.isEmpty() && hostname != "UNKNOWN"){
      d.hostname = hostname;
    }

    if (!mac.isEmpty() && mac != "UNKNOWN"){
      d.mac = mac;
    }

    if (!vendor.isEmpty() && vendor != "UNKNOWN" && vendor != "Unknown vendor"){
      d.vendor = vendor;
    } else if (d.vendor.isEmpty()){
      d.vendor = vendor;
    }

    if (!type.isEmpty() && type != "UNKNOWN" && type != "Unknown"){
      d.type = type;
    }

    readIncomingPorts(incoming["ports"], d);

    if (incoming["fingerprint"].is<int>()){
      d.fingerprint = incoming["fingerprint"].as<int>();
    }

   
    d.lastSeen = millis();
    d.online = true;

    Serial.printf("[DEVICE] %s | online | ports=%d\n", d.ip.c_str(), d.portCount);
  }

  // Ingest scanner events.
  JsonArray incomingEvents = doc["events"].as<JsonArray>();

  for (JsonObject incomingEvent : incomingEvents){
    
    String type = incomingEvent["type"] | "SCAN_EVENT";
    String sourceIP = incomingEvent["sourceIP"] | "";

    if (sourceIP.isEmpty()){
      sourceIP = incomingEvent["ip"] | "";
    }

    String details = incomingEvent["details"] | "";

    if (details.isEmpty()){
      details = incomingEvent["description"] | "";
    }

    int port = incomingEvent["port"] | 0;
    String service = incomingEvent["service"] | "";

    if (service.isEmpty() && port > 0){
      service = "TCP";
    }

    unsigned long eventTimestamp = incomingEvent["timestamp"] | 0UL;

    String severity = determineSeverity(type);

    if (eventAlreadyStored("SCANNER", type, sourceIP, port, details, eventTimestamp)){
      continue;
    }

    addSecurityEvent("SCANNER", type, severity, sourceIP, port, service, details);

    if (eventTimestamp != 0 && eventCount > 0) {
      events[eventCount - 1].timestamp = eventTimestamp;
    }
  }

  Serial.println();
  Serial.println(F("Scanner Update"));

  Serial.print(F("Scanner IP: "));
  Serial.println(scannerIP);

  Serial.print(F("Reported devices: "));
  Serial.println(scannerReportedDeviceCount);

  Serial.print(F("Complete: "));
  Serial.println(scannerScanComplete ? F("yes") : F("no"));

  Serial.print(F("Truncated: "));
  Serial.println(scannerScanTruncated ? F("yes") : F("no"));

  Serial.print(F("Stored events: "));
  Serial.println(eventCount);

  JsonDocument response;

  response["success"] = true;
  response["message"] = "Scan received";
  response["scanComplete"] = scannerScanComplete;
  response["scanSequence"] = scannerScanSequence;
  response["receivedDevices"] = scannerReportedDeviceCount;
  response["storedDevices"] = deviceCount;
  response["storedEvents"] = eventCount;

  sendDocument(response);
}

// API: Honey Pot Ingestion
void handleHoneypot(){
  if (server.method() != HTTP_POST){
    sendError(405, "Method not allowed");
    return;
  }

  if (!server.hasArg("plain")){
    sendError(400, "Missing JSON body");
    return;
  }

  String body = server.arg("plain");

  if (body.length() > MAX_REQUEST_SIZE){
    sendError(413, "Payload too large");
    return;
  }

  JsonDocument doc;

  DeserializationError error = deserializeJson(doc, body);

  if (error){
    sendError(400, "Invalid JSON");
    return;
  }

  honeypotIP = doc["honeypotIP"] | "";
  lastHoneypotUpdate = millis();

  String type = doc["type"] | "HONEYPOT_ACTIVITY";
  String sourceIP = doc["sourceIP"] | "";
  int port = doc["port"] | 0;
  String service = doc["service"] | "Unknown";
  String details = doc["details"] | "";

  addSecurityEvent("HONEYPOT", type, determineSeverity(type), sourceIP, port, service, details);

  JsonDocument response;

  response["success"] = true;
  response["message"] = "Honeypot event received";

  sendDocument(response);
}

// Root Dashboard
void handleRoot(){
  server.sendHeader("Cache-Control", "no-store");

  server.send_P(200, "text/html; charset=utf-8", DASHBOARD_HTML);
}

// Wifi Connection
void connectWiFi(){
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  unsigned long start = millis();

  while (WiFi.status() != WL_CONNECTED && millis() - start < WIFI_TIMEOUT){
    delay(250);
  }

  if (WiFi.status() == WL_CONNECTED){
    Serial.print(F("NETWATCH IP: "));
    Serial.println(WiFi.localIP());
  } else {
    Serial.println(F("[WARN] Initial Wi-Fi connection failed."));
  }
}

//Setup
void setup() {
  Serial.begin(115200);
  delay(500);

  Serial.println();
  Serial.println(F("ESP32 #3 DASHBOARD"));

  connectWiFi();

  server.on("/", HTTP_GET, handleRoot);

  server.on("/api/status", HTTP_GET, handleStatus);
  server.on("/api/data", HTTP_GET, handleData);

  server.on("/api/scan", HTTP_POST, handleScan);
  server.on("/api/honeypot", HTTP_POST, handleHoneypot);

  server.onNotFound([](){
    sendError(404, "Not found");
  });

  if (WiFi.status() == WL_CONNECTED){
    server.begin();
    serverStarted = true;

    Serial.println(F("[OK] NETWATCH server started."));

    String dashboardURL = "http://";
    dashboardURL += WiFi.localIP().toString();
    dashboardURL += "/";

    Serial.print(F("Dashboard URL: "));
    Serial.println(dashboardURL);

    sendNotification( "Your message", "Your dashboard is ready:\n" + dashboardURL);
  }
}

// Main Loop
void loop(){
  if (WiFi.status() == WL_CONNECTED){
    if (!serverStarted){
      server.begin();
      serverStarted = true;

      Serial.println(F("[OK] NETWATCH server restarted."));
    }

    server.handleClient();
  } else {
    if (serverStarted){
      serverStarted = false;
      Serial.println(F("[WARN] Wi-Fi disconnected."));
    }

    if (millis() - lastWiFiAttempt >= WIFI_TIMEOUT){
      lastWiFiAttempt = millis();
      WiFi.reconnect();
    }
  }
}
