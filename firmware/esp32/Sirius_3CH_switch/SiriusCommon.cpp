#include "SiriusCommon.h"
#include <WiFiClientSecure.h>
#include <PubSubClient.h>

DeviceState currentState = STATE_BOOT;
WebServer server(80);
WebSocketsServer wsServer = WebSocketsServer(81);
WiFiClientSecure secureClient;
PubSubClient mqttClient(secureClient);

String wifiSsid = "";
String wifiPass = "";
unsigned long lastBlinkTime = 0;
bool ledState = false;
uint32_t stateRevision = 0;
String sessionId = "";
uint32_t bootCount = 0;
unsigned long lastMqttStatePublish = 0;
const unsigned long MQTT_STATE_HEARTBEAT_MS = 30000;

// --- Command Deduplication Cache ---
// Command Identity Definition:
// A command is identical if ALL of the following match:
// - commandId, deviceId, expectedBootCount, expectedSessionId, expectedRevision,
//   expiresAt, targetChannel, targetValue, and targetAction ('SET_STATE').
// Metadata fields (source, timestamp, correlationId) are excluded from identity
// as they do not affect the resulting state or the safety fence.
struct CommandRecord {
  char commandId[37];
  char deviceId[32];
  uint32_t enrollmentGeneration;
  uint32_t expBoot;
  char expSess[32];
  uint32_t expRev;
  long long expiresAt;
  int targetChannel;
  bool targetValue;
  // Result for ACK reproduction
  char status[16];      // 'accepted', 'rejected'
  uint32_t resultingRev;
  char reason[32];       // Bounded reason codes (e.g., "FENCE_FAIL")
  bool active = false;
};
CommandRecord cmdCache[10];

String mqttFactoryId() {
  uint64_t chipid = ESP.getEfuseMac();
  char macStr[18];
  snprintf(macStr, sizeof(macStr), "%02x:%02x:%02x:%02x:%02x:%02x",
           (uint8_t)chipid, (uint8_t)(chipid >> 8), (uint8_t)(chipid >> 16),
           (uint8_t)(chipid >> 24), (uint8_t)(chipid >> 32), (uint8_t)(chipid >> 40));
  return "ESP32-" + String(macStr);
}

void publishMqttPresence(const char* status) {
  if (!mqttClient.connected()) return;

  StaticJsonDocument<256> doc;
  doc["enrollmentGeneration"] = MQTT_ENROLLMENT_GENERATION;
  doc["bootCount"] = bootCount;
  doc["sessionId"] = sessionId;
  doc["status"] = status;

  char payload[256];
  serializeJson(doc, payload, sizeof(payload));
  const String topic = "sirius/devices/" + mqttFactoryId() + "/presence";
  if (!mqttClient.publish(topic.c_str(), payload, true)) {
    Serial.printf("[MQTT] Presence publish failed: %s\\n", topic.c_str());
  }
}

void publishMqttState() {
  if (!mqttClient.connected()) return;

  StaticJsonDocument<512> doc;
  doc["enrollmentGeneration"] = MQTT_ENROLLMENT_GENERATION;
  doc["bootCount"] = bootCount;
  doc["sessionId"] = sessionId;
  doc["revision"] = stateRevision;
  doc["rssi"] = WiFi.RSSI();
  JsonArray channelStates = doc.createNestedArray("channels");
  for (int i = 0; i < CHANNEL_COUNT; i++) {
    JsonObject channel = channelStates.createNestedObject();
    channel["index"] = i + 1;
    channel["state"] = channels[i].state ? "on" : "off";
  }

  char payload[512];
  serializeJson(doc, payload, sizeof(payload));
  const String topic = "sirius/devices/" + mqttFactoryId() + "/state";
  if (!mqttClient.publish(topic.c_str(), payload, false)) {
    Serial.printf("[MQTT] State publish failed: %s\\n", topic.c_str());
  }
}

// Physical Button State
unsigned long lastButtonPressTime[3] = {0, 0, 0};
bool lastButtonState[3] = {HIGH, HIGH, HIGH};
const unsigned long DEBOUNCE_DELAY = 50;

const int BUTTON_PINS[] = {4, 19, 18};
const char* FIRMWARE_VERSION = "1.0.0";

void logState(const char* msg) {
  Serial.printf("[STATE] %s\\n", msg);
}

bool updateBootCount() {
  Preferences preferences;
  if (!preferences.begin("sirius-sys", false)) {
    Serial.println("[SYS] NVS failure: Could not open sirius-sys namespace");
    return false;
  }

  uint32_t currentCount = preferences.getUInt("bootCount", 0);
  if (currentCount == UINT32_MAX) {
    Serial.println("[SYS] Boot count overflow (UINT32_MAX). Failing closed.");
    preferences.end();
    return false;
  }

  uint32_t nextCount = currentCount + 1;
  if (!preferences.putUInt("bootCount", nextCount)) {
    Serial.println("[SYS] NVS failure: putUInt returned false");
    preferences.end();
    return false;
  }

  // Verification: Read back to ensure write succeeded
  if (preferences.getUInt("bootCount", 0) != nextCount) {
    Serial.println("[SYS] NVS failure: bootCount write verification failed");
    preferences.end();
    return false;
  }

  bootCount = nextCount;
  preferences.end();
  Serial.printf("[SYS] Boot Count updated to: %u\\n", bootCount);
  return true;
}

CommandResult processUnifiedCommand(const char* payload, size_t payloadLength, bool isLocal, const char* mqttTopic) {
  StaticJsonDocument<512> doc;
  DeserializationError error = deserializeJson(doc, payload, payloadLength);
  if (error) {
    Serial.println("[Cmd] Parse error");
    return {false, "rejected", "PARSE_ERROR", stateRevision};
  }

  // 1. MQTT Topic Validation (for remote commands)
  if (!isLocal) {
    if (mqttTopic == nullptr) {
      Serial.println("[Cmd] Remote command rejected: Missing MQTT topic");
      return {false, "rejected", "MISSING_TOPIC", stateRevision};
    }
    uint64_t chipid = ESP.getEfuseMac();
    char macStr[18];
    snprintf(macStr, sizeof(macStr), "%02x:%02x:%02x:%02x:%02x:%02x",
             (uint8_t)chipid, (uint8_t)(chipid >> 8), (uint8_t)(chipid >> 16),
             (uint8_t)(chipid >> 24), (uint8_t)(chipid >> 32), (uint8_t)(chipid >> 40));
    String expectedTopic = "sirius/cmd/ESP32-" + String(macStr);
    if (strcmp(mqttTopic, expectedTopic.c_str()) != 0) {
      Serial.printf("[Cmd] Topic mismatch: %s != %s\\n", mqttTopic, expectedTopic.c_str());
      return {false, "rejected", "TOPIC_MISMATCH", stateRevision};
    }
  }

  // 2. Basic Contract Validation (Types & Presence)
  if (!doc["commandId"].is<const char*>()) return {false, "rejected", "INVALID_CMDID", stateRevision};
  if (!doc["deviceId"].is<const char*>()) return {false, "rejected", "INVALID_DEVID", stateRevision};
  if (!doc["enrollmentGeneration"].is<uint32_t>()) return {false, "rejected", "INVALID_GENERATION", stateRevision};
  if (!doc["expectedSessionId"].is<const char*>()) return {false, "rejected", "INVALID_SESSID", stateRevision};
  if (!doc["expectedBootCount"].is<uint32_t>()) return {false, "rejected", "INVALID_BOOTCNT", stateRevision};
  if (!doc["expectedRevision"].is<uint32_t>()) return {false, "rejected", "INVALID_REV", stateRevision};
  if (!doc["expiresAt"].is<long long>()) return {false, "rejected", "INVALID_EXPIRY", stateRevision};
  if (!doc["target"].is<JsonObject>()) return {false, "rejected", "INVALID_TARGET", stateRevision};

  // Bound check string fields before storing in cache to prevent overflows
  const char* rawCmdId = doc["commandId"];
  if (strlen(rawCmdId) >= 37) return {false, "rejected", "CMDID_TOO_LONG", stateRevision};

  const char* rawDevId = doc["deviceId"];
  if (strlen(rawDevId) >= 32) return {false, "rejected", "DEVID_TOO_LONG", stateRevision};

  const char* rawSessId = doc["expectedSessionId"];
  if (strlen(rawSessId) >= 32) return {false, "rejected", "SESSID_TOO_LONG", stateRevision};

  const JsonObject target = doc["target"];
  if (!target["action"].is<const char*>()) return {false, "rejected", "INVALID_ACTION", stateRevision};
  if (!target["channel"].is<int>()) return {false, "rejected", "INVALID_CHAN", stateRevision};
  if (!target["value"].is<const char*>()) return {false, "rejected", "INVALID_VAL", stateRevision};

  const char* cmdId = doc["commandId"];
  const char* action = target["action"];
  if (strcmp(action, "SET_STATE") != 0) {
    Serial.println("[Cmd] Invalid action");
    return {false, "rejected", "INVALID_ACTION", stateRevision};
  }

  const char* deviceId = doc["deviceId"];
  uint32_t enrollmentGeneration = doc["enrollmentGeneration"];
  uint32_t expBoot = doc["expectedBootCount"];
  const char* expSess = doc["expectedSessionId"];
  uint32_t expRev = doc["expectedRevision"];
  long long expiresAt = doc["expiresAt"];
  int channel = target["channel"];
  const char* value = target["value"];

  // Validate value is strictly "on" or "off"
  if (strcmp(value, "on") != 0 && strcmp(value, "off") != 0) {
    Serial.println("[Cmd] Invalid value (must be on/off)");
    return {false, "rejected", "INVALID_VALUE", stateRevision};
  }

  // 3. Factory Identity Validation
  uint64_t chipid = ESP.getEfuseMac();
  char macStr[18];
  snprintf(macStr, sizeof(macStr), "%02x:%02x:%02x:%02x:%02x:%02x",
           (uint8_t)chipid, (uint8_t)(chipid >> 8), (uint8_t)(chipid >> 16),
           (uint8_t)(chipid >> 24), (uint8_t)(chipid >> 32), (uint8_t)(chipid >> 40));
  String myId = "ESP32-" + String(macStr);
  if (strcmp(deviceId, myId.c_str()) != 0) {
    Serial.printf("[Cmd] DeviceId mismatch: %s != %s\\n", deviceId, myId.c_str());
    return {false, "rejected", "DEVID_MISMATCH", stateRevision};
  }
  if (enrollmentGeneration != MQTT_ENROLLMENT_GENERATION) {
    return {false, "rejected", "GENERATION_FAIL", stateRevision};
  }

  // 4. Identity and Deduplication
  time_t now;
  time(&now);

  for (int i = 0; i < 10; i++) {
    if (cmdCache[i].active && strcmp(cmdCache[i].commandId, cmdId) == 0) {
      bool clockSynced = (now >= 1000000);
      if (clockSynced && now >= cmdCache[i].expiresAt / 1000) {
        cmdCache[i].active = false; // Expired
        break;
      }

      if (cmdCache[i].enrollmentGeneration == enrollmentGeneration &&
          cmdCache[i].expBoot == expBoot &&
          strcmp(cmdCache[i].deviceId, deviceId) == 0 &&
          strcmp(cmdCache[i].expSess, expSess) == 0 &&
          cmdCache[i].expRev == expRev &&
          cmdCache[i].expiresAt == expiresAt &&
          cmdCache[i].targetChannel == channel &&
          (strcmp(value, "on") == 0) == cmdCache[i].targetValue) {

        Serial.println("[Cmd] Duplicate found. Returning cached result.");
        return {
          strcmp(cmdCache[i].status, "accepted") == 0,
          cmdCache[i].status,
          cmdCache[i].reason,
          cmdCache[i].resultingRev
        };
      } else {
        Serial.println("[Cmd] ID collision: same ID, different payload. Rejecting.");
        return {false, "rejected", "ID_COLLISION", stateRevision};
      }
    }
  }

  // 5. Temporal Validation (with Local Bypass)
  bool clockSynced = (now >= 1000000);
  if (!isLocal && !clockSynced) {
    Serial.println("[Cmd] Remote command rejected: Clock unsynchronized.");
    return {false, "rejected", "CLOCK_UNSYNC", stateRevision};
  }
  if (clockSynced && now >= expiresAt / 1000) {
    Serial.println("[Cmd] Command expired.");
    return {false, "rejected", "EXPIRED", stateRevision};
  }
  if (isLocal && !clockSynced) {
    Serial.println("[Cmd] Local bypass: skipping expiry check (clock unsynced).");
  }

  // 6. Fencing Validation
  if (expBoot != bootCount || strcmp(expSess, sessionId.c_str()) != 0 || expRev != stateRevision) {
    Serial.printf("[Cmd] Fencing fail: Boot %u/%u, Sess %s/%s, Rev %u/%u\\n",
                  expBoot, bootCount, expSess, sessionId.c_str(), expRev, stateRevision);
    return {false, "rejected", "FENCE_FAIL", stateRevision};
  }

  // 7. Actuation
  if (channel >= 1 && channel <= CHANNEL_COUNT) {
    // Deduplication Slot Check: Ensure we can record this command before actuating
    int slot = -1;
    for(int i=0; i<10; i++) {
      if(!cmdCache[i].active) { slot = i; break; }
      // Check if current slot is actually expired
      time_t now_slot;
      time(&now_slot);
      if (now_slot >= cmdCache[i].expiresAt / 1000) {
        cmdCache[i].active = false;
        slot = i;
        break;
      }
    }
    if(slot == -1) {
      Serial.println("[Cmd] Deduplication cache full. Rejecting new command.");
      return {false, "rejected", "CACHE_FULL", stateRevision};
    }

    bool newState = (strcmp(value, "on") == 0);
    setChannelState(channel - 1, newState);

    // Store result in cache
    strncpy(cmdCache[slot].commandId, cmdId, 36);
    strncpy(cmdCache[slot].deviceId, deviceId, 31);
    cmdCache[slot].enrollmentGeneration = enrollmentGeneration;
    cmdCache[slot].expBoot = expBoot;
    strncpy(cmdCache[slot].expSess, expSess, 31);
    cmdCache[slot].expRev = expRev;
    cmdCache[slot].expiresAt = expiresAt;
    cmdCache[slot].targetChannel = channel;
    cmdCache[slot].targetValue = newState;
    strncpy(cmdCache[slot].status, "accepted", 15);
    cmdCache[slot].resultingRev = stateRevision;
    strncpy(cmdCache[slot].reason, "OK", 31);
    cmdCache[slot].active = true;

    Serial.println("[Cmd] Actuation successful.");
    return {true, "accepted", "OK", stateRevision};
  } else {
    Serial.println("[Cmd] Invalid channel index.");
    return {false, "rejected", "INVALID_CHANNEL", stateRevision};
  }
}

void mqttCallback(char* topic, byte* payload, unsigned int length) {
  Serial.printf("[MQTT] Message arrived [%s]\\n", topic);

  // Route incoming unified commands to the safety fence
  CommandResult result = processUnifiedCommand((const char*)payload, length, false, topic);

  if (result.success) {
    Serial.printf("[MQTT] Command accepted. Resulting Rev: %u\\n", result.resultingRev);
  } else {
    Serial.printf("[MQTT] Command rejected. Reason: %s\\n", result.reason);
  }

  // --- MQTT Acknowledgement ---
  // Publish structured ACK to sirius/ack/ESP32-<mac>
  uint64_t chipid = ESP.getEfuseMac();
  char macStr[18];
  snprintf(macStr, sizeof(macStr), "%02x:%02x:%02x:%02x:%02x:%02x",
           (uint8_t)chipid, (uint8_t)(chipid >> 8), (uint8_t)(chipid >> 16),
           (uint8_t)(chipid >> 24), (uint8_t)(chipid >> 32), (uint8_t)(chipid >> 40));
  const String factoryId = "ESP32-" + String(macStr);
  String ackTopic = "sirius/ack/" + factoryId;

  StaticJsonDocument<512> ackDoc;
  // Attempt to extract commandId from payload if possible
  StaticJsonDocument<256> cmdDoc;
  DeserializationError err = deserializeJson(cmdDoc, payload, length);

  const char* cmdId = (err == DeserializationError::Ok && cmdDoc["commandId"].is<const char*>())
                      ? cmdDoc["commandId"].as<const char*>() : "unknown";

  ackDoc["commandId"] = cmdId;
  ackDoc["deviceId"] = factoryId;
  ackDoc["enrollmentGeneration"] = MQTT_ENROLLMENT_GENERATION;
  ackDoc["bootCount"] = bootCount;
  ackDoc["sessionId"] = sessionId;
  ackDoc["resultingRevision"] = result.resultingRev;
  ackDoc["status"] = result.status;
  ackDoc["reason"] = result.reason;

  char ackPayload[512];
  serializeJson(ackDoc, ackPayload, sizeof(ackPayload));
  if (mqttClient.publish(ackTopic.c_str(), ackPayload)) {
    Serial.printf("[MQTT] ACK published to %s\\n", ackTopic.c_str());
  } else {
    Serial.printf("[MQTT] ACK publish FAILED to %s\\n", ackTopic.c_str());
  }
}

void connectMQTT() {
  if (currentState != STATE_STA) return;

  Serial.println("[MQTT] Connecting to EMQX...");
  secureClient.setCACert(MQTT_CA);
  mqttClient.setServer(MQTT_BROKER, 8883);
  // The fenced command and full-state snapshot exceed PubSubClient's
  // 256-byte default once topic bytes are included.
  mqttClient.setBufferSize(1024);
  mqttClient.setCallback(mqttCallback);

  // Use unique client ID based on factory identity to avoid collisions
  uint64_t chipid = ESP.getEfuseMac();
  char clientId[32];
  snprintf(clientId, sizeof(clientId), "%s-%04X%08X", PRODUCT_NAME, (uint16_t)(chipid & 0xFFFF), (uint32_t)(chipid >> 16));

  const String presenceTopic = "sirius/devices/" + mqttFactoryId() + "/presence";
  StaticJsonDocument<256> offlineDoc;
  offlineDoc["enrollmentGeneration"] = MQTT_ENROLLMENT_GENERATION;
  offlineDoc["bootCount"] = bootCount;
  offlineDoc["sessionId"] = sessionId;
  offlineDoc["status"] = "offline";
  char offlinePayload[256];
  serializeJson(offlineDoc, offlinePayload, sizeof(offlinePayload));

  if (mqttClient.connect(clientId, MQTT_USER, MQTT_PASS, presenceTopic.c_str(), 1, true, offlinePayload)) {
    Serial.println("[MQTT] Connected successfully!");

    // Subscribe to device-specific command topic for Unified Control
    uint64_t chipid = ESP.getEfuseMac();
    char macStr[18];
    snprintf(macStr, sizeof(macStr), "%02x:%02x:%02x:%02x:%02x:%02x",
             (uint8_t)chipid, (uint8_t)(chipid >> 8), (uint8_t)(chipid >> 16),
             (uint8_t)(chipid >> 24), (uint8_t)(chipid >> 32), (uint8_t)(chipid >> 40));
    String cmdTopic = "sirius/cmd/ESP32-" + String(macStr);

    mqttClient.subscribe(cmdTopic.c_str());
    Serial.printf("[MQTT] Subscribed to: %s\\n", cmdTopic.c_str());
    publishMqttPresence("online");
    publishMqttState();
  } else {
    Serial.printf("[MQTT] Connection failed, rc=%d\\n", mqttClient.state());
  }
}

void broadcastStateEvent() {
  uint64_t chipid = ESP.getEfuseMac();
  char macStr[18];
  snprintf(macStr, sizeof(macStr), "%02x:%02x:%02x:%02x:%02x:%02x",
           (uint8_t)chipid, (uint8_t)(chipid >> 8), (uint8_t)(chipid >> 16),
           (uint8_t)(chipid >> 24), (uint8_t)(chipid >> 32), (uint8_t)(chipid >> 40));

  String json = "{";
  json += "\"type\":\"state_update\", ";
  json += "\"deviceId\":\"ESP32-" + String(macStr) + "\", ";
  json += "\"product\":\"" + String(PRODUCT_NAME) + "\", ";
  json += "\"sessionId\":\"" + sessionId + "\", ";
  json += "\"revision\":" + String(stateRevision) + ", ";
  json += "\"bootCount\":" + String(bootCount) + ", ";
  json += "\"enrollmentGeneration\":" + String(MQTT_ENROLLMENT_GENERATION) + ", ";
  json += "\"timestamp\":" + String(millis()) + ", ";
  json += "\"state\":{";
  json += "\"channels\":[";
  for (int i = 0; i < CHANNEL_COUNT; i++) {
    json += "{\"index\":" + String(i + 1) + ", \"state\":\"" + String(channels[i].state ? "on" : "off") + "\"}";
    if (i < CHANNEL_COUNT - 1) json += ",";
  }
  json += "]}";
  json += "}";

  wsServer.broadcastTXT(json);
  publishMqttState();
}

void setChannelState(int index, bool state) {
  if (index < 0 || index >= CHANNEL_COUNT) return;
  if (channels[index].state == state) return;

  channels[index].state = state;
  digitalWrite(channels[index].pin, state ? HIGH : LOW);
  Serial.printf("Channel %d (%s) set to: %s\\n", index + 1, channels[index].name, state ? "ON" : "OFF");

  stateRevision++;
  broadcastStateEvent();
}

void factoryReset() {
  logState("Performing Factory Reset...");
  Preferences preferences;
  preferences.begin("aion-proto", false);
  preferences.remove("ssid");
  preferences.remove("pass");
  preferences.end();
  Serial.println("Credentials cleared. Rebooting...");
  delay(1000);
  ESP.restart();
}

void handleProvision() {
  if (server.hasArg("plain")) {
    String body = server.arg("plain");
    int ssidStart = body.indexOf("\"ssid\":\"") + 8;
    int ssidEnd = body.indexOf("\"", ssidStart);
    int passStart = body.indexOf("\"pass\":\"") + 8;
    int passEnd = body.indexOf("\"", passStart);

    if (ssidStart > 7 && passStart > 7) {
      wifiSsid = body.substring(ssidStart, ssidEnd);
      wifiPass = body.substring(passStart, passEnd);

      Preferences preferences;
      preferences.begin("aion-proto", false);
      preferences.putString("ssid", wifiSsid);
      preferences.putString("pass", wifiPass);
      preferences.end();

      Serial.printf("Provisioned: SSID=%s, PASS=%s\\n", wifiSsid.c_str(), wifiPass.c_str());
      server.send(200, "application/json", "{\"status\":\"connecting\"}");

      delay(2000);
      ESP.restart();
    } else {
      server.send(400, "application/json", "{\"status\":\"error\", \"msg\":\"invalid_payload\"}");
    }
  } else {
    server.send(400, "application/json", "{\"status\":\"error\", \"msg\":\"no_payload\"}");
  }
}

void handleStatus() {
  String state = (currentState == STATE_SOFTAP) ? "provisioning" : "operational";
  server.send(200, "application/json", "{\"state\":\"" + state + "\"}");
}

void handleInfo() {
  uint64_t chipid = ESP.getEfuseMac();
  char macStr[18];
  snprintf(macStr, sizeof(macStr), "%02x:%02x:%02x:%02x:%02x:%02x",
           (uint8_t)chipid, (uint8_t)(chipid >> 8), (uint8_t)(chipid >> 16),
           (uint8_t)(chipid >> 24), (uint8_t)(chipid >> 32), (uint8_t)(chipid >> 40));

  String json = "{";
  json += "\"id\":\"ESP32-" + String(macStr) + "\", ";
  json += "\"productType\":\"" + String(PRODUCT_NAME) + "\", ";
  json += "\"version\":\"" + String(FIRMWARE_VERSION) + "\", ";
  json += "\"channelCount\":" + String(CHANNEL_COUNT) + ", ";
  json += "\"channels\":[";
  for (int i = 0; i < CHANNEL_COUNT; i++) {
    json += "{\"index\":" + String(i + 1) + ", \"state\":\"" + String(channels[i].state ? "on" : "off") + "\"}";
    if (i < CHANNEL_COUNT - 1) json += ",";
  }
  json += "]}";

  server.send(200, "application/json", json);
}

void handleState() {
  String json = "{";
  json += "\"channels\":[";
  for (int i = 0; i < CHANNEL_COUNT; i++) {
    json += "{\"index\":" + String(i + 1) + ", \"state\":\"" + String(channels[i].state ? "on" : "off") + "\"}";
    if (i < CHANNEL_COUNT - 1) json += ",";
  }
  json += "], \"rssi\":" + String(WiFi.RSSI()) + ", \"revision\":" + String(stateRevision) + ", \"sessionId\":\"" + sessionId + "\", \"bootCount\":" + String(bootCount) + ", \"enrollmentGeneration\":" + String(MQTT_ENROLLMENT_GENERATION) + "}";
  server.send(200, "application/json", json);
}

void handleControl() {
  if (server.hasArg("plain")) {
    String body = server.arg("plain");
    int channelStart = body.indexOf("\"channel\":") + 10;
    int channelEnd = body.indexOf(",", channelStart);
    if (channelEnd == -1) channelEnd = body.indexOf("}", channelStart);

    int stateStart = body.indexOf("\"state\":\"") + 9;
    int stateEnd = body.indexOf("\"", stateStart);

    if (channelStart > 9 && stateStart > 8) {
      int channelIdx = body.substring(channelStart, channelEnd).toInt();
      String stateStr = body.substring(stateStart, stateEnd);

      if (channelIdx >= 1 && channelIdx <= CHANNEL_COUNT) {
        bool newState = (stateStr == "on");
        setChannelState(channelIdx - 1, newState);

        String json = "{\"status\":\"ok\", \"state\":\"" + String(channels[channelIdx-1].state ? "on" : "off") + "\"}";
        server.send(200, "application/json", json);
        return;
      } else {
        server.send(400, "application/json", "{\"status\":\"error\", \"msg\":\"invalid_channel\"}");
        return;
      }
    }
    server.send(400, "application/json", "{\"status\":\"error\", \"msg\":\"invalid_payload\"}");
  } else {
    server.send(400, "application/json", "{\"status\":\"error\"}");
  }
}

void handleResetProvisioning() {
  server.send(200, "application/json", "{\"status\":\"resetting\"}");
  Serial.println("Reset request received. Rebooting to SoftAP mode...");
  delay(500);
  factoryReset();
}

void handleUnifiedCommand() {
  if (server.hasArg("plain")) {
    String body = server.arg("plain");
    CommandResult result = processUnifiedCommand(body.c_str(), body.length(), true);

    String response = "{";
    response += "\"status\":\"" + String(result.status) + "\", ";
    response += "\"reason\":\"" + String(result.reason) + "\", ";
    response += "\"revision\":" + String(result.resultingRev);
    response += "}";
    server.send(200, "application/json", response);
  } else {
    server.send(400, "application/json", "{\"status\":\"error\", \"msg\":\"missing_payload\"}");
  }
}

void registerRoutes(int state) {
  server.on("/api/info", HTTP_GET, handleInfo);
  server.on("/api/state", HTTP_GET, handleState);
  server.on("/api/reset-provisioning", HTTP_POST, handleResetProvisioning);

  if (state == STATE_SOFTAP) {
    server.on("/provision", HTTP_POST, handleProvision);
    server.on("/status", HTTP_GET, handleStatus);
  } else if (state == STATE_STA) {
    server.on("/api/command", HTTP_POST, handleUnifiedCommand);
    server.on("/api/control", HTTP_POST, handleControl);
  }
}

void setupCommon() {
  Serial.begin(115200);

  for (int i = 0; i < CHANNEL_COUNT; i++) {
    pinMode(channels[i].pin, OUTPUT);
    digitalWrite(channels[i].pin, LOW);
    channels[i].state = false;
  }
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);
  pinMode(RESET_PIN, INPUT_PULLUP);
  for (int i = 0; i < CHANNEL_COUNT; i++) {
    pinMode(BUTTON_PINS[i], INPUT_PULLUP);
  }

  // 1. Safety Fence: Boot Count Persistence
  if (!updateBootCount()) {
    Serial.println("[FATAL] Boot count persistence failed. Disabling connectivity for safety.");
    // Maintain safe relay state, but stop further setup.
    // Networking and API will remain uninitialized.
    return;
  }

  // Generate Session ID
  sessionId = String(ESP.getEfuseMac(), HEX);
  sessionId += String(random(0, 0xFFFF), HEX);
  sessionId += String(millis());

  Preferences preferences;
  preferences.begin("aion-proto", true);
  wifiSsid = preferences.getString("ssid", "");
  wifiPass = preferences.getString("pass", "");
  preferences.end();

  if (wifiSsid == "" || wifiPass == "") {
    currentState = STATE_SOFTAP;
    logState("No credentials found. Entering SoftAP Mode...");
    WiFi.mode(WIFI_AP);
    WiFi.softAP(PRODUCT_SSID, SOFTAP_PASS);
    registerRoutes(currentState);
    server.begin();
    wsServer.begin();
    Serial.println("SoftAP Started");
    Serial.print("SSID: "); Serial.println(PRODUCT_SSID);
    Serial.print("IP: "); Serial.println(WiFi.softAPIP());
  } else {
    currentState = STATE_STA;
    logState("Credentials found. Connecting to WiFi...");
    WiFi.mode(WIFI_STA);
    WiFi.begin(wifiSsid.c_str(), wifiPass.c_str());

    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 20) {
      delay(500);
      Serial.print(".");
      attempts++;
    }

    if (WiFi.status() == WL_CONNECTED) {
      logState("Connected to WiFi!");
      Serial.print("IP Address: "); Serial.println(WiFi.localIP());

      if (!MDNS.begin(MDNS_HOSTNAME)) {
        Serial.println("Error setting up MDNS responder!");
      } else {
        Serial.println("mDNS responder started");
        MDNS.addService("http", "tcp", 80);
      }

      registerRoutes(currentState);
      server.begin();
      wsServer.begin();

      // Remote commands are accepted only after this clock is synchronized.
      configTime(0, 0, "pool.ntp.org", "time.nist.gov");
      connectMQTT();
    } else {
      logState("WiFi Connection Failed. Falling back to SoftAP...");
      ESP.restart();
    }
  }
}

void loopCommon() {
  if (currentState == STATE_SOFTAP) {
    if (millis() - lastBlinkTime >= 250) {
      lastBlinkTime = millis();
      ledState = !ledState;
      digitalWrite(LED_PIN, ledState);
    }
  } else {
    digitalWrite(LED_PIN, LOW);

    // Keep MQTT connection alive
    if (mqttClient.connected()) {
      mqttClient.loop();
      if (millis() - lastMqttStatePublish >= MQTT_STATE_HEARTBEAT_MS) {
        lastMqttStatePublish = millis();
        publishMqttState();
      }
    } else {
      // Attempt reconnect every 10 seconds if disconnected
      static unsigned long lastMqttRetry = 0;
      if (millis() - lastMqttRetry > 10000) {
        lastMqttRetry = millis();
        connectMQTT();
      }
    }
  }

  // Physical Button Polling
  for (int i = 0; i < CHANNEL_COUNT; i++) {
    bool currentButtonState = digitalRead(BUTTON_PINS[i]);
    if (currentButtonState != lastButtonState[i]) {
      if (millis() - lastButtonPressTime[i] > DEBOUNCE_DELAY) {
        if (currentButtonState == LOW) { // Falling edge
          Serial.printf("[BUTTON %d] Pressed - Toggling Channel %d\\n", i + 1, i + 1);
          setChannelState(i, !channels[i].state);
        }
        lastButtonState[i] = currentButtonState;
        lastButtonPressTime[i] = millis();
      }
    }
  }

  server.handleClient();
  wsServer.loop();

  // Heartbeat: Send periodic ping to clients to detect dead connections
  static unsigned long lastHeartbeat = 0;
  if (millis() - lastHeartbeat > 15000) {
    lastHeartbeat = millis();
    wsServer.broadcastPing();
  }

  if (digitalRead(RESET_PIN) == LOW) {
    unsigned long pressStart = millis();
    while (digitalRead(RESET_PIN) == LOW) {
      if (millis() - pressStart > 5000) {
        factoryReset();
        break;
      }
      delay(10);
    }
  }
}
