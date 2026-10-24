# AION UX Reference

**Disclaimer:** AION is used strictly as a UX/reference benchmark. No proprietary code, credentials, or private infrastructure from AION are used in the Sirius project.

## Observed UX Characteristics
- **Onboarding:** Simple "Add Device" flow with Wi-Fi credential entry.
- **Organization:** Hierarchy of Home $\rightarrow$ Room $\rightarrow$ Device.
- **Interaction:** Simple toggles for power state; clean, modern visual style.
- **Feedback:** Clear "Online/Offline" indicators and state-based responses.

## Architecture Reference
- **Identity:** Uses a unique `USID` for every device.
- **Control Plane:** Appears to be cloud-centric (Firebase Realtime Database) rather than local-first.
- **Provisioning:** Social provisioning flow (SoftAP $\rightarrow$ Cloud Registration).
