# 03 - WiFi Provisioning
**Date:** 2026-09-11
**Status:** Completed

## Implementation
Implemented a SoftAP-based provisioning flow.
- **SoftAP SSID:** `AION_Prototype_ESP32`
- **Endpoint:** `POST /provision`
- **Mechanism:** Accepts SSID and Password, saves them to NVS, and restarts into Station Mode.

## Verification
Successfully tested the flow: SoftAP $\rightarrow$ Provision $\rightarrow$ Reboot $\rightarrow$ Home WiFi Connection.
Verified mDNS (`aion-prototype.local`) functionality for local discovery.
