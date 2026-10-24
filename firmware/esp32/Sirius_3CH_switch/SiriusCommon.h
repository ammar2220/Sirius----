#ifndef SIRIUS_COMMON_H
#define SIRIUS_COMMON_H

#include <WiFi.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <Preferences.h>
#include <WebSocketsServer.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include "mqtt_config.h"

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
struct CommandResult {
  bool success;
  const char* status;
  const char* reason;
  uint32_t resultingRev;
};

void setupCommon();
void loopCommon();
void factoryReset();
void logState(const char* msg);
void setChannelState(int index, bool state);
void broadcastStateEvent();
CommandResult processUnifiedCommand(const char* payload, size_t payloadLength, bool isLocal, const char* mqttTopic = nullptr);
bool updateBootCount();

// --- API Handlers ---
void handleProvision();
void handleStatus();
void handleInfo();
void handleState();
void handleControl();
void handleUnifiedCommand();
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
extern uint32_t bootCount;

#endif
