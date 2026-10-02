# ESP32-WIFI-TOOLS
Wifi scanner, Web dashboard, and honey pot devices I made for my home and personal uses

A a network scanner and monitoring project that I have been developing on with ESP32 microcontrollers. 

This project used three separate ESP 32's to scan the local WiFi network and identify devices that are currently reachable. Scan result are then sent to another ESP that hosts a web server and providing a simple dashboard that the user can connect to that shows the discovered devices on the local network. 

Features of this project:
- WiFi Scanning
- Detect Avaible Devices
- Collects network information from the devices
- Sends results and communicate between ESP 32
- Uses HTTP and JSON for communication
- ESP32-hosted web dashboard
- Displays discovered devices through a browser

Hardware:
- 2-3 ESP 32
- Wifi network
- USB cables(for uploading software and power)

Software:
- Arduino IDE
- ESP32 Arduino Core
- C++
- Wi-Fi
- HTTP
- JSON
- HTML/CSS/JavaScript
