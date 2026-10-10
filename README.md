# IoT Gateway Honeypot Using ESP32

An educational IoT security prototype. An ESP32 hosts a small HTTP service that records requests to selected gateway-like paths and forwards event metadata to a Python/Flask collector. The collector stores events in SQLite and presents a local dashboard.

> **Safety and scope:** This is a learning prototype, not a production IDS or a hardened gateway. Run it only on a private lab network you own or are authorized to test. Do not expose the ESP32 service or the collector to the public Internet.

## Repository layout

```text
firmware/IoT_Gateway_Honeypot.ino  ESP32 firmware
server/server.py                  Flask collector and dashboard
server/requirements.txt            Python dependencies
docs/architecture.mmd             Mermaid architecture diagram
```

## Hardware

- ESP32 DevKit V1 / ESP-WROOM-32
- USB data cable
- Wi-Fi router and laptop
- Optional breadboard, jumper wires, red LED, and a suitable series resistor

## 1. Configure and upload the ESP32 firmware

1. Install Arduino IDE and the `esp32` board package by Espressif Systems.
2. Open `firmware/IoT_Gateway_Honeypot.ino`.
3. Set `WIFI_SSID` and `WIFI_PASSWORD` to the lab Wi-Fi credentials. Do not commit real credentials to a public repository.
4. Find the laptop's private IPv4 address and update `LOG_SERVER_URL`, for example `http://192.168.1.20:5000/api/log`.
5. Select the matching ESP32 board and serial port in Arduino IDE, then upload.
6. Open Serial Monitor at **115200 baud** and note the IP address printed by the ESP32.

## 2. Run the Python collector

From the repository root, run:

```bash
python -m venv .venv
```

Activate the environment, then install dependencies and run the server:

```bash
python -m pip install -r server/requirements.txt
python server/server.py
```

The dashboard is available at `http://127.0.0.1:5000` on the laptop. For ESP32 access, use the laptop's private LAN IP and allow Python on the private-network firewall only if required.

## 3. Test in the authorized lab

From another device on the same lab Wi-Fi, browse to the ESP32 IP and try the provided paths such as `/`, `/admin`, `/config`, `/debug`, and `/login`. Then open the collector dashboard on the laptop and verify that events appear. The collector classifies the listed sensitive paths as `SUSPICIOUS` using simple path matching; this is not machine-learning detection and should not be described as a full IDS.

## Current status

The source files are a starting prototype. End-to-end operation has **not yet been verified**. Record actual upload results, event forwarding, and dashboard screenshots during testing before marking tests as passed in the report.

## Privacy

Do not commit Wi-Fi passwords, access tokens, private addresses that should not be public, database files, virtual environments, or personal data. Keep real credentials local and out of Git history.
