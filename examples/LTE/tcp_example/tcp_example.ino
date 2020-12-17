#include "AK-030.h"
#include "Arduino.h"

#define APN            "soracom.io"
#define DAYTIME_SERVER "time-c.nist.gov"
#define DAYTIME_PORT   13

AK030 *ak030 = new AK030();

void daytime_demo() {
  Serial.println();
  Serial.println("@@@@@@@@@@@@@@@@@@@@@@@@@@@");
  Serial.println("daytime protocol demo start");
  Serial.println("@@@@@@@@@@@@@@@@@@@@@@@@@@@");
  if (!ak030->connected()) {
    Serial.println("connecting to LTE network...");
    ak030->connect();  // connect to LTE Network
    if (!ak030->ok()) {
      Serial.println("cannot connect LTE network");
      return;
    }
    Serial.println("...connected");
  }

  // resolve ip address
  Serial.printf("dns lookup: %s\n", DAYTIME_SERVER);
  const char *ipaddr = ak030->dnsLookup(DAYTIME_SERVER);
  if (ak030->ng()) {
    Serial.println("dns lookup failed");
    return;
  }
  Serial.printf("=> %s\n", ipaddr);

  // open tcp socket
  Serial.printf("open tcp: ipaddr=%s, port=%d\n", ipaddr, DAYTIME_PORT);
  ak030->openTcp(ipaddr, DAYTIME_PORT);
  if (ak030->ng()) {
    Serial.printf("cannot open tcp\n");
    return;
  } else {
    Serial.println("...opened");
  }

  ak030->waitEvent(10);              // wait for socket event for 10 seconds(at most)
  if (ak030->eventDataReceived()) {  // check if data received
    char data[1500];
    int len;
    // recieve data from NTP Server
    ak030->receive(data, sizeof(data), &len);
    Serial.printf("received %d bytes\n", len);
    Serial.println("==== received data start ====");
    Serial.println(data);
    Serial.println("==== received data end ======");
  } else {
    Serial.println("data is not received");
  }

  ak030->waitEvent(3);
  if (ak030->eventTerminated()) {
    Serial.println("...tcp closed by peer");
  }
  Serial.println("close tcp");
  ak030->close();
}

void setup() {
  Serial.begin(115200);

  int cnt = 10;
  while (--cnt >= 0) {
    Serial.printf("count down: %d\n", cnt);
    delay(1000);
  }

  // ak030->debug = true;
  ak030->begin(APN);
}

void loop() {
  daytime_demo();
  delay(1000 * 30);
}
