#include "AK-030.h"
#include "Arduino.h"

#define APN             "soracom.io"
#define UDP_ECHO_SERVER "140.112.148.237"
#define UDP_ECHO_PORT   7

AK030 *ak030 = new AK030();

void udp_echo_demo() {
  Serial.println();
  Serial.println("@@@@@@@@@@@@@@@@@@@");
  Serial.println("UDP Echo demo start");
  Serial.println("@@@@@@@@@@@@@@@@@@@");
  if (!ak030->connected()) {
    Serial.println("connecting to LTE network...");
    ak030->connect();  // connect to LTE Network
    if (!ak030->ok()) {
      Serial.println("cannot connect LTE network");
      return;
    }
    Serial.println("LTE network connected");
  }

  // open udp socket
  Serial.printf("open udp: ipaddr=%s, port=%d\n", UDP_ECHO_SERVER, UDP_ECHO_PORT);
  ak030->openUdp(UDP_ECHO_SERVER, UDP_ECHO_PORT);
  if (ak030->ng()) {
    Serial.println("open udp failed");
    return;
  }

  Serial.println("sending data");
  ak030->send("Hello");
  if (ak030->ng()) {
    Serial.println("send data failed");
    ak030->close();
  }

  ak030->waitEvent(30);              // wait for socket event for 30 seconds(at most)
  if (ak030->eventDataReceived()) {  // check if data received
    char data[1500];
    int len;
    ak030->receive(data, sizeof(data), &len);
    Serial.printf("received %d bytes\n", len);
    Serial.println("==== received data begin ===");
    Serial.println(data);
    Serial.println("==== received data end =====");
  } else {
    Serial.println("data is not received");
  }

  Serial.println("close upd");
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
  udp_echo_demo();
  delay(1000 * 60);
}
