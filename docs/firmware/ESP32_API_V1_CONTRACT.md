# ESP32 API V1 Contract

This document defines the current implementation of the ESP32 local API. This is the source of truth for the current prototype firmware.

## Base Configuration
- **Port:** 80
- **Protocol:** HTTP

## SoftAP Provisioning API
Available only when the device is in `STATE_SOFTAP`.

### POST /provision
Provisions the device with Wi-Fi credentials.
- **Payload:** `{"ssid": "<string>", "pass": "<string>"}`
- **Success Response:** `200 OK` | `{"status":"connecting"}`
- **Failure Response:** `400 Bad Request` | `{"status":"error", "msg":"..."}`
- **Behavior:** Saves credentials to NVS (`aion-proto` namespace) and restarts the device to enter Station Mode.

### GET /status
Checks the current operational state.
- **Success Response:** `200 OK` | `{"state":"provisioning"}`

---

## Station Mode API
Available when the device is in `STATE_STA`.

### GET /api/info
Returns the unique factory identity of the device.
- **Success Response:** `200 OK` | `{"id":"ESP32-80:f3:da:5e:1b:ec", "ver":"1.0.0"}`

### GET /api/state
Returns the current state of the relay and signal strength.
- **Success Response:** `200 OK` | `{"power":"on"|"off", "rssi": <int>}`

### POST /api/control
Controls the physical relay.
- **Payload:** `{"power":"on"}` or `{"power":"off"}`
- **Success Response:** `200 OK` | `{"status":"ok", "state":"on"|"off"}`
- **Failure Response:** `400 Bad Request` | `{"status":"error"}`

---

## Device Management

### Factory Reset
- **Trigger:** Hold physical BOOT button (GPIO 0) for 5 seconds.
- **Action:** Clears `ssid` and `pass` from NVS.
- **Result:** Device reboots into `STATE_SOFTAP` mode.

### Connectivity
- **mDNS Hostname:** `aion-prototype.local`
- **SoftAP SSID:** `AION_Prototype_ESP32`
- **SoftAP IP:** `192.168.4.1`
