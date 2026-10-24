#include "SiriusCommon.h"

int CHANNEL_COUNT = 1;
const char* PRODUCT_NAME = "SIRIUS_SWITCH_1CH";
const char* PRODUCT_SSID = "Sirius_1CH_Switch";

Channel channels[] = {
  {12, false, "Main"}
};

void setup() {
  setupCommon();
}

void loop() {
  loopCommon();
}
