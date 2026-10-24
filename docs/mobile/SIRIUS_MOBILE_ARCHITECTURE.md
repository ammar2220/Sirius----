# Sirius Mobile Architecture

## 1. Recommended Framework: React Native (CLI / Bare Workflow)

### Why React Native CLI?
After evaluating Native Android, Flutter, and React Native, **React Native (Bare Workflow)** is selected for the following reasons:
- **Native API Access:** Provisioning an ESP32 requires low-level Wi-Fi management (connecting to a specific SoftAP, monitoring network changes, and managing `WifiManager`). Expo's managed workflow is too restrictive for these requirements.
- **Commercial Viability:** Provides a path to iOS support in the future without rewriting the core business logic.
- **Development Velocity:** Rapid UI iteration using TypeScript and a vast ecosystem of components.
- **Compatibility:** Works well with Android 15 (API 35) and supports the necessary native modules for mDNS (NSD) and network control.

---

## 2. Application Architecture

### High-Level Layered Design
The app follows a **Layered Architecture** to decouple the UI from the networking and business logic.

1. **UI Layer (Presentation):**
    - **Screens:** Full-page views (Home, Room, Device).
    - **Components:** Reusable UI elements (Device Card, Toggle Switch, Modal).
    - **Navigation:** React Navigation (Stack and Tab navigation).

2. **Domain Layer (Business Logic):**
    - **Use Cases:** Specific actions (e.g., `ToggleDevicePower`, `ProvisionDevice`, `RenameRoom`).
    - **Models:** TypeScript interfaces defining the structure of Homes, Rooms, and Devices.

3. **Data Layer (Infrastructure):**
    - **Repositories:** Orchestrate data between the local cache and the remote/local API.
    - **Network Services:** `LocalApiClient` (HTTP) and `DiscoveryService` (mDNS).
    - **Storage:** Local persistence for cached device metadata.

### Folder Structure
```text
mobile/sirius/
├── src/
│   ├── api/                # Network clients (Axios, mDNS)
│   │   ├── local/          # ESP32 Local API implementation
│   │   └── cloud/          # Future Cloud API implementation
│   ├── components/        # Atomic UI components
│   ├── constants/          # App constants, theme, and config
│   ├── hooks/              # Custom React hooks (e.g., useDeviceState)
│   ├── navigation/         # Navigation config and routing
│   ├── screens/            # Page-level components
│   │   ├── auth/           # Login, SignUp, Verify
│   │   ├── home/           # Dashboard, Room View
│   │   ├── device/         # Device Detail, Control
│   │   └── setup/          # Provisioning, Registration
│   ├── store/              # State management (Zustand)
│   ├── types/              # TypeScript interfaces and types
│   └── utils/              # Helpers (MAC formatting, validation)
├── android/                # Native Android project
└── ios/                    # Native iOS project (future)
```

---

## 3. Network Architecture

### Local Communication (V1)
The app communicates directly with the ESP32 over the local network.
- **Protocol:** HTTP/1.1 (Plaintext).
- **Client:** `Axios` with a strict 5-second timeout for control commands.
- **Discovery:** Uses **Android NSD (Network Service Discovery)** to resolve `aion-prototype.local` to a local IP address.
- **Reachability:** The app maintains a heartbeat by polling `/api/state` every 30 seconds when a device view is active.

### Provisioning Architecture
The "Provisioning Dance" is handled by a custom Native Module:
1. **Trigger:** User starts "Add Device".
2. **Connection:** App uses `WifiNetworkSpecifier` to connect to `AION_Prototype_ESP32`.
3. **Handshake:** App sends `POST /provision` with the target Home Wi-Fi SSID and Password.
4. **Switch:** App disconnects from SoftAP and returns to the Home Wi-Fi.
5. **Validation:** App uses mDNS to find the device on the local network and verifies identity via `/api/info`.

---

## 4. Device Data Model

### Immutable Identity
- `factoryId`: `string` (The eFuse MAC, e.g., `ESP32-80:f3:da:5e:1b:ec`).
- `firmwareVersion`: `string`.

### Mutable Metadata
- `displayName`: `string` (User-defined, e.g., "Kitchen Light").
- `roomId`: `string` (ID of the assigned room).
- `homeId`: `string` (ID of the home).

### Dynamic State
- `powerState`: `'on' | 'off' | 'unknown'`.
- `connectionStatus`: `'online' | 'offline' | 'connecting'`.
- `rssi`: `number`.
- `lastSeen`: `timestamp`.

---

## 5. State Management

**Tool:** `Zustand` (Lightweight, decoupled state).

### State Domains
- **Auth Store:** User token, profile, and authentication status.
- **Home Store:** Currently active `homeId`, list of rooms.
- **Device Store:** Map of `factoryId` $\rightarrow$ `DeviceModel`. This acts as the local cache.
- **Provisioning Store:** Current step in the setup flow, temporary credentials.

### State Sync Logic
The **Physical Device is Authoritative**.
- When a user toggles a switch:
    1. UI enters `pending` state.
    2. HTTP request is sent.
    3. On success: UI updates to the state returned by the device.
    4. On failure: UI reverts to the previous state and shows an error.

---

## 6. Local Storage
- **Technology:** `AsyncStorage` (Standard React Native key-value store).
- **Stored Data:**
    - User session token.
    - Cached list of Homes and Rooms.
    - Mapping of `factoryId` $\rightarrow$ `localIp` (to speed up reconnection).
    - App preferences (Theme, Language).

---

## 7. Authentication & Authorization Abstraction

To avoid coupling the UI to a specific provider (Firebase, AWS, etc.), the app uses an **Auth Provider Interface**:

```typescript
interface IAuthProvider {
  login(credentials): Promise<User>;
  signup(details): Promise<User>;
  logout(): Promise<void>;
  verifyEmail(code): Promise<boolean>;
  getCurrentUser(): User | null;
}
```
The UI interacts only with this interface. The concrete implementation (e.g., `FirebaseAuthProvider`) can be swapped without changing the screens.

### Roles
- **Owner:** Has `canManageHome: true`.
- **Member:** Has `canManageHome: false`.

---

## 8. Error Handling Model

Technical errors are mapped to user-friendly messages via a `MessageMapper`.

| Error Code | User Message | Action |
| :--- | :--- | :--- |
| `NETWORK_ERROR` | "No internet or local network connection." | Check Wi-Fi |
| `DEVICE_OFFLINE` | "Device is currently offline." | Check Power |
| `PROVISION_FAIL` | "Could not configure device." | Retry |
| `AUTH_REQUIRED` | "Please log in to continue." | Redirect to Login |
| `TIMEOUT` | "Device took too long to respond." | Retry |

---

## 9. Testing Strategy

### Test Levels
1. **Unit Tests (Jest):** Business logic, MAC formatting, state transitions.
2. **Integration Tests:** API client $\leftrightarrow$ Mock Server.
3. **E2E Tests (Detox):** Critical paths (Login $\rightarrow$ Add Device $\rightarrow$ Toggle).

### Emulator vs Physical Phone
| Feature | Emulator | Physical Phone | Requirement |
| :--- | :---: | :---: | :--- |
| UI/UX Layout | ✅ | ✅ | Either |
| Basic API Calls | ✅ | ✅ | Either (via IP) |
| SoftAP Connection | ❌ | ✅ | **Physical** |
| mDNS Discovery | ⚠️ | ✅ | **Physical** |
| Relay Control | ✅ | ✅ | Either |

**First E2E Test Case:**
1. Reset ESP32 $\rightarrow$ SoftAP Mode.
2. App connects to SoftAP $\rightarrow$ sends `POST /provision`.
3. ESP32 joins Home Wi-Fi $\rightarrow$ App discovers via mDNS.
4. App calls `POST /api/control` $\rightarrow$ Physical relay clicks.

---

## 10. Security Baseline (V1)
- **Network:** Local HTTP is used for simplicity in V1. All sensitive data (User credentials) are handled via HTTPS in the future cloud layer.
- **Storage:** No Wi-Fi passwords stored in plain text in `AsyncStorage`.
- **Identity:** Only the `factoryId` (MAC) is used to link physical hardware to cloud accounts.

---

## 11. Future Cloud Integration
When remote control is introduced:
1. A `CloudDeviceService` will be added to the Data Layer.
2. The app will check for a Cloud connection first; if unavailable, it will fallback to the `LocalApiClient`.
3. Control commands will be sent to a Cloud Broker (e.g., MQTT/Firebase) which then pushes the command to the ESP32.
