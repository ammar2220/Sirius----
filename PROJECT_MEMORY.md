# Sirius Project Memory

## Current Status
* **Current Project Phase:** Phase 4: Sirius Mobile Application MVP Implementation.
* **Current Working State:** Real React Native project initialized. Integration of LocalApiClient and HardwareTestScreen complete. AndroidManifest configured for cleartext traffic. Gradle build in progress.
* **What is Verified:** 
    * React Native project structure created.
    * Dependencies (axios, etc.) installed.
    * AndroidManifest configured for HTTP.
    * Android emulator (API 35) detected.
* **What is Not Verified:** 
    * Long-term stability of the prototype.
    * Mobile app implementation.
    * Commercial hardware constraints (no neutral wire).
* **Current Active Task:** Finalizing architecture documentation.
* **Immediate Next Task:** Phase 4: Sirius Mobile Application MVP Implementation.

## Project Goal
Sirius is intended to become a commercial Egyptian smart-home / smart-switch product.

**Initial Focus:**
* Smart lighting and light loads.
* ESP32-based hardware.
* Mobile application.
* Simple and intuitive UX.
* Local Wi-Fi/LAN control initially.

**Future Possibilities:**
* AC control.
* expanded product line.
* Scenes and automation.
* Additional smart-home devices.

## Product Vision
### V1 Requirements
* **Hardware:**
    * 1 / 2 / 3 channel switch family.
    * Standard vertical physical size (~15 cm × 8 cm).
    * Horizontal capacitive touch interface.
    * No neutral wire required (2-wire switch-box approach).
    * Separate 2-wire supply.
    * Target load: ~2 kW lighting.
    * Mechanical relays (starts OFF by default).
    * Visual LED indication.
    * Hidden physical reset button.
* **Firmware/Control:**
    * One touch toggles the channel.
    * Long press reserved for future functionality.
    * Local Wi-Fi/LAN control.
    * Device online/offline state and last-seen info.
    * Physical state is authoritative during offline changes.
    * Unique factory device identity.
* **User Model:**
    * Owner/member model.
    * Owner can invite members by email.
    * Multiple homes per account.
    * User-friendly device naming.
    * Dynamic channel architecture.
    * Channel-specific notifications.
    * Presence indicator.
    * History retention: 60 days (configurable).
* **Security:**
    * Email verification and password reset.
    * Password lockout after 5 failed attempts for 5 minutes.
* **Out of Scope for V1:**
    * Scenes (Reserved for V2).
    * Common-neutral product version.

### Status Matrix
| Feature | Status |
| :--- | :--- |
| Local Wi-Fi Control | **VERIFIED** (Prototype) |
| Device Identity (MAC) | **VERIFIED** (Prototype) |
| Relay Control (GPIO2) | **VERIFIED** (Prototype) |
| Factory Reset | **VERIFIED** (Prototype) |
| SoftAP Provisioning | **VERIFIED** (Prototype) |
| No-Neutral Hardware | **NOT YET DECIDED** |
| User/Member Model | **PROPOSED** |
| Mobile App | **PROPOSED** |
| Room/Home Org | **PROPOSED** |

## Claude Session Recovery Rules
1. At the beginning of every new session, read `PROJECT_MEMORY.md`.
2. Then read `ROADMAP.md`.
3. Then inspect the current filesystem.
4. Never assume an item is complete without evidence.
5. Never claim a hardware test was performed unless it actually was.
6. Never erase working code.
7. Never flash the ESP32 without explicit instruction.
8. Never change working firmware without explicit approval.
9. Never store Wi-Fi passwords in project documentation.
10. Never use AION's private backend or credentials.
11. Treat AION as UX/reference only.
12. Update `PROJECT_MEMORY.md` after meaningful milestones.
13. Record important decisions and why they were made.
14. Record failed approaches when they are useful for preventing repeated mistakes.
15. Preserve working baselines.
16. Prefer copying over moving when project organization is uncertain.
