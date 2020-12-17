#ifndef ABIT_LTE_H
#define ABIT_LTE_H

#include "Arduino.h"

#define PIN_LED_POWER 27  // Left Side LED(BLUE)
#define PIN_LED_RADIO 32  // Right Side LED(ORANGE)

#define AK030_AT_CMD_RESULT_NONE 0
#define AK030_AT_CMD_OK          1
#define AK030_AT_CMD_ERROR       2
#define AK030_AT_CMD_TIMEOUT     3
#define AK030_AT_CMD_OVERFLOW    4

class AK030 {
private:
  int m_at_cmd_result;

  char m_ip_address[16];  // local ip adress: 000.000.000.000
  char m_dns_lookup[16];  // result of dnsLoookup()

  int m_socket_id;
  bool m_socket_opened;
  bool m_event_data_received;
  bool m_event_data_sent;
  bool m_event_socket_terminated;

  bool m_cmd_ok;

  void send_at_cmd(const char *cmd = NULL);
  void wait_at_cmd_result(int tmout = 10, bool flush = false);

  int at_cmd_result() { return m_at_cmd_result; };
  bool is_at_cmd_error() { return m_at_cmd_result == AK030_AT_CMD_ERROR; };
  bool is_at_cmd_ok() { return m_at_cmd_result == AK030_AT_CMD_OK; };
  bool is_at_cmd_ng() {
    return (m_at_cmd_result == AK030_AT_CMD_ERROR) || (m_at_cmd_result == AK030_AT_CMD_TIMEOUT) ||
           (m_at_cmd_result == AK030_AT_CMD_OVERFLOW);
  };

  bool _get_local_ip_address();
  // receive tcp data (max data size is 1500 bytes)

  void _receive(char *data, int max_size, int *received_size, int *more_bytes);

public:
  bool debug = false;

  AK030();
  ~AK030();

  bool ok() { return m_cmd_ok; }  // return true if previous function call suceeded
  bool ng() { return !m_cmd_ok; }

  void begin(const char *apn);  // begin to connect LTE Network
  void connect();               // connect to LTE network
  bool connected();             // check whether connected to LTE network
  void disconnect();            // disconnect from LTE network
  void cleanup(int n);

  int getRSSI();

  // get local ip address
  const char *ipAddress() { return m_ip_address; }

  // resolve ip address for hostname
  // return NULL if failed
  const char *dnsLookup(const char *hostname);

  // install credential file
  void installCertificate(const char *fname, int num, const char *str);
  // open SSL socket
  void openSSL(const char *ipaddr, int port, int num);

  // open udp socket
  void openUdp(const char *ipaddr, int port);
  // open tcp socket
  void openTcp(const char *ipaddr, int port);
  // return true if socket is opened
  bool opened() { return m_socket_opened; }
  // close socket
  void close();
  // send data (max 1500 bytes)
  // if length is omitted, length is strlen(data)
  void send(const char *data, int length = 0);
  // receive data
  void receive(char *data, int max_size, int *received_size);

  // wait for socket event
  void waitEvent(int tmout = 2);
  // return true if socket data arrived
  bool eventDataReceived() { return m_event_data_received; }
  // return true if socket terminated by peer
  bool eventTerminated() { return m_event_socket_terminated; }
};

#endif
