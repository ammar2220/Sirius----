# Sirius UX Specification

## 1. App Information Architecture

### V1 Implementation
**Authentication Layer**
- **Welcome Screen:** Introduction and value proposition.
- **Login Screen:** Email and password entry.
- **Create Account:** Name, email, password.
- **Forgot Password:** Email-based reset request.
- **Email Verification:** Verification code entry.

**Main Application Layer**
- **Home Screen (Dashboard):** 
    - Home Selector (Dropdown/Picker for multi-home support).
    - Room List (Quick access to rooms).
    - Favorite/Recent Devices (Optional quick toggles).
- **Room View:**
    - Room Header (Name, Edit).
    - Device Grid (All devices assigned to the room).
- **Device Control View:**
    - Power Toggle (Primary action).
    - Device Info (Name, ID, RSSI).
    - State Indicator (Online/Offline).
    - Settings Access.
- **Add Device Flow:**
    - Provisioning Setup.
    - Connection Progress.
    - Device Registration (Naming, Room Assignment).
- **Profile & Settings:**
    - User Profile (Name, Email).
    - Account Settings (Password change, Logout).
    - Home Management (Create Home, Rename Home, Manage Members).
    - App Settings (Notifications, Theme).

### V2/Future
- **Automation/Scenes:** Interface for creating and triggering scenes.
- **Advanced Analytics:** Detailed history graphs.
- **Detailed Device Settings:** LED brightness, timer settings.

---

## 2. Navigation Model

### Main Navigation
**Top-Level:**
- **Home Dashboard** (Default)
- **Profile/Account** (Right-top icon or Bottom Tab)

**Structural Flow:**
`Home Dashboard` $\rightarrow$ `Room View` $\rightarrow$ `Device Control View`

### Interaction Patterns
- **Secondary Navigation:** Bottom sheet for "Add Device" or "Room Settings".
- **Back Behavior:** Standard Android back navigation (Device $\rightarrow$ Room $\rightarrow$ Home).
- **Modal Screens:** Used for critical confirmations (e.g., "Delete Room", "Remove Device").
- **Empty States:** 
    - No Homes: "Welcome! Let's create your first home."
    - No Rooms: "Add a room to organize your devices."
    - No Devices: "Your room is empty. Add a device to get started."

---

## 3. Onboarding Flow (First-Time Experience)

### User Journey: Install to Ready
1. **Install & Launch:** User opens app $\rightarrow$ **Welcome Screen**.
2. **Account Setup:** User chooses **Login** or **Create Account**.
3. **Verification:** User verifies email $\rightarrow$ **Auth Success**.
4. **Home Creation:** User creates their first Home (e.g., "My Apartment").
5. **Device Addition:** User taps **"+" (Add Device)**.
6. **Provisioning Mode:** App instructs user to hold the physical reset button on the ESP32.
7. **Discovery:** App scans for the `AION_Prototype_ESP32` SoftAP.
8. **Credential Handshake:** App prompts for Home Wi-Fi SSID/Pass $\rightarrow$ sends to device via `POST /provision`.
9. **Connection Wait:** App monitors for the device to appear on the local network via mDNS (`aion-prototype.local`).
10. **Registration:** App prompts user to:
    - Name the device (e.g., "Living Room Light").
    - Assign to a Room (e.g., "Living Room").
11. **Ready:** Device appears in the Room View; control is enabled.

### Error Handling
- **Wrong Wi-Fi Pass:** App detects device failed to join $\rightarrow$ Prompt for credentials again.
- **Discovery Timeout:** App suggests checking if the device is powered and in SoftAP mode.
- **Registration Fail:** App allows retrying the naming/room assignment without re-provisioning.

---

## 4. Device States & UX

### Connection States
| State | Visual Indicator | App Behavior |
| :--- | :--- | :--- |
| **Online** | Green dot / Solid icon | Full control enabled. |
| **Offline** | Grey dot / Dimmed icon | Controls disabled or "Optimistic" with warning. |
| **Connecting** | Pulsing blue icon | "Connecting..." overlay; controls disabled. |
| **Provisioning** | Yellow icon | "Setting up device..." progress bar. |
| **Unknown** | Question mark icon | "Unable to reach device". |
| **Error** | Red exclamation | "Connection Error" notification. |

### Metadata Display
- **Last Seen:** "Last seen 2 mins ago" (visible in Device View).
- **Power State:** Clear "ON" / "OFF" text or distinct color changes.
- **Signal Strength:** RSSI displayed as a simple signal bar icon.

---

## 5. Device Control Logic

### Primary Action: Toggle
- **Tapping the switch:** Sends a request to toggle the state.
- **Visual Feedback:** UI updates immediately to "Pending" state $\rightarrow$ confirms to "ON/OFF" upon device response.

### Edge Case Handling
- **Control while Offline:**
    - User taps ON $\rightarrow$ App shows "Device Offline" toast $\rightarrow$ Command not sent.
- **External Change (Physical Switch):**
    - App polls `/api/state` or uses a websocket (Future) $\rightarrow$ UI updates in real-time to match physical state.
- **Concurrent Control:**
    - Last command sent wins. UI reflects the final state returned by the device.
- **Reconnection:**
    - When a device returns to `Online` state, the app automatically refreshes the power state to ensure consistency.

---

## 6. Room & Home Structure

### Hierarchy
`Account` $\rightarrow$ `Homes` $\rightarrow$ `Rooms` $\rightarrow$ `Devices`

### Management
- **Adding Room:** User taps "+" in Home View $\rightarrow$ enters name $\rightarrow$ Room created.
- **Renaming:** Long-press room card $\rightarrow$ Edit Name $\rightarrow$ Save.
- **Moving Device:** Device Settings $\rightarrow$ Change Room $\rightarrow$ Select new room.
- **Deletion:** Delete Room $\rightarrow$ Prompt to "Move devices to another room" or "Delete devices".

---

## 7. User Roles & Permissions

| Permission | Owner | Member |
| :--- | :---: | :---: |
| Control Devices | ✅ | ✅ |
| Rename Devices | ✅ | ❌ |
| Manage Rooms | ✅ | ❌ |
| Invite Members | ✅ | ❌ |
| Remove Members | ✅ | ❌ |
| Home Settings | ✅ | ❌ |
| Delete Home | ✅ | ❌ |

- **Member Invitation:** Owner enters email $\rightarrow$ Member receives invitation $\rightarrow$ Accept $\rightarrow$ Joined to Home.

---

## 8. Device Identity Model

### Immutable Identity
- **Factory ID:** The unique eFuse MAC (e.g., `ESP32-80:f3:da:5e:1b:ec`).
- **Purpose:** Used for backend registration and internal tracking. Never shown to the user in primary views.

### Mutable Identity
- **Device Name:** User-defined string (e.g., "Kitchen Light").
- **Purpose:** User-facing label.
- **UX:** Easily editable via Device Settings.

---

## 9. Multi-Home Support
- **Home Selector:** A dropdown in the top-left of the Home Screen.
- **Switching:** Selecting a different home instantly updates the Room/Device list.
- **Ownership:** The user who creates the home is the `Owner`.

---

## 10. Error & Empty States

| State | UI Representation | User Action | Recovery |
| :--- | :--- | :--- | :--- |
| **No Homes** | Illustration + "Create your first home" | Tap "Create Home" | Redirects to Home Setup |
| **No Rooms** | "Your home is empty" | Tap "Add Room" | Redirects to Room Setup |
| **No Devices** | "No devices in this room" | Tap "Add Device" | Starts Provisioning Flow |
| **Offline** | Greyed out device card | Tap $\rightarrow$ "Device is offline" | Check Power/Router |
| **Provision Fail** | "Failed to connect to device" | Tap "Retry" | Re-enters Handshake |
| **Wrong Pass** | "Invalid Wi-Fi Password" | Edit Password | Retry Handshake |
| **Timeout** | "Device took too long to respond" | Tap "Retry" | Re-scan for device |

---

## 11. Offline Behavior (V1 Local-First)

- **Phone without Internet:** App can still control devices if connected to the same local Wi-Fi via local IP/mDNS.
- **ESP32 Offline:** App shows "Offline" state; control is disabled.
- **Physical Switch Change:** The app periodically polls `/api/state` to reflect the physical state on the UI.
- **No Backend:** In V1, the app stores the mapping of `Device Name $\rightarrow$ IP Address` locally.

---

## 12. Visual Design Principles

### Aesthetic
- **Modern & Premium:** High contrast, clean lines, ample whitespace.
- **Reliable:** Stable animations, clear confirmation of actions.

### UI Components
- **Device Cards:** Rounded corners, subtle shadows, large toggle switch.
- **Room Cards:** Large, image-background (optional), centered name.
- **Status Indicators:**
    - **Online:** Solid Green dot.
    - **Offline:** Hollow Grey circle.
    - **Connecting:** Rotating blue spinner.
- **Colors:**
    - **Primary:** Professional Deep Blue / Slate.
    - **Active:** Vibrant Teal / Gold for "ON".
    - **Error:** Soft Red.
- **Typography:** Sans-serif, clear hierarchy (Bold headers, muted sub-text).
- **Dark Mode:** Supported in V1 to match modern smart-home app trends.

---

## 13. V1 vs Future

| Feature | V1 (MVP) | V2 | Future |
| :--- | :---: | :---: | :---: |
| Basic Lighting Control | ✅ | ✅ | ✅ |
| Local Wi-Fi Control | ✅ | ✅ | ✅ |
| Multi-channel Support | ✅ | ✅ | ✅ |
| Room Organization | ✅ | ✅ | ✅ |
| Owner/Member Model | ✅ | ✅ | ✅ |
| Multi-Home Support | ✅ | ✅ | ✅ |
| SoftAP Provisioning | ✅ | ✅ | ✅ |
| Device History (60d) | ✅ | ✅ | ✅ |
| Cloud Remote Control | ❌ | ✅ | ✅ |
| Scenes & Automation | ❌ | ✅ | ✅ |
| OTA Updates | ❌ | ✅ | ✅ |
| AC Control | ❌ | ❌ | ✅ |
| Advanced Analytics | ❌ | ❌ | ✅ |

---

## 14. User Journeys

### Journey 1: New User Setup
`App Install` $\rightarrow$ `Welcome` $\rightarrow$ `Create Account` $\rightarrow$ `Verify Email` $\rightarrow$ `Create Home ("My Home")` $\rightarrow$ `Home Dashboard (Empty)`.

### Journey 2: Adding a Device
`Home Dashboard` $\rightarrow$ `Add Device` $\rightarrow$ `Enter Provisioning Mode` $\rightarrow$ `Connect to SoftAP` $\rightarrow$ `Submit Home Wi-Fi` $\rightarrow$ `Wait for Connection` $\rightarrow$ `Name Device ("Living Room Light")` $\rightarrow$ `Assign to Room ("Living Room")` $\rightarrow$ `Home Dashboard`.

### Journey 3: Controlling a Light
`Home Dashboard` $\rightarrow$ `Select Living Room` $\rightarrow$ `Tap Light Switch` $\rightarrow$ `UI shows Pending` $\rightarrow$ `Device Relay Clicks` $\rightarrow$ `UI shows ON`.

### Journey 4: Device Goes Offline
`Device is Online` $\rightarrow$ `Power cut to ESP32` $\rightarrow$ `App polls /api/state` $\rightarrow$ `Timeout` $\rightarrow$ `UI updates to Grey/Offline` $\rightarrow$ `Control disabled`.

### Journey 5: Physical State Change
`App shows OFF` $\rightarrow$ `User presses physical switch` $\rightarrow$ `App polls /api/state` $\rightarrow$ `Response: "on"` $\rightarrow$ `UI updates to ON`.

---

## 15. Implementation Screen Inventory

| Screen | Purpose | Entry Point | Main Actions | Nav | Data Req. | V1 |
| :--- | :--- | :--- | :--- | :--- | :--- | :---: |
| **Welcome** | Intro | Launch | Login / Sign Up | $\rightarrow$ Login | None | ✅ |
| **Login** | Auth | Welcome | Enter creds | $\rightarrow$ Home | Email, Pass | ✅ |
| **SignUp** | Account | Welcome | Enter details | $\rightarrow$ Verify | Email, Pass | ✅ |
| **HomeDash** | Overview | Login / Home | Select Room, Add Device | $\rightarrow$ Room | Home List, Rooms | ✅ |
| **RoomView** | Room Ctrl | HomeDash | Toggle Device, Edit Room | $\rightarrow$ Device | Devices in Room | ✅ |
| **DeviceView** | Detail Ctrl | RoomView | Toggle Power, Rename | $\rightarrow$ Room | Device State, ID | ✅ |
| **Provision** | Setup | HomeDash | Enter Wi-Fi creds | $\rightarrow$ Home | SSID, Pass | ✅ |
| **RegDevice** | Identity | Provision | Name device, pick room | $\rightarrow$ Room | USID, Room List | ✅ |
| **Profile** | Account | HomeDash | Change pass, Logout | $\rightarrow$ Home | User Info | ✅ |
| **HomeMgmt** | Home Org | Profile | Add Home, Invite Member | $\rightarrow$ Profile | Homes, Members | ✅ |
