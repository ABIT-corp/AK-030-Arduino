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

#define SERVER "example.com"
#define PORT   80
#define URI    "/"

#define LOOP_INTERVAL         (1000 * 60 * 1)
#define LOOP_INTERVAL_WHEN_NG (1000 * 5)

static char http_response[5000];

AK030 *ak030 = new AK030();

bool http_get_example() {
  Serial.println();
  Serial.println("::::::::::::::::::::::::::");
  Serial.println("::: http_get_example() :::");
  Serial.println("::::::::::::::::::::::::::");
  Serial.println();

  if (!ak030->connected()) {
    Serial.println("connecting to LTE network");
    ak030->connect();  // connect to LTE network
    if (ak030->ng()) {
      Serial.println("=> cannot connect to LTE network");
      return false;
    }
    Serial.println("=> connected");
  }

  Serial.printf("dns lookup start for %s\n", SERVER);
  const char *ipaddr = ak030->dnsLookup(SERVER);
  if (ak030->ng()) {
    Serial.println("=> dns lookup failed()");
    return false;
  }
  Serial.printf("=> ipaddress=%s\n", ipaddr);

  Serial.printf("open tcp: ipaddr=%s port=%d\n", ipaddr, PORT);
  ak030->openTcp(ipaddr, PORT);
  if (!ak030->ok()) {
    Serial.println("=> cannot open tcp");
    return false;
  }
  Serial.println("=> opened");

  const char req_template[] =
      "GET %s HTTP/1.1\r\n"
      "Host: %s:%d\r\n"
      "Accept: application/json\r\n"
      "\r\n";

  static char req[128];
  snprintf(req, sizeof(req), req_template, URI, SERVER, PORT, SERVER, PORT);

  Serial.printf("http get start: http://%s:%d%s\n", SERVER, PORT, URI);
  ak030->send(req);
  if (!ak030->ok()) {
    Serial.println("=> send() failed");
    ak030->close();
    return false;
  }

  int total_size = 0;
  int left_size = sizeof(http_response);
  ak030->waitEvent(30);  // wait for socket event in 30 seconds(at most)
  while (ak030->eventDataReceived()) {
    int n;
    ak030->receive(http_response + total_size, left_size, &n);
    total_size += n;
    left_size -= n;
    if (left_size <= 0) {
      Serial.println("=> overflow !!!");
      break;
    }
    ak030->waitEvent();
  }
  if (ak030->ng()) {
    Serial.println("=> receive() failed");
    ak030->close();
    return false;
  }

  Serial.printf("received %d bytes\n", total_size);
  Serial.println(">>>> received data begin >>>>");
  Serial.println(http_response);
  Serial.println("<<<< received data end <<<<<<");

  Serial.println("close tcp");
  ak030->close();

  return true;
}

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);

  // count down
  int cnt = 10;
  while (--cnt >= 0) {
    Serial.printf("count down: %d\n", cnt);
    delay(1000);
  }

  Serial.println();
  Serial.println("@@@@@@@@@@@@@@@@@@@@@@");
  Serial.println("http get example start");
  Serial.println("@@@@@@@@@@@@@@@@@@@@@@");

  char msg[128];
  Serial.println();
  snprintf(msg, sizeof(msg), "initialize AK-030 : APN='%s', USER='%s', PASSWD='%s', PPPAUTH='%s'", APN, USER, PASSWD,
           PPPAUTH);
  Serial.println(msg);

  // ak030->debug = 1;
  // ak030->begin(APN); // You may omit USER,PASSWRD,PPPAUTH, when apn is 'soracom.io'.
  ak030->begin(APN, USER, PASSWD, PPPAUTH);
}

void loop() {
  char msg[64];
  unsigned long interval = LOOP_INTERVAL;

  bool ok = http_get_example();
  if (!ok) {
    interval = LOOP_INTERVAL_WHEN_NG;
  }
  snprintf(msg, sizeof(msg), "waiting %.1f sec...", interval / 1000.0);
  Serial.println();
  Serial.println(msg);
  delay(interval);
}