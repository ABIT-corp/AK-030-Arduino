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

#define DAYTIME_SERVER "time-c.nist.gov"
#define DAYTIME_PORT   13

#define LOOP_INTERVAL         (1000 * 60 * 1)
#define LOOP_INTERVAL_WHEN_NG (1000 * 5)

AK030 *ak030 = new AK030();

bool daytime_demo() {
  Serial.println();
  Serial.println("::::::::::::::::::::::");
  Serial.println("::: daytime_demo() :::");
  Serial.println("::::::::::::::::::::::");
  Serial.println();

  if (!ak030->connected()) {
    Serial.println("connecting to LTE network...");
    ak030->connect();  // connect to LTE Network
    if (!ak030->ok()) {
      Serial.println("=> cannot connect LTE network");
      return false;
    }
    Serial.println("=> connected");
  }

  // resolve ip address
  Serial.printf("dns lookup: %s\n", DAYTIME_SERVER);
  const char *ipaddr = ak030->dnsLookup(DAYTIME_SERVER);
  if (ak030->ng()) {
    Serial.println("=> dns lookup failed");
    return false;
  }
  Serial.printf("=> %s\n", ipaddr);

  // open tcp socket
  Serial.printf("open tcp: ipaddr=%s, port=%d\n", ipaddr, DAYTIME_PORT);
  ak030->openTcp(ipaddr, DAYTIME_PORT);
  if (ak030->ng()) {
    Serial.printf("=> cannot open tcp\n");
    return false;
  } else {
    Serial.println("=> opened");
  }

  ak030->waitEvent(10);              // wait for socket event for 10 seconds(at most)
  if (ak030->eventDataReceived()) {  // check if data received
    char data[1500];
    int len;
    // receive data from NTP Server
    ak030->receive(data, sizeof(data), &len);
    Serial.printf("received %d bytes\n", len);
    Serial.println(">>>> received data start >>>>");
    Serial.println(data);
    Serial.println("<<<< received data end <<<<<<");
  } else {
    Serial.println("=> data is not received");
    ak030->close();
    return false;
  }

  ak030->waitEvent(3);
  if (ak030->eventTerminated()) {
    Serial.println("...tcp closed by peer");
  }
  Serial.println("close tcp");
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
  Serial.println("@@@@@@@@@@@@@@@@@@@@@@@@@@@");
  Serial.println("daytime protocol demo start");
  Serial.println("@@@@@@@@@@@@@@@@@@@@@@@@@@@");

  char msg[128];
  Serial.println();
  snprintf(msg, sizeof(msg), "initialize AK-030 : APN='%s', USER='%s', PASSWD='%s', PPPAUTH='%s'", APN, USER, PASSWD,
           PPPAUTH);
  Serial.println(msg);

  // ak030->debug = true;
  // ak030->begin(APN); // You may omit USER/PASSWRD,PPPAUTH, when apn is 'soracom.io'.
  ak030->begin(APN, USER, PASSWD, PPPAUTH);
}

void loop() {
  char msg[64];
  unsigned long interval = LOOP_INTERVAL;

  bool ok = daytime_demo();
  if (!ok) {
    interval = LOOP_INTERVAL_WHEN_NG;
  }
  snprintf(msg, sizeof(msg), "waiting %.1f sec...", interval / 1000.0);
  Serial.println();
  Serial.println(msg);
  delay(interval);
}