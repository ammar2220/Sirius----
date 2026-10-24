#ifndef SIRIUS_COMMON_H
#define SIRIUS_COMMON_H

#include <WiFi.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <Preferences.h>
#include <WebSocketsServer.h>

// --- Product Types ---
struct Channel {
  int pin;
  bool state;
  const char* name;
};

// --- External Product Configuration (defined in .ino) ---
extern int CHANNEL_COUNT;
extern const char* PRODUCT_NAME;
extern const char* PRODUCT_SSID;
extern Channel channels[];

// --- Constants ---
const int RESET_PIN = 0;
extern const int BUTTON_PINS[];
const int LED_PIN = 2;
extern const char* FIRMWARE_VERSION;
const char* const MDNS_HOSTNAME = "aion-prototype";
const char* const SOFTAP_PASS = "";

// --- Common Logic Prototypes ---
void setupCommon();
void loopCommon();
void factoryReset();
void logState(const char* msg);
void setChannelState(int index, bool state);
void broadcastStateEvent();

// --- API Handlers ---
void handleProvision();
void handleStatus();
void handleInfo();
void handleState();
void handleControl();
void handleResetProvisioning();
void registerRoutes(int currentState);

enum DeviceState {
  STATE_BOOT,
  STATE_SOFTAP,
  STATE_STA
};

extern DeviceState currentState;
extern WebServer server;
extern WebSocketsServer wsServer;
extern String wifiSsid;
extern String wifiPass;
extern unsigned long lastBlinkTime;
extern bool ledState;
extern uint32_t stateRevision;
extern String sessionId;

#endif
