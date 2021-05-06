#include "AK-030.h"
#include "Arduino.h"

#define SORACOM
//#define IIJ

#ifdef SORACOM
#define APN     "soracom.io"
#define USER    "sora"
#define PASSWD  "sora"
#define PPPAUTH "PAP"
#endif

#ifdef IIJ
#define APN     "iijmio.jp"
#define USER    "mio@iij"
#define PASSWD  "iij"
#define PPPAUTH "PAP"
#endif

#define UDP_ECHO_SERVER "ak030-test.abit.xyz"
#define UDP_ECHO_PORT   7

#define LOOP_INTERVAL         (1000 * 60 * 1)
#define LOOP_INTERVAL_WHEN_NG (1000 * 5)

AK030 *ak030 = new AK030();

bool udp_echo_demo() {
  Serial.println();
  Serial.println(":::::::::::::::::::::::");
  Serial.println("::: udp_echo_demo() :::");
  Serial.println(":::::::::::::::::::::::");
  Serial.println();

  if (!ak030->connected()) {
    Serial.println("connecting to LTE network...");
    ak030->connect();  // connect to LTE Network
    if (!ak030->ok()) {
      Serial.println("=> cannot connect LTE network");
      return false;
    }
    Serial.println("=> LTE network connected");
  }

  Serial.printf("dns lookup : %s\n", UDP_ECHO_SERVER);
  const char *ipaddr = ak030->dnsLookup(UDP_ECHO_SERVER);
  if (!ipaddr) {
    Serial.println("=> dns lookup failed");
    return false;
  }
  Serial.printf("=> ipaddress=%s\n", ipaddr);

  // open udp socket
  Serial.printf("open udp: ipaddr=%s, port=%d\n", ipaddr, UDP_ECHO_PORT);
  ak030->openUdp(ipaddr, UDP_ECHO_PORT);
  if (ak030->ng()) {
    Serial.println("=> open udp failed");
    return false;
  }

  Serial.println("sending data");
  ak030->send("Hello");
  if (ak030->ng()) {
    Serial.println("=> send data failed");
    ak030->close();
    return false;
  }

  ak030->waitEvent(30);              // wait for socket event for 30 seconds(at most)
  if (ak030->eventDataReceived()) {  // check if data received
    static char data[1500];
    int len;
    ak030->receive(data, sizeof(data), &len);
    Serial.printf("=> received %d bytes\n", len);
    Serial.println(">>>> received data begin >>>");
    Serial.println(data);
    Serial.println("<<<< received data end <<<<<");
  } else {
    Serial.println("=> data is not received");
  }

  Serial.println("close udp");
  ak030->close();

  return true;
}

void setup() {
  Serial.begin(115200);

  int cnt = 10;
  while (--cnt >= 0) {
    Serial.printf("count down: %d\n", cnt);
    delay(1000);
  }

  Serial.println();
  Serial.println("@@@@@@@@@@@@@@@@@@@");
  Serial.println("UDP Echo demo start");
  Serial.println("@@@@@@@@@@@@@@@@@@@");

  char msg[128];
  Serial.println();
  snprintf(msg, sizeof(msg), "initialize AK-030 : APN='%s', USER='%s', PASSWD='%s', PPPAUTH='%s'", APN, USER, PASSWD,
           PPPAUTH);
  Serial.println(msg);

  // ak030->debug = true;
  // ak030->begin('soracom.io'); // You may omit USER/PASSWD/PPPAUTH, when apn is 'soracom.io'.
  ak030->begin(APN, USER, PASSWD, PPPAUTH);
}

void loop() {
  char msg[64];
  unsigned long interval = LOOP_INTERVAL;

  bool ok = udp_echo_demo();
  if (!ok) {
    interval = LOOP_INTERVAL_WHEN_NG;
  }
  snprintf(msg, sizeof(msg), "waiting %.1f sec...", interval / 1000.0);
  Serial.println();
  Serial.println(msg);
  delay(interval);
}