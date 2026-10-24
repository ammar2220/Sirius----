#include <cstdint>
// Local MQTT configuration - NOT tracked in Git
// Fill these values based on your EMQX Cloud instance
const char* MQTT_BROKER = "your-broker-host.emqxsl.com";
const char* MQTT_USER = "your_username";
const char* MQTT_PASS = "your_password";
const uint32_t MQTT_ENROLLMENT_GENERATION = 1;
const char* MQTT_CA = R"PEM(-----BEGIN CERTIFICATE-----
... Paste your Root CA here ...
-----END CERTIFICATE-----
)PEM";
