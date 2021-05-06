#include "AK-030.h"

#define LOOP_INTERVAL (1000 * 1)

AK030 *ak030 = new AK030();

void setup() {
  Serial.begin(115200);

  int cnt = 10;
  while (--cnt >= 0) {
    Serial.printf("count down: %d\n", cnt);
    delay(1000);
  }

  Serial.println();
  Serial.println("@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@");
  Serial.println("If AK-030 is booted after long power down,");
  Serial.println("it may take long time to get non 'N/A' RSSI result.");
  Serial.println("@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@");
  Serial.println();
  delay(1000 * 3);

#if 0
  // You do not need to call ak030->begin() to get RSSI.
  ak030->begin("soracom.io");
  // ak030->debug = true;
  while (!ak030->connected()) {
    Serial.println("connecting to LTE network");
    ak030->connect();  // connect to LTE network
    if (ak030->ng()) {
      Serial.println("cannot connect to LTE network");
      delay(1000 * 10);
    } else {
      Serial.println("...connected");
      break;
    }
  }
#endif
}

void loop() {
  int rssi = ak030->getRSSI();
  if (ak030->ok()) {
    Serial.printf("RSSI = %d\n", rssi);
  } else {
    Serial.println("RSSI = N/A");
  }
  delay(LOOP_INTERVAL);
}