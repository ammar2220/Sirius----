#include "SiriusCommon.h"

int CHANNEL_COUNT = 3;
const char* PRODUCT_NAME = "SIRIUS_SWITCH_3CH";
const char* PRODUCT_SSID = "Sirius_3CH_Switch";

Channel channels[] = {
  {12, false, "Main"},
  {13, false, "Secondary"},
  {14, false, "Auxiliary"}
};

void setup() {
  setupCommon();
}

void loop() {
  loopCommon();
}
