#include "SiriusCommon.h"

DeviceState currentState = STATE_BOOT;
WebServer server(80);
WebSocketsServer wsServer = WebSocketsServer(81);
String wifiSsid = "";
String wifiPass = "";
unsigned long lastBlinkTime = 0;
bool ledState = false;
uint32_t stateRevision = 0;
String sessionId = "";

// Physical Button State
unsigned long lastButtonPressTime[3] = {0, 0, 0};
bool lastButtonState[3] = {HIGH, HIGH, HIGH};
const unsigned long DEBOUNCE_DELAY = 50;

const int BUTTON_PINS[] = {4, 19, 18};
const char* FIRMWARE_VERSION = "1.0.0";

void logState(const char* msg) {
  Serial.printf("[STATE] %s\\n", msg);
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
  json += "], \"rssi\":" + String(WiFi.RSSI()) + ", \"revision\":" + String(stateRevision) + ", \"sessionId\":\"" + sessionId + "\"}";
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

void registerRoutes(int state) {
  server.on("/api/info", HTTP_GET, handleInfo);
  server.on("/api/state", HTTP_GET, handleState);
  server.on("/api/reset-provisioning", HTTP_POST, handleResetProvisioning);

  if (state == STATE_SOFTAP) {
    server.on("/provision", HTTP_POST, handleProvision);
    server.on("/status", HTTP_GET, handleStatus);
  } else if (state == STATE_STA) {
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
