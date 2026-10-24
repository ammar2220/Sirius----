#include "SiriusCommon.h"

int CHANNEL_COUNT = 2;
const char* PRODUCT_NAME = "SIRIUS_SWITCH_2CH";
const char* PRODUCT_SSID = "Sirius_2CH_Switch";

Channel channels[] = {
  {12, false, "Main"},
  {13, false, "Secondary"}
};

void setup() {
  setupCommon();
}

void loop() {
  loopCommon();
}
