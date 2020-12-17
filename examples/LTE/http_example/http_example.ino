#include "AK-030.h"
#include "Arduino.h"

#define SERVER "example.com"
#define PORT   80

static char http_response[5000];

AK030 *ak030 = new AK030();

void http_get_example() {
  Serial.println();
  Serial.println("@@@@@@@@@@@@@@@@@@@@@@");
  Serial.println("http get example start");
  Serial.println("@@@@@@@@@@@@@@@@@@@@@@");

  if (!ak030->connected()) {
    Serial.println("connecting to LTE network");
    ak030->connect();  // connect to LTE network
    if (ak030->ng()) {
      Serial.println("cannot connect to LTE network");
      return;
    }
    Serial.println("...connected");
  }

  Serial.printf("dns lookup start for %s\n", SERVER);
  const char *ipaddr = ak030->dnsLookup(SERVER);
  if (!ipaddr) {
    Serial.println("dns lookup failed()");
    return;
  }
  Serial.printf("=> ipaddress=%s\n", ipaddr);

  Serial.printf("open tcp: ipaddr=%s port=%d\n", ipaddr, PORT);
  ak030->openTcp(ipaddr, PORT);
  if (!ak030->ok()) {
    Serial.println("cannot open tcp");
    return;
  }
  Serial.println("...opened");

  // get http://example.com:80/
  const char req_template[] =
      "GET /200 HTTP/1.1\r\n"
      "Host: %s:%d\r\n"
      "Accept: application/json\r\n"
      "\r\n";

  char req[128];
  snprintf(req, sizeof(req), req_template, SERVER, PORT, SERVER, PORT);

  Serial.printf("http get start: http://%s:%d/200\n", SERVER, PORT);
  ak030->send(req);
  if (!ak030->ok()) {
    Serial.println("send() failed");
    return;
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
      Serial.println("overflow");
      break;
    }
    ak030->waitEvent();
  }
  if (ak030->ng()) {
    Serial.println("receive() failed");
    ak030->close();
    return;
  }

  Serial.printf("received %d bytes\n", total_size);
  Serial.println("===== recieved data begin =====");
  Serial.println(http_response);
  Serial.println("===== recieved data end =======");

  Serial.println("close tcp");
  ak030->close();
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

  // ak030->debug = 1;
  ak030->begin("soracom.io");
}

void loop() {
  http_get_example();
  delay(1000 * 60 * 3);
}
