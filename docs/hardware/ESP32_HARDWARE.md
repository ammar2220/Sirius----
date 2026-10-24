# ESP32 Hardware Specification

## Components
- **Microcontroller:** ESP32-D0WD-V3 (Revision v3.1)
- **USB-to-UART:** CP2102
- **Form Factor:** ESP32-S 30-pin development board

## Pin Mapping
| Component | Pin | Type | Notes |
| :--- | :--- | :--- | :--- |
| Relay | GPIO 2 | OUTPUT | Controls the physical load |
| Reset Button | GPIO 0 | INPUT_PULLUP | BOOT button used for factory reset |

## Device Identity
- **Identity Source:** eFuse MAC
- **Verified MAC:** `80:f3:da:5e:1b:ec`
- **ID Format:** `ESP32-<MAC>`
