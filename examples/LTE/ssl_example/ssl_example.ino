#include "AK-030.h"
#include "Arduino.h"

#define APN "soracom.io"
//#define SERVER  "www.howsmyssl.com"
#define SERVER  "httpstat.us"
#define PORT    443
#define CERT_NO 3

#define _LF "\x0a"  // pseudo line feed for PEM format

// clang-format off
const char cert_data[] =
  "-----BEGIN CERTIFICATE-----" _LF
  "MIIDdzCCAl+gAwIBAgIEAgAAuTANBgkqhkiG9w0BAQUFADBaMQswCQYDVQQGEwJJ" _LF
  "RTESMBAGA1UEChMJQmFsdGltb3JlMRMwEQYDVQQLEwpDeWJlclRydXN0MSIwIAYD" _LF
  "VQQDExlCYWx0aW1vcmUgQ3liZXJUcnVzdCBSb290MB4XDTAwMDUxMjE4NDYwMFoX" _LF
  "DTI1MDUxMjIzNTkwMFowWjELMAkGA1UEBhMCSUUxEjAQBgNVBAoTCUJhbHRpbW9y" _LF
  "ZTETMBEGA1UECxMKQ3liZXJUcnVzdDEiMCAGA1UEAxMZQmFsdGltb3JlIEN5YmVy" _LF
  "VHJ1c3QgUm9vdDCCASIwDQYJKoZIhvcNAQEBBQADggEPADCCAQoCggEBAKMEuyKr" _LF
  "mD1X6CZymrV51Cni4eiVgLGw41uOKymaZN+hXe2wCQVt2yguzmKiYv60iNoS6zjr" _LF
  "IZ3AQSsBUnuId9Mcj8e6uYi1agnnc+gRQKfRzMpijS3ljwumUNKoUMMo6vWrJYeK" _LF
  "mpYcqWe4PwzV9/lSEy/CG9VwcPCPwBLKBsua4dnKM3p31vjsufFoREJIE9LAwqSu" _LF
  "XmD+tqYF/LTdB1kC1FkYmGP1pWPgkAx9XbIGevOF6uvUA65ehD5f/xXtabz5OTZy" _LF
  "dc93Uk3zyZAsuT3lySNTPx8kmCFcB5kpvcY67Oduhjprl3RjM71oGDHweI12v/ye" _LF
  "jl0qhqdNkNwnGjkCAwEAAaNFMEMwHQYDVR0OBBYEFOWdWTCCR1jMrPoIVDaGezq1" _LF
  "BE3wMBIGA1UdEwEB/wQIMAYBAf8CAQMwDgYDVR0PAQH/BAQDAgEGMA0GCSqGSIb3" _LF
  "DQEBBQUAA4IBAQCFDF2O5G9RaEIFoN27TyclhAO992T9Ldcw46QQF+vaKSm2eT92" _LF
  "9hkTI7gQCvlYpNRhcL0EYWoSihfVCr3FvDB81ukMJY2GQE/szKN+OMY3EU/t3Wgx" _LF
  "jkzSswF07r51XgdIGn9w/xZchMB5hbgF/X++ZRGjD8ACtPhSNzkE1akxehi/oCr0" _LF
  "Epn3o0WC4zxe9Z2etciefC7IpJ5OCBRLbf1wbWsaY71k5h+3zvDyny67G7fyUIhz" _LF
  "ksLi4xaNmjICq44Y3ekQEe5+NauQrz4wlHrQMz2nZQ/1/I6eYs9HRCwBXbsdtTLS" _LF
  "R9I4LtD+gdwyah617jzV/OeBHRnDJELqYzmp" _LF
  "-----END CERTIFICATE-----";
// clang-format on

AK030 *ak030 = new AK030();

char http_response[5000];

void ssl_example() {
  Serial.println();
  Serial.println("@@@@@@@@@@@@@@@@@");
  Serial.println("SSL example start");
  Serial.println("@@@@@@@@@@@@@@@@@");
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
  Serial.printf("dns lookup: %s\n", SERVER);
  const char *ipaddr = ak030->dnsLookup(SERVER);
  if (ak030->ng()) {
    Serial.println("dns lookup failed");
    return;
  }
  Serial.printf("=> %s\n", ipaddr);

  // open tcp socket
  Serial.printf("open tcp: ipaddr=%s, port=%d\n", ipaddr, PORT);
  ak030->openSSL(ipaddr, PORT, CERT_NO);
  if (ak030->ng()) {
    Serial.printf("cannot open tcp\n");
    return;
  } else {
    Serial.println("...opened");
  }

  const char req_template[] =
      "GET /200 HTTP/1.1\r\n"
      "Host: %s:%d\r\n"
      "Accept: application/json\r\n"
      "\r\n";

  char req[128];
  snprintf(req, sizeof(req), req_template, SERVER, PORT);

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
    Serial.println("tcpReceive() failed");
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
  Serial.begin(115200);

  int cnt = 10;
  while (--cnt >= 0) {
    Serial.printf("count down: %d\n", cnt);
    delay(1000);
  }

  ak030->debug = true;
  ak030->begin(APN);

  ak030->installCertificate("Baltimore_CyberTrust_Roo.crt", CERT_NO, cert_data);
}

void loop() {
  ssl_example();
  delay(1000 * 60 * 3);
}
