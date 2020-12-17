#include "AK-030.h"

AK030 *ak030 = new AK030();

void setup() {
  Serial.begin(115200);

  int cnt = 10;
  while (--cnt >= 0) {
    Serial.printf("count down: %d\n", cnt);
    delay(1000);
  }

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
}

void loop() {
  int rssi = ak030->getRSSI();
  if (ak030->ng()) {
    Serial.println("getRSSI failed");
    return;
  }
  Serial.printf("rssi = %d\n", rssi);
  delay(1000 * 3);
}