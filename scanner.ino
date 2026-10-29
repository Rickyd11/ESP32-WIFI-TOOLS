// Libraries needed for the file
#include <WiFi.h>
#include <WiFiClient.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <esp32ARP.h>
#include <ESPmDNS.h>
#include <WiFiUdp.h>

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
constexpr unsigned long SCAN_INTERVAL = 30000UL;
constexpr unsigned long WIFI_RETRY_INTERVAL = 5000UL;
constexpr unsigned long ARP_RETRY_INTERVAL = 5000UL;
constexpr uint16_t CONNECT_TIMEOUT_MS = 250;
constexpr uint16_t ARP_WAIT_MS = 100;
constexpr uint8_t OFFLINE_MISSED_SCANS = 2;

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
  String deviceType;

  bool online;

  uint16_t openPorts[MAX_PORTS];
  int portCount;

  int fingerprintScore;

  unsigned long firstSeen;
  unsigned long lastSeen;
};
// Security Event
struct SecurityEvent {
  String type;
  String ip;
  String description;
  int port;
  unsigned long timestamp;
};

// Global Storage 
NetworkDevice currentDevices[MAX_DEVICES];
NetworkDevice previousDevices[MAX_DEVICES];
SecurityEvent events[MAX_EVENTS];

int currentDeviceCount = 0;
int previousDeviceCount = 0;
int eventCount = 0;

uint32_t droppedEventCount = 0;
int lastUploadHttpCode = 0;
bool lastUploadSucceeded = false;

bool arpReady = false;
bool baselineReady = false;
bool scannerInitialized = false;
bool mdnsReady = false;

IPAddress localIP;
IPAddress networkAddress;
IPAddress subnetMask;

bool lastScanComplete = false;
bool lastScanTruncated = false;

uint32_t scanSequence = 0;
unsigned long lastScanDurationMs = 0;
unsigned long lastArpRetry = 0;

struct MissingDeviceTrack {
  IPAddress ip;
  uint8_t missedCompleteScans;
  bool offlineEventSent;
};

MissingDeviceTrack missingTracks[MAX_DEVICES];
int missingTrackCount = 0;

unsigned long lastScan = 0;
unsigned long lastWiFiRetry = 0;

// Function Declearation
bool connectWiFi();
void maintainWiFi();
void initializeScannerIfReady();
void printNetworkInformation();

IPAddress calculateNetworkAddress();
void runNetworkScan();
void clearCurrentScan();
void discoverDevices();

bool checkHost(IPAddress ip, String& discoveredMac);
String macToString(const mac_addr_t& mac);
String lookupVendor(const String& mac);
String detectHostname(IPAddress ip);

void collectDeviceInfo(NetworkDevice& device);
void detectHostname(NetworkDevice& device);
void initializeMDNS();
String queryMDNSHostname(IPAddress target);
static String queryNetBIOSHostname(IPAddress target);
void identifyVendor(NetworkDevice& device);
void detectService(NetworkDevice& device);
bool checkPort(IPAddress ip, uint16_t port);
void fingerprintDevice(NetworkDevice& device);

void compareScans();
void processMissingDeviceTracks();
bool currentScanContains(IPAddress ip);
bool isMissingDeviceTracked(IPAddress ip);
bool sameDevice(const NetworkDevice& a, const NetworkDevice& b);
void removeMissingTrack(int index);

void generateNewDeviceEvent(NetworkDevice& device);
void generateOfflineDeviceEvent(NetworkDevice& device);
void generatePortChangeEvent(NetworkDevice& device, uint16_t port, bool opened);
void addEvent(const String& type, const String& ip, const String& description, int port = 0);

void printDevices();
void printEvents();
void sendResultsToServer();
void saveCurrentAsPrevious();

// Setup 
void setup(){
  Serial.begin(115200);
  delay(1000);


  Serial.println(F("Scanner is ready"));

  connectWiFi();
  initializeScannerIfReady();
}

// Loop 
void loop(){
  maintainWiFi();

  if (WiFi.status() != WL_CONNECTED){
    delay(10);
    return;
  }

  initializeScannerIfReady();
  localIP = WiFi.localIP();
  subnetMask = WiFi.subnetMask();
  networkAddress = calculateNetworkAddress();
  initializeMDNS();

  if (!scannerInitialized) {
    delay(10);
    return;
  }

  if (!arpReady && millis() - lastArpRetry >= ARP_RETRY_INTERVAL){

    lastArpRetry = millis();
    Serial.println(F("[INFO] Retrying ARP initialization..."));

    if (arp.init() == ERR_OK){
      arpReady = true;
      Serial.println(F("[OK] ARP initialized on retry."));

      runNetworkScan();
      lastScan = millis();
    } else {
      Serial.println(F("[WARN] ARP retry failed."));
    }
  }

  if (arpReady && millis() - lastScan >= SCAN_INTERVAL){

    runNetworkScan();
    lastScan = millis();
  }

  delay(2);
}

// Connect to Wifi
bool connectWiFi(){
  WiFi.mode(WIFI_STA);
  WiFi.setSleep(false);

  Serial.print(F("Connecting to "));
  Serial.println(WIFI_SSID);

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  const unsigned long startedAt = millis();

  while (WiFi.status() != WL_CONNECTED &&
         millis() - startedAt < 15000UL){

    Serial.print('.');
    delay(500);
  }

  Serial.println();

  if (WiFi.status() != WL_CONNECTED){
    Serial.println(F("[ERROR] Wi-Fi connection failed."));
    return false;
  }

  localIP = WiFi.localIP();

  Serial.println(F("[OK] Wi-Fi connected."));

  Serial.print(F("Scanner IP: "));
  Serial.println(localIP);

  Serial.print(F("Scanner MAC: "));
  Serial.println(WiFi.macAddress());

  return true;
}

// Initializes the scanner once Wi-Fi is connected
void initializeScannerIfReady(){
  if (scannerInitialized || WiFi.status() != WL_CONNECTED){
    return;
  }

  localIP = WiFi.localIP();
  subnetMask = WiFi.subnetMask();
  networkAddress = calculateNetworkAddress();

  if (arp.init() == ERR_OK){
    arpReady = true;
    Serial.println(F("[OK] ARP initialized."));
  } else {
    arpReady = false;

    Serial.println(F("[ERROR] ARP initialization failed."));
    Serial.println(F("MAC discovery may not work."));
  }

  printNetworkInformation();

  scannerInitialized = true;

  Serial.println(F("[OK] Scanner initialized."));

  if (arpReady){
    runNetworkScan();
    lastScan = millis();
  }
}

// Maintain the Wifi connection
void maintainWiFi(){
  if (WiFi.status() == WL_CONNECTED){
    return;
  }

  if (millis() - lastWiFiRetry < WIFI_RETRY_INTERVAL){
    return;
  }

  lastWiFiRetry = millis();

  Serial.println(F("[WARN] Wi-Fi disconnected. Reconnecting..."));

  WiFi.disconnect();
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
}

// Network Information
IPAddress calculateNetworkAddress(){
  IPAddress ip = WiFi.localIP();
  IPAddress mask = WiFi.subnetMask();
  IPAddress network;

  for (int i = 0; i < 4; i++){
    network[i] = ip[i] & mask[i];
  }

  return network;
}

// Initializes mDNS when Wi-Fi is connected allows for the device to be discovered on the local network using its hostname
void initializeMDNS() {
  if (mdnsReady || WiFi.status() != WL_CONNECTED){
    return;
  }

  mdnsReady = MDNS.begin("netwatch-sentinel");

  if (mdnsReady){
    Serial.println(F("[MDNS] Initialized"));
  } else {
    Serial.println(F("[MDNS] Initialization failed"));
  }
}

// Queries mDNS services on the local network to identify a device's hostname by matching the discovered service addresses to the target IP address
String queryMDNSHostname(IPAddress target){
  if (!mdnsReady){
    return "UNKNOWN";
  }

  const char* services[] = {"http","ipp","airplay","raop","googlecast","ssh"};

  const size_t serviceCount = sizeof(services) / sizeof(services[0]);

  for (size_t s = 0; s < serviceCount; ++s){
    int count = MDNS.queryService(services[s], "tcp");

    for (int i = 0; i < count; ++i){
      if (MDNS.address(i) == target){
        String name = MDNS.hostname(i);

        if (name.length() > 0){
          Serial.print(F("[MDNS] Found "));
          Serial.print(target);
          Serial.print(F(" -> "));
          Serial.println(name);

          return name;
        }
      }
    }
  }

  return "UNKNOWN";
}

// For debugging prints the network information
void printNetworkInformation(){
  Serial.println();
  Serial.println(F("Network information"));

  Serial.print(F("IP address: "));
  Serial.println(WiFi.localIP());

  Serial.print(F("Gateway: "));
  Serial.println(WiFi.gatewayIP());

  Serial.print(F("Subnet mask: "));
  Serial.println(WiFi.subnetMask());

  Serial.print(F("DNS: "));
  Serial.println(WiFi.dnsIP());

  Serial.print(F("Network address: "));
  Serial.println(calculateNetworkAddress());

  if (subnetMask != IPAddress(255, 255, 255, 0)){
    Serial.println(F("[WARN] This scanner is designed for /24 networks."));
  }
}

// Scan control
void runNetworkScan(){
  if (WiFi.status() != WL_CONNECTED){
    Serial.println(F("[WARN] Skipping scan: Wi-Fi disconnected."));
    lastScanComplete = false;
    return;
  }

  const unsigned long scanStartedAt = millis();

  scanSequence++;
  lastScanComplete = false;
  lastScanTruncated = false;

  Serial.println();
  Serial.println(F("Starting network scan"));

  clearCurrentScan();
  discoverDevices();

  Serial.println();
  Serial.println(F("Collecting device information..."));

  for (int i = 0; i < currentDeviceCount; i++){
    if (WiFi.status() != WL_CONNECTED){
      lastScanComplete = false;

      Serial.println(F("[WARN] Wi-Fi dropped during inspection."));
      break;
    }

    collectDeviceInfo(currentDevices[i]);
  }

  if (lastScanComplete) {
    Serial.println();
    Serial.println(F("Comparing against previous complete scan..."));

    compareScans();
  } else {
    Serial.println(F("[WARN] Incomplete scan; comparison and baseline update skipped."));
  }

  lastScanDurationMs = millis() - scanStartedAt;

  printDevices();
  printEvents();
  sendResultsToServer();

  if (lastScanComplete){
    saveCurrentAsPrevious();
  }

  Serial.print(F("Scan sequence: "));
  Serial.println(scanSequence);

  Serial.print(F("Scan complete: "));
  Serial.println(lastScanComplete ? F("yes") : F("no"));

  Serial.print(F("Scan truncated: "));
  Serial.println(lastScanTruncated ? F("yes") : F("no"));

  Serial.print(F("Scan duration ms: "));
  Serial.println(lastScanDurationMs);
}

// Clear the current scan
void clearCurrentScan(){
  currentDeviceCount = 0;

  for (int i = 0; i < MAX_DEVICES; i++){
    currentDevices[i].online = false;
    currentDevices[i].portCount = 0;
    currentDevices[i].fingerprintScore = 0;
  }
}

// Device Discovery
void discoverDevices(){
  lastScanComplete = false;
  lastScanTruncated = false;

  if (WiFi.status() != WL_CONNECTED || !arpReady){
    Serial.println(F("[WARN] Discovery unavailable: Wi-Fi or ARP is not ready."));
    return;
  }

  networkAddress = calculateNetworkAddress();
  subnetMask = WiFi.subnetMask();

  if (subnetMask != IPAddress(255, 255, 255, 0)){
    Serial.println(F("[ERROR] Refusing to scan: subnet is not /24."));
    return;
  }

  Serial.print(F("Scanning network: "));
  Serial.println(networkAddress);

  for (int host = 1; host <= 254; host++){
    if (WiFi.status() != WL_CONNECTED) {
      Serial.println(F("[WARN] Wi-Fi dropped during discovery."));
      return;
    }

    IPAddress target(networkAddress[0], networkAddress[1], networkAddress[2], host);

    if (target == localIP){
      continue;
    }

    Serial.print(F("[CHECK] "));
    Serial.print(target);
    Serial.print(F(" -> "));

    String discoveredMac;

    if (!checkHost(target, discoveredMac)){
      Serial.println(F("No ARP response"));
      delay(5);
      continue;
    }

    Serial.print(F("ONLINE | MAC: "));
    Serial.println(discoveredMac);

    if (currentDeviceCount >= MAX_DEVICES){
      lastScanTruncated = true;

      Serial.println(F("[WARN] Device capacity reached; scan is truncated."));

      break;
    }

    NetworkDevice& device = currentDevices[currentDeviceCount];

    device.ip = target;
    device.online = true;
    device.mac = discoveredMac;
    device.hostname = "UNKNOWN";
    device.vendor = "UNKNOWN";
    device.deviceType = "Unknown";
    device.portCount = 0;
    device.fingerprintScore = 0;

    bool existed = false;

    for (int i = 0; i < previousDeviceCount; i++){
      if ((previousDevices[i].mac.length() > 0 && previousDevices[i].mac != "UNKNOWN" && discoveredMac.length() > 0 && discoveredMac != "UNKNOWN" && previousDevices[i].mac.equalsIgnoreCase(discoveredMac)) || previousDevices[i].ip == target){

        existed = true;
        device.firstSeen = previousDevices[i].firstSeen;

        if (previousDevices[i].hostname != "UNKNOWN"){
          device.hostname = previousDevices[i].hostname;
        }

        if (previousDevices[i].vendor != "UNKNOWN"){
          device.vendor = previousDevices[i].vendor;
        }

        break;
      }
    }

    if (!existed){
      device.firstSeen = millis();
    }

    device.lastSeen = millis();
    currentDeviceCount++;
    Serial.print(F("Device added: "));
    Serial.println(target);

    delay(5);
  }

  Serial.print(F("Discovered devices: "));
  Serial.println(currentDeviceCount);

  lastScanComplete = (WiFi.status() == WL_CONNECTED) && !lastScanTruncated;

  if (!lastScanComplete){
    Serial.println(F("[WARN] Discovery did not complete successfully."));
  }
}

// ARP / MAC Discovery
bool checkHost(IPAddress ip, String& discoveredMac){
  discoveredMac = "UNKNOWN";

  if (!arpReady){
    return false;
  }

  int requestResult = arp.sendRequest(ip.toString());

  if (requestResult != ERR_OK){
    return false;
  }

  delay(ARP_WAIT_MS);
  mac_addr_t mac;
  int result = arp.lookupEntry(ip.toString(), mac);

  if (result != ERR_OK){
    return false;
  }

  discoveredMac = macToString(mac);
  return discoveredMac != "UNKNOWN";
}

// Converts a device's MAC address into a readable string
String macToString(const mac_addr_t& mac){
  char buffer[18];

  snprintf(buffer, sizeof(buffer), "%02X:%02X:%02X:%02X:%02X:%02X", mac.addr[0], mac.addr[1], mac.addr[2], mac.addr[3], mac.addr[4], mac.addr[5]);

  return String(buffer);
}

// Device Information
void collectDeviceInfo(NetworkDevice& device){
  Serial.println();
  Serial.print(F("Inspecting "));
  Serial.println(device.ip);

  detectHostname(device);
  identifyVendor(device);
  detectService(device);
  fingerprintDevice(device);
}

// Sends a NetBIOS Name Service (NBNS) query to a target IP address
static String queryNetBIOSHostname(IPAddress target){
  WiFiUDP udp;

  const uint16_t localPort = 40137;
  const uint16_t remotePort = 137;

  uint8_t packet[50] = {0};

  // NBNS transaction ID
  packet[0] = 0x12;
  packet[1] = 0x34;

  // Standard query flags
  packet[2] = 0x00;
  packet[3] = 0x00;

  // One question
  packet[4] = 0x00;
  packet[5] = 0x01;

  // No answer, authority, or additional records
  packet[6] = 0x00;
  packet[7] = 0x00;
  packet[8] = 0x00;
  packet[9] = 0x00;
  packet[10] = 0x00;
  packet[11] = 0x00;

  // Encoded NetBIOS wildcard name: *<00>
  packet[12] = 0x20;

  for (int i = 0; i < 16; i++){
    uint8_t value = (i == 0) ? '*' : ' ';
    packet[13 + i * 2] = 'A' + ((value >> 4) & 0x0F);
    packet[14 + i * 2] = 'A' + (value & 0x0F);
  }

  packet[45] = 0x00;  // End of encoded name
  packet[46] = 0x00;  // NB query type
  packet[47] = 0x21;
  packet[48] = 0x00;  // Internet class
  packet[49] = 0x01;

  if (!udp.begin(localPort)){
    return "UNKNOWN";
  }

  if (!udp.beginPacket(target, remotePort)){
    udp.stop();
    return "UNKNOWN";
  }

  udp.write(packet, sizeof(packet));

  if (!udp.endPacket()){
    udp.stop();
    return "UNKNOWN";
  }

  unsigned long started = millis();

  while (millis() - started < 700){
    int packetSize = udp.parsePacket();

    if (packetSize >= 12){
      uint8_t response[512];
      int bytesRead = udp.read(response, min(packetSize, (int)sizeof(response)));

      udp.stop();

      if (bytesRead < 12){
        return "UNKNOWN";
      }

      // Validate transaction ID and response flag
      if (response[0] != 0x12 || response[1] != 0x34 || !(response[2] & 0x80)){
        return "UNKNOWN";
      }

      // NBNS responses contain variable-length records.
      // This minimal implementation doesn't parse those records.
      // A complete parser is needed to extract a reliable name.
      return "UNKNOWN";
    }

    delay(5);
  }

  udp.stop();
  return "UNKNOWN";
}

// Detects the hostname 
void detectHostname(NetworkDevice& device){
  // Keep previously discovered hostnames.
  if (device.hostname.length() > 0 && device.hostname != "UNKNOWN"){
    return;
  }

  Serial.print(F("[HOSTNAME] Resolving "));
  Serial.println(device.ip);

  // First attempt: discover advertised mDNS services.
  String resolved = queryMDNSHostname(device.ip);

  // Second attempt: use the existing NetBIOS query.
  if (resolved == "UNKNOWN"){
    resolved = queryNetBIOSHostname(device.ip);
  }

  if (resolved.length() > 0 &&
      resolved != "UNKNOWN") {
    device.hostname = resolved;

    Serial.print(F("[HOSTNAME] "));
    Serial.print(device.ip);
    Serial.print(F(" -> "));
    Serial.println(device.hostname);
  } else {
    device.hostname = "UNKNOWN";

    Serial.println(F("[HOSTNAME] No hostname response"));
  }
}

// Identifies the vender of the device
void identifyVendor(NetworkDevice& device){
  if (device.mac.length() == 0 || device.mac == "UNKNOWN"){
    device.vendor = "UNKNOWN";
  } else {
    device.vendor = lookupVendor(device.mac);
  }

  Serial.print(F("MAC: "));
  Serial.println(device.mac);

  Serial.print(F("Vendor: "));
  Serial.println(device.vendor);
}

// Searches for the vendor 
String lookupVendor(const String& mac){
  String normalized;
  normalized.reserve(12);

  for (size_t i = 0; i < mac.length(); i++){
    char c = mac[i];

    if (c >= '0' && c <= '9'){
      normalized += c;
    } else if (c >= 'a' && c <= 'f'){
      normalized += static_cast<char>(c - 'a' + 'A');
    } else if (c >= 'A' && c <= 'F'){
      normalized += c;
    }
  }

  if (normalized.length() != 12){
    return "Unknown vendor";
  }

  const char firstOctetText[] = {
    normalized[0], normalized[1], '\0'
  };

  const uint8_t firstOctet = static_cast<uint8_t>(strtoul(firstOctetText, nullptr, 16));

  if (firstOctet & 0x02){
    return "Locally administered (vendor uncertain)";
  }

  const String prefix = normalized.substring(0, 6);

  if (prefix == "246F28" || prefix == "7CDFA1"){
    return "Espressif (known prefix)";
  }

  return "Unknown vendor";
}

// TCP Port Scan
void detectService(NetworkDevice& device){
  Serial.print(F("Open ports: "));

  bool foundPort = false;

  for (size_t i = 0; i < COMMON_PORT_COUNT; i++){
    if (device.portCount >= MAX_PORTS){
      break;
    }

    uint16_t port = COMMON_PORTS[i];

    if (checkPort(device.ip, port)){
      device.openPorts[device.portCount++] = port;

      if (foundPort){
        Serial.print(F(", "));
      }

      Serial.print(port);
      foundPort = true;
    }
  }

  if (!foundPort){
    Serial.print(F("None detected"));
  }

  Serial.println();
}

// Checks whether a specified TCP port is reachable on a target IP address
bool checkPort(IPAddress ip, uint16_t port){
  WiFiClient client;
  client.setTimeout(CONNECT_TIMEOUT_MS);

  bool connected = client.connect(ip, port, CONNECT_TIMEOUT_MS);

  client.stop();

  return connected;
}

// Device Fingerprinting
void fingerprintDevice(NetworkDevice& device){
  bool hasFTP = false;
  bool hasSSH = false;
  bool hasTelnet = false;
  bool hasDNS = false;
  bool hasHTTP = false;
  bool hasHTTPS = false;
  bool hasMQTT = false;
  bool hasHTTPAlt = false;

  int score = 0;

  for (int i = 0; i < device.portCount; i++){
    uint16_t port = device.openPorts[i];

    if (port == 21){
      hasFTP = true;
      score += 10;
    }

    if (port == 22){
      hasSSH = true;
      score += 15;
    }

    if (port == 23){
      hasTelnet = true;
      score += 25;
    }

    if (port == 53){
      hasDNS = true;
      score += 10;
    }

    if (port == 80){
      hasHTTP = true;
      score += 20;
    }

    if (port == 443){
      hasHTTPS = true;
      score += 20;
    }

    if (port == 1883){
      hasMQTT = true;
      score += 25;
    }

    if (port == 8080){
      hasHTTPAlt = true;
      score += 15;
    }
  }

  if (hasMQTT && (hasHTTP || hasHTTPAlt)){
    device.deviceType = "IoT / Network Service";
  } else if (hasTelnet){
    device.deviceType = "Network Device";
  } else if (hasSSH && (hasHTTP || hasHTTPS)){
    device.deviceType = "Computer / Server";
  } else if (hasDNS && !hasHTTP && !hasHTTPS){
    device.deviceType = "Possible DNS Server";
  } else if (hasHTTP || hasHTTPS || hasHTTPAlt){
    device.deviceType = "Network Service";
  } else if (hasFTP || hasSSH){
    device.deviceType = "Possible Server";
  } else {
    device.deviceType = "Unknown";
  }

  device.fingerprintScore = score;

  Serial.print(F("Device type: "));
  Serial.println(device.deviceType);

  Serial.print(F("Fingerprint score: "));
  Serial.println(device.fingerprintScore);
}

// Scan comparison
// Looks at the device Mac address
bool sameDevice(const NetworkDevice& a, const NetworkDevice& b){
  const bool aHasMac = a.mac.length() > 0 && a.mac != "UNKNOWN";
  const bool bHasMac = b.mac.length() > 0 && b.mac != "UNKNOWN";

  if (aHasMac && bHasMac){
    String aMac = a.mac;
    String bMac = b.mac;
    aMac.toUpperCase();
    bMac.toUpperCase();
    return aMac == bMac;
  }

  return a.ip == b.ip;
}

void compareScans(){
  if (!lastScanComplete){
    Serial.println(F("[WARN] Skipping comparison: scan incomplete."));
    return;
  }

  if (!baselineReady){
    Serial.println(F("Initial complete scan: establishing baseline."));
    return;
  }

  for (int i = 0; i < currentDeviceCount; i++){
    NetworkDevice& current = currentDevices[i];
    bool foundBefore = false;

    for (int j = 0; j < previousDeviceCount; j++){
      if (sameDevice(current, previousDevices[j])){
        foundBefore = true;
        break;
      }
    }

    if (!foundBefore && !isMissingDeviceTracked(current.ip)){
      generateNewDeviceEvent(current);
    }
  }

  processMissingDeviceTracks();

  for (int i = 0; i < currentDeviceCount; i++){
    NetworkDevice& current = currentDevices[i];

    for (int j = 0; j < previousDeviceCount; j++){
      NetworkDevice& previous = previousDevices[j];

      if (!sameDevice(current, previous)){
        continue;
      }

      for (int p = 0; p < current.portCount; p++){
        uint16_t port = current.openPorts[p];
        bool existed = false;

        for (int old = 0; old < previous.portCount; old++){
          if (previous.openPorts[old] == port){
            existed = true;
            break;
          }
        }

        if (!existed) {
          generatePortChangeEvent(current, port, true);
        }
      }

      for (int old = 0; old < previous.portCount; old++){
        uint16_t port = previous.openPorts[old];
        bool stillOpen = false;

        for (int p = 0; p < current.portCount; p++){
          if (current.openPorts[p] == port) {
            stillOpen = true;
            break;
          }
        }

        if (!stillOpen){
          generatePortChangeEvent(current, port, false);
        }
      }

      break;
    }
  }
}

// Checks whether a device with the specified IP address already exist
bool currentScanContains(IPAddress ip){
  for (int i = 0; i < currentDeviceCount; i++){
    if (currentDevices[i].ip == ip){
      return true;
    }
  }

  return false;
}

// Checks whether a device with the specified IP address is already recorded in the missing-device tracking list
bool isMissingDeviceTracked(IPAddress ip){
  for (int i = 0; i < missingTrackCount; i++){
    if (missingTracks[i].ip == ip){
      return true;
    }
  }

  return false;
}

// Removes a device from the missing-device tracking list by index and shifts the remaining entries to maintain a continuous list
void removeMissingTrack(int index){
  if (index < 0 || index >= missingTrackCount){
    return;
  }

  for (int i = index + 1; i < missingTrackCount; i++){
    missingTracks[i - 1] = missingTracks[i];
  }

  missingTrackCount--;
}

// Tracks devices missing from completed scans, detects when they return
void processMissingDeviceTracks(){
  for (int i = 0; i < missingTrackCount;){
    bool deviceReturned = currentScanContains(missingTracks[i].ip);

    // Treat a device with the same MAC at a new DHCP address as returned.
    if (!deviceReturned){
      for (int p = 0; p < previousDeviceCount && !deviceReturned; p++){
        if (previousDevices[p].ip == missingTracks[i].ip){
          for (int c = 0; c < currentDeviceCount; c++){
            if (sameDevice(previousDevices[p], currentDevices[c])) {
              deviceReturned = true;
              break;
            }
          }
        }
      }
    }

    if (deviceReturned){
      if (missingTracks[i].offlineEventSent){
        addEvent(
          "DEVICE_RETURNED",
          missingTracks[i].ip.toString(),
          "Device returned after being absent from completed scans"
        );
      }

      removeMissingTrack(i);
      continue;
    }

    if (missingTracks[i].missedCompleteScans < OFFLINE_MISSED_SCANS){
      missingTracks[i].missedCompleteScans++;
    }

    if (missingTracks[i].missedCompleteScans >= OFFLINE_MISSED_SCANS &&
        !missingTracks[i].offlineEventSent){
      addEvent(
        "DEVICE_OFFLINE",
        missingTracks[i].ip.toString(),
        "Device absent from two consecutive complete scans"
      );

      missingTracks[i].offlineEventSent = true;
    }

    i++;
  }

  for (int p = 0; p < previousDeviceCount; p++){
    const IPAddress missingIP = previousDevices[p].ip;

    bool sameDeviceStillPresent = currentScanContains(missingIP);

    for (int c = 0; c < currentDeviceCount && !sameDeviceStillPresent; c++){
      if (sameDevice(previousDevices[p], currentDevices[c])){
        sameDeviceStillPresent = true;
      }
    }

    if (sameDeviceStillPresent || isMissingDeviceTracked(missingIP)){
      continue;
    }

    if (missingTrackCount < MAX_DEVICES){
      missingTracks[missingTrackCount].ip = missingIP;
      missingTracks[missingTrackCount].missedCompleteScans = 1;
      missingTracks[missingTrackCount].offlineEventSent = false;

      missingTrackCount++;
    } else {
      Serial.println(F("[WARN] Missing-device tracking capacity reached."));
    }
  }
}

// Records a new device when it is detected
void generateNewDeviceEvent(NetworkDevice& device) {
  addEvent("DEVICE_NEW", device.ip.toString(), "New device detected");
}

// Records when a device goes offline
void generateOfflineDeviceEvent(NetworkDevice& device){
  addEvent("DEVICE_OFFLINE", device.ip.toString(), "Previously seen device is offline");
}

// Records when a devices port changes
void generatePortChangeEvent(NetworkDevice& device, uint16_t port, bool opened){
  String description = opened ? "New service detected on port " + String(port) : "Previously open service closed on port " + String(port);
  addEvent(opened ? "PORT_OPENED" : "PORT_CLOSED", device.ip.toString(), description, port);
}

// Event Queue
void addEvent(const String& type, const String& ip, const String& description, int port){
  if (eventCount >= MAX_EVENTS){
    droppedEventCount++;

    Serial.println(F("[WARN] Event queue full; oldest event dropped."));

    for (int i = 1; i < MAX_EVENTS; i++){
      events[i - 1] = events[i];
    }

    eventCount = MAX_EVENTS - 1;
  }

  SecurityEvent& event = events[eventCount++];

  event.type = type;
  event.ip = ip;
  event.description = description;
  event.port = port;
  event.timestamp = millis();

  Serial.print(F("[EVENT] "));
  Serial.print(type);
  Serial.print(F(" | "));
  Serial.print(ip);
  Serial.print(F(" | "));
  Serial.println(description);
}

// Device Serial output
void printDevices(){
  Serial.println();
  Serial.println(F("Device Inventory"));

  if (currentDeviceCount == 0){
    Serial.println(F("No devices discovered."));
    return;
  }

  for (int i = 0; i < currentDeviceCount; i++){
    NetworkDevice& device = currentDevices[i];

    Serial.println();
    Serial.print(F("Device #"));
    Serial.println(i + 1);

    Serial.print(F("IP: "));
    Serial.println(device.ip);

    Serial.print(F("Hostname: "));
    Serial.println(device.hostname);

    Serial.print(F("MAC: "));
    Serial.println(device.mac);

    Serial.print(F("Vendor: "));
    Serial.println(device.vendor);

    Serial.print(F("Type: "));
    Serial.println(device.deviceType);

    Serial.print(F("Ports: "));

    if (device.portCount == 0){
      Serial.println(F("None detected"));
    } else {
      for (int p = 0; p < device.portCount; p++){
        if (p > 0){
          Serial.print(F(", "));
        }

        Serial.print(device.openPorts[p]);
      }

      Serial.println();
    }

    Serial.print(F("Fingerprint: "));
    Serial.println(device.fingerprintScore);

    Serial.print(F("First seen (uptime ms): "));
    Serial.println(device.firstSeen);

    Serial.print(F("Last seen (uptime ms): "));
    Serial.println(device.lastSeen);
  }
}

// Prints Events 
void printEvents(){
  Serial.println();
  Serial.println(F("Security Events"));

  Serial.print(F("Queued events: "));
  Serial.print(eventCount);
  Serial.print(F(" | Dropped since boot: "));
  Serial.println(droppedEventCount);

  if (eventCount == 0){
    Serial.println(F("No new events."));
    return;
  }

  for (int i = 0; i < eventCount; i++){
    Serial.print(F("["));
    Serial.print(events[i].type);
    Serial.print(F("] IP: "));
    Serial.print(events[i].ip);
    Serial.print(F(" | "));
    Serial.println(events[i].description);
  }
}

// JSON Results sent to the Dashboard
void sendResultsToServer(){
  if (WiFi.status() != WL_CONNECTED){
    lastUploadSucceeded = false;
    lastUploadHttpCode = 0;

    Serial.println(F("[ERROR] Cannot upload: Wi-Fi disconnected."));
    return;
  }

  DynamicJsonDocument doc(12000);

  doc["scanner"] = "ESP32-01";
  doc["scannerIP"] = WiFi.localIP().toString();
  doc["network"] = calculateNetworkAddress().toString();
  doc["deviceCount"] = currentDeviceCount;

  doc["scanComplete"] = lastScanComplete;
  doc["scanTruncated"] = lastScanTruncated;
  doc["scanSequence"] = scanSequence;
  doc["scanDurationMs"] = lastScanDurationMs;
  doc["scannerCapacity"] = MAX_DEVICES;

  doc["eventCapacity"] = MAX_EVENTS;
  doc["eventQueueCount"] = eventCount;
  doc["droppedEventCount"] = droppedEventCount;

  doc["previousUploadSucceeded"] = lastUploadSucceeded;
  doc["previousUploadHttpCode"] = lastUploadHttpCode;

  JsonArray deviceArray = doc.createNestedArray("devices");

  for (int i = 0; i < currentDeviceCount; i++){
    NetworkDevice& device = currentDevices[i];

    JsonObject item = deviceArray.createNestedObject();

    item["ip"] = device.ip.toString();
    item["hostname"] = device.hostname;
    item["mac"] = device.mac;
    item["vendor"] = device.vendor;
    item["type"] = device.deviceType;
    item["fingerprint"] = device.fingerprintScore;
    item["firstSeen"] = device.firstSeen;
    item["lastSeen"] = device.lastSeen;
    item["online"] = device.online;

    item["portScanStatus"] = (device.portCount > 0) ? "open ports detected" : "no open ports detected; closed/filtered/inconclusive not distinguished";

    JsonArray ports = item.createNestedArray("ports");

    for (int p = 0; p < device.portCount; p++){
      ports.add(device.openPorts[p]);
    }
  }

  JsonArray eventArray = doc.createNestedArray("events");

  for (int i = 0; i < eventCount; i++){
    SecurityEvent& event = events[i];

    JsonObject item = eventArray.createNestedObject();

    item["type"] = event.type;
    item["sourceIP"] = event.ip;
    item["port"] = event.port;
    item["service"] = "TCP";
    item["details"] = event.description;
    item["timestamp"] = event.timestamp;
  }

  String json;
  serializeJson(doc, json);

  Serial.println();
  Serial.println(F("Upload"));

  Serial.print(F("JSON bytes: "));
  Serial.println(json.length());

  if (doc.overflowed()){
    lastUploadSucceeded = false;
    lastUploadHttpCode = 0;

    Serial.println(F("[ERROR] JSON document overflowed."));
    return;
  }

  HTTPClient http;
  http.setTimeout(5000);

  if (!http.begin(SERVER_URL)){
    lastUploadSucceeded = false;
    lastUploadHttpCode = 0;

    Serial.println(F("[ERROR] Could not initialize HTTP."));
    return;
  }

  http.addHeader("Content-Type", "application/json");
  http.addHeader("Connection", "close");

  int responseCode = http.POST(json);
  lastUploadHttpCode = responseCode;

  Serial.print(F("HTTP response: "));
  Serial.println(responseCode);

  bool uploadSucceeded = false;

  if (responseCode >= 200 && responseCode < 300){
    Serial.print(F("Server reply: "));
    Serial.println(http.getString());

    uploadSucceeded = true;
  } else {
    Serial.print(F("Upload failed: "));
    Serial.println(http.errorToString(responseCode));
  }

  http.end();

  lastUploadSucceeded = uploadSucceeded;

  if (uploadSucceeded && lastScanComplete){
    eventCount = 0;

    Serial.println(F("[OK] Complete scan accepted; uploaded events cleared."));
  } else if (uploadSucceeded){
    Serial.println(F("[WARN] Partial scan accepted; event queue retained."));
  }
}

// Saves the current scan as the next baseline
void saveCurrentAsPrevious(){
  previousDeviceCount = currentDeviceCount;

  for (int i = 0; i < currentDeviceCount; i++){
    previousDevices[i] = currentDevices[i];
  }

  baselineReady = true;

  Serial.print(F("Baseline updated. Devices: "));
  Serial.println(previousDeviceCount);
}
