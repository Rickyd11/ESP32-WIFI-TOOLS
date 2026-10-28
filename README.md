# ESP32-WIFI-TOOLS
Wifi scanner, Web dashboard, and honey pot devices I made for my home and personal uses

An ESP32 based wifi discovery, network monitoring, and eventually a honeypot system to the network. This is a simple homelab setup and further cybersecurity research. 

ESP32 is a personal wifi-tool and security project built around multiple micro controllers. It combines local network device discovery, HTML/ JSON communication, and a browser based monitoring dashboard. The main goal of this project is to make monitoring local network activity more easier to observe as well as understand the microcontrollers and build a self hosted web interface.

# Overview

The system has multiple micro controller that will handle the dashboard, scanning, discovery, and communicating the results to other devices. 

The typical deployment consist of one ESP32 preforming the network scan and then sends the results to the dashboard. Which is hosted on the separate ESP32 and displays that information allowing for the user to view the local network and devices on the network. 

# How it Works
1. Discovery - The scanner checks the local network for devices that are reachable
2. Collect - The scanner gathers all available information about the device on the network
3. Send - The scanner sends the information it gathers to the dashboard using HTTP requests and JSON dat
4. Monitor - The dashboard displays reported devices and available monitoring information
5. Observe - See the information that has been collected (Later having notification for secuirty and other events sent to personal phone)

# Current Limitations
- Device Discovery: A device may be online but not responding to the scanner's discovery method
- Hostname resolution: Hostnames are not available for every device
- Manufacturer identification: MAC address prefixes can sometimes indicate a registered manufacturer, but they do not reliably identify an exact model
- Network compatibility: Discovery behavior depends on Wi-Fi configuration, subnet settings, network isolation, and firmware support
- Monitoring scope: The dashboard can only display information collected by the scanner or other configured components

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

# Contributing
This is a personal development project, but suggestions, bug reports, and technical feedback are welcome.

