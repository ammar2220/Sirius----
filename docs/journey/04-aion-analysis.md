# 04 - AION Analysis
**Date:** 2026-09-11
**Status:** Completed

## Objective
Analyze the AION APK to understand its UX, provisioning flow, and communication protocol to use as a benchmark for Sirius.

## Methodology
- Static analysis of the React Native JS bundle.
- Dynamic analysis using an Android 15 emulator and `adb logcat`.
- Network inspection via `netstat` and `curl`.

## Key Findings
- **Backend:** Heavily reliant on Firebase (Auth, Realtime Database, Messaging).
- **Identity:** Uses a `USID` for unique device identification.
- **Provisioning:** Employs a "social provisioning" flow via SoftAP, followed by cloud registration.
- **Control:** Control signals appear to flow through Firebase RTDB paths (e.g., `devicesByRoom`) rather than local REST APIs.

## Conclusion
AION is an excellent UX reference for simplicity and organization, but its private cloud-centric backend is proprietary. Sirius will adopt the UX principles but implement its own independent architecture.
