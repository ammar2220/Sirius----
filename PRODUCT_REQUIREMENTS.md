# Sirius Product Requirements

## Project Goal
Sirius is a commercial smart-home product family based in Egypt, focusing on high-quality, intuitive smart-switch hardware and software.

## Hardware Requirements (V1)
- **Form Factor:** Standard vertical physical size approx. 15 cm x 8 cm.
- **Interface:** Horizontal capacitive touch interface.
- **Electrical:**
    - No neutral wire required (2-wire switch-box approach).
    - Separate 2-wire supply.
    - Target lighting load: ~2 kW.
    - Mechanical relays.
- **I/O:**
    - Relay starts OFF by default until state is determined.
    - Visual LED indication.
    - Hidden physical reset button.
- **Control:**
    - One touch toggles the channel.
    - Long press reserved for future use.
    - No buzzer.

## Software & UX Requirements (V1)
- **Connectivity:** Local Wi-Fi/LAN control.
- **Device Management:**
    - Unique factory device identity.
    - User-friendly device naming.
    - Dynamic channel architecture.
    - Presence indicator (Online/Offline).
    - Last-seen timestamp.
- **User Model:**
    - Owner/Member model.
    - Owner can invite members via email.
    - Multiple homes per account.
- **Security:**
    - Email verification.
    - Password reset.
    - Password lockout: 5 failed attempts $\rightarrow$ 5 min lockout.
- **Data:**
    - History retention target: 60 days (configurable).
- **Out of Scope (V1):**
    - Scenes/Automation.
    - Common-neutral versions.

## Design Principles
- Simple, Clean, Intuitive.
- Fast response times.
- Minimal onboarding friction.
- Clear state representation.
