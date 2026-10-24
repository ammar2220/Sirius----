# 02 - ESP32 Prototype
**Date:** 2026-09-11
**Status:** Completed

## Hardware Setup
A prototype was built using an ESP32-D0WD-V3 development board.
- Relay connected to GPIO 2.
- Factory reset button mapped to GPIO 0 (BOOT).

## Initial Implementation
The first version of the firmware focused on basic connectivity and relay control.
- Implemented a basic WebServer.
- Established a three-state machine: `STATE_BOOT`, `STATE_SOFTAP`, `STATE_STA`.
- Implemented manual JSON parsing for provisioning.
