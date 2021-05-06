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

// GET https://httpbin.org/ip
#define SERVER "httpbin.org"
#define PORT   443
#define URI    "/ip"

#define LOOP_INTERVAL         (1000 * 60)
#define LOOP_INTERVAL_WHEN_NG (1000 * 5)

#define _LF "\x0a"  // pseudo line feed for PEM format

#define CERT_NO 3

// clang-format off
const char cert_data_amazon_root_ca_1[] =
"-----BEGIN CERTIFICATE-----" _LF
"MIIDQTCCAimgAwIBAgITBmyfz5m/jAo54vB4ikPmljZbyjANBgkqhkiG9w0BAQsF" _LF
"ADA5MQswCQYDVQQGEwJVUzEPMA0GA1UEChMGQW1hem9uMRkwFwYDVQQDExBBbWF6" _LF
"b24gUm9vdCBDQSAxMB4XDTE1MDUyNjAwMDAwMFoXDTM4MDExNzAwMDAwMFowOTEL" _LF
"MAkGA1UEBhMCVVMxDzANBgNVBAoTBkFtYXpvbjEZMBcGA1UEAxMQQW1hem9uIFJv" _LF
"b3QgQ0EgMTCCASIwDQYJKoZIhvcNAQEBBQADggEPADCCAQoCggEBALJ4gHHKeNXj" _LF
"ca9HgFB0fW7Y14h29Jlo91ghYPl0hAEvrAIthtOgQ3pOsqTQNroBvo3bSMgHFzZM" _LF
"9O6II8c+6zf1tRn4SWiw3te5djgdYZ6k/oI2peVKVuRF4fn9tBb6dNqcmzU5L/qw" _LF
"IFAGbHrQgLKm+a/sRxmPUDgH3KKHOVj4utWp+UhnMJbulHheb4mjUcAwhmahRWa6" _LF
"VOujw5H5SNz/0egwLX0tdHA114gk957EWW67c4cX8jJGKLhD+rcdqsq08p8kDi1L" _LF
"93FcXmn/6pUCyziKrlA4b9v7LWIbxcceVOF34GfID5yHI9Y/QCB/IIDEgEw+OyQm" _LF
"jgSubJrIqg0CAwEAAaNCMEAwDwYDVR0TAQH/BAUwAwEB/zAOBgNVHQ8BAf8EBAMC" _LF
"AYYwHQYDVR0OBBYEFIQYzIU07LwMlJQuCFmcx7IQTgoIMA0GCSqGSIb3DQEBCwUA" _LF
"A4IBAQCY8jdaQZChGsV2USggNiMOruYou6r4lK5IpDB/G/wkjUu0yKGX9rbxenDI" _LF
"U5PMCCjjmCXPI6T53iHTfIUJrU6adTrCC2qJeHZERxhlbI1Bjjt/msv0tadQ1wUs" _LF
"N+gDS63pYaACbvXy8MWy7Vu33PqUXHeeE6V/Uq2V8viTO96LXFvKWlJbYK8U90vv" _LF
"o/ufQJVtMVT8QtPHRh8jrdkPSHCa2XV4cdFyQzR1bldZwgJcJmApzyMZFo6IQ6XU" _LF
"5MsI+yMRQ+hDKXJioaldXgjUkK642M4UwtBV8ob2xJNDd2ZhwLnoQdeXeGADbkpy" _LF
"rqXRfboQnoZsG4q5WTP468SQvvG5" _LF
"-----END CERTIFICATE-----" _LF;

AK030 *ak030 = new AK030();

char http_response[5000];

bool ssl_example() {
  Serial.println();
  Serial.println(":::::::::::::::::");
  Serial.println(":: ssl_example ::");
  Serial.println(":::::::::::::::::");
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
  Serial.printf("dns lookup: %s\n", SERVER);
  const char *ipaddr = ak030->dnsLookup(SERVER);
  if (ak030->ng()) {
    Serial.println("=> dns lookup failed");
    return false;
  }
  Serial.printf("=> %s\n", ipaddr);

  // open tcp socket
  Serial.printf("open tcp: ipaddr=%s, port=%d\n", ipaddr, PORT);
  ak030->openSSL(ipaddr, PORT, CERT_NO);
  if (ak030->ng()) {
    Serial.printf("=> cannot open tcp\n");
    return false;
  } else {
    Serial.println("=> opened");
  }

  const char req_template[] =
      "GET %s HTTP/1.1\r\n"
      "Host: %s:%d\r\n"
      "Accept: application/json\r\n"
      "\r\n";

  static char req[128];
  snprintf(req, sizeof(req), req_template, URI, SERVER, PORT);

  Serial.printf("http get start: http://%s:%d%s\n", SERVER, PORT, URI);
  ak030->send(req);
  if (!ak030->ok()) {
    Serial.println("~> send() failed");
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
      Serial.println("overflow");
      break;
    }
    ak030->waitEvent();
  }
  if (ak030->ng()) {
    Serial.println("tcpReceive() failed");
    ak030->close();
    return false;
  }

  Serial.printf("received %d bytes\n", total_size);
  Serial.println(">>>> received data begin >>>>");
  Serial.println(http_response);
  Serial.println("<<<< received data end <<<<");

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
  Serial.println("@@@@@@@@@@@@@@@@@");
  Serial.println("SSL example start");
  Serial.println("@@@@@@@@@@@@@@@@@");

  char msg[128];
  Serial.println();
  snprintf(msg,sizeof(msg),"initialize AK-030 : APN='%s', USER='%s', PASSWD='%s', PPPAUTH='%s'", APN, USER, PASSWD, PPPAUTH);
  Serial.println(msg);

  // ak030->debug = true;
  //ak030->begin(APN); // You may omit USER/PASSWRD,PPPAUTH, when apn is 'soracom.io'.
  ak030->begin(APN, USER, PASSWD, PPPAUTH);

  ak030->installCertificate("amzn_rt_ca_1.crt", CERT_NO, cert_data_amazon_root_ca_1);
}

void loop() {
  char msg[64];
  unsigned long interval = LOOP_INTERVAL; 
  
  bool ok = ssl_example();
  if (!ok) {
    interval = LOOP_INTERVAL_WHEN_NG;
  }
  snprintf(msg,sizeof(msg),"waiting %.1f sec...", interval / 1000.0);
  Serial.println();
  Serial.println(msg);
  delay(interval);
}