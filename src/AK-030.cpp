#include "AK-030.h"

#include "Arduino.h"

#define AT_RESULT_OK    "OK\r\n"
#define AT_RESULT_ERROR "ERROR\r\n"

#define MAX_DATA_LEN 1500

char cmd_buffer[(MAX_DATA_LEN * 2) + 1 + 128];

/**
 * @brief send AT command to LTE-Module
 *
 * @param cmd : at command(string) to send
 */
void AK030::send_at_cmd(const char *cmd) {
  if (cmd == NULL) {
    cmd = cmd_buffer;
  }
  if (debug) {
    Serial.print("<< ");
    Serial.println(cmd);
  }

  Serial2.print(cmd);
  Serial2.print("\r");
  m_at_cmd_result = AK030_AT_CMD_RESULT_NONE;
}

/**
 * @brief wait AT command result from LTE-Module
 *
 * @param tmout : timeout seconds
 */
void AK030::wait_at_cmd_result(int tmout, bool flash) {
  int cnt = 0;

  if (flash) {
    Serial2.flush();
  }

  memset(cmd_buffer, 0, sizeof(cmd_buffer));
  unsigned long tm = millis();

  bool prev_lf = true;
  while (1) {
    while (Serial2.available()) {
      int ch = Serial2.read();
      if (ch >= 0) {
        if (cnt >= sizeof(cmd_buffer)) {
          // TODO : Error処理
          if (debug) {
            Serial.println();
            Serial.println("wait_at_cmd_result() : BUFFER_OVER_FLOW");
          }
          m_at_cmd_result = AK030_AT_CMD_OVERFLOW;
          return;
        }
        cmd_buffer[cnt++] = ch;
        if (debug) {
          if (prev_lf) {
            Serial.print("|");
          }
          Serial.write(ch);
          if (ch == '\n') {
            prev_lf = true;
          } else {
            prev_lf = false;
          }
        }
        if (prev_lf) {
          if (strstr(cmd_buffer + cnt - sizeof(AT_RESULT_OK), AT_RESULT_OK)) {
            if (debug) Serial.println(">> AT_CMD_OK");
            m_at_cmd_result = AK030_AT_CMD_OK;
            return;
          }
          if (strstr(cmd_buffer + cnt - sizeof(AT_RESULT_ERROR), AT_RESULT_ERROR)) {
            if (debug) Serial.println(">> AT_CMD_ERROR");
            m_at_cmd_result = AK030_AT_CMD_ERROR;
            return;
          }
        }
        tm = millis();
      }
    }
    if (millis() - tm > tmout * 1000) {
      if (debug) {
        Serial.println();
        Serial.print(">> AT_CMD_TIMEOUT");
      }
      m_at_cmd_result = AK030_AT_CMD_TIMEOUT;
      break;
    }
  }
}

AK030::AK030() {
  if (debug) Serial.print("AK030 Initialize\r\n");
  Serial2.begin(115200, SERIAL_8N1, 16, 17);  // LTE Module
  Serial2.flush();

  m_at_cmd_result = AK030_AT_CMD_RESULT_NONE;
  m_socket_id = 0;
  m_socket_opened = false;
  m_event_data_received = false;
  m_event_data_sent = false;
  m_event_socket_terminated = false;

  m_cmd_ok = false;

  debug = false;

  memset(m_ip_address, 0, sizeof(m_ip_address));
  memset(m_dns_lookup, 0, sizeof(m_dns_lookup));

  // disconnect();
}

/**
 * @brief AK030 preparation to connect
 *
 * @param apn : APN string
 * @param user : user
 * @param passwd : password
 * @param pppauth: PAP/CHAP/NONE
 */
void AK030::begin(const char *apn, const char *user, const char *passwd, const char *pppauth) {
  if (debug) {
    Serial.println();
    Serial.printf("****************\n");
    Serial.printf("* AK030::begin()\n");
    Serial.printf("* APN: %s\n", apn);
    Serial.println();
  }

  send_at_cmd("AT");
  wait_at_cmd_result();
  send_at_cmd("AT");
  wait_at_cmd_result();
  send_at_cmd("ATE0");
  wait_at_cmd_result();

  if (strlen(pppauth) > 0) {
    if (strcmp(pppauth, "PAP") == 0 || strcmp(pppauth, "CHAP") == 0 || strcmp(pppauth, "NONE") == 0) {
      // ok
    } else {
      m_cmd_ok = false;
      if (debug) {
        Serial.printf("Invalid pppauth : '%s' !", pppauth);
      }
      return;
    }
  }

#if 0
  snprintf(cmd_buffer, sizeof(cmd_buffer), "AT+CGDCONT=1,\"IP\",\"%s\"", apn);
  send_at_cmd();
  wait_at_cmd_result();
#else
  snprintf(cmd_buffer, sizeof(cmd_buffer), "AT%%PDNSET=1,\"%s\",\"%s\",\"%s\",\"%s\",\"%s\"", apn, "IP", pppauth, user,
           passwd);
  send_at_cmd();
  wait_at_cmd_result(10);
  if (is_at_cmd_ng()) {
    m_cmd_ok = false;
    return;
  }
#endif

  send_at_cmd("AT+CFUN=0");
  wait_at_cmd_result();
  if (is_at_cmd_ng()) {
    Serial.println("AT+CFUN failed");
    m_cmd_ok = false;
    return;
  }
  send_at_cmd("AT%CMATT=0");
  wait_at_cmd_result();
  if (is_at_cmd_ng()) {
    Serial.println("AT%CMATT=0 failed");
    m_cmd_ok = false;
    return;
  }

  for (int i = 1; i <= 4; i++) {
    cleanup(i);
  }

  m_cmd_ok = true;
}

/**
 * @brief connect to LTE network
 *
 */
void AK030::connect() {
  if (debug) {
    Serial.println();
    Serial.println("******************");
    Serial.println("* AK030::connect()");
    Serial.println();
  }
  memset(m_ip_address, 0, sizeof(m_ip_address));

  Serial2.flush();

  send_at_cmd("AT+CFUN=0");
  wait_at_cmd_result();
  if (is_at_cmd_ng()) {
    m_cmd_ok = false;
    return;
  }
  send_at_cmd("AT+CFUN=1");
  wait_at_cmd_result();
  if (is_at_cmd_ng()) {
    m_cmd_ok = false;
    return;
  }
  send_at_cmd("AT%CMATT=1");
  wait_at_cmd_result();
  if (is_at_cmd_ng()) {
    m_cmd_ok = false;
    return;
  }
  int retry_cnt = 30;
  bool ok = false;
  while (--retry_cnt >= 0) {
    if (connected()) {
      ok = true;
      break;
    }
    delay(1000);
  }
  if (!ok) {
    m_cmd_ok = false;
    return;
  }

  Serial2.flush();

  ////////////////////
  // Get IP Address
  ////////////////////
  retry_cnt = 30;
  ok = false;
  while (--retry_cnt >= 0) {
    if (_get_local_ip_address()) {
      ok = true;
      break;
    }
    delay(1000);
  }
  m_cmd_ok = ok;
}

/**
 * @brief disconnect to LTE network
 *
 */
void AK030::disconnect() {
  if (debug) {
    Serial.println();
    Serial.println("*********************");
    Serial.println("* AK030::disconnect()");
    Serial.println();
  }
  memset(m_ip_address, 0, sizeof(m_ip_address));

  send_at_cmd("AT+CFUN=0");
  wait_at_cmd_result();
  if (is_at_cmd_ng()) {
    m_cmd_ok = false;
    return;
  }
  send_at_cmd("AT%CMATT=0");
  wait_at_cmd_result();
  if (is_at_cmd_ng()) {
    m_cmd_ok = false;
    return;
  }
  m_cmd_ok = true;
}

/**
 * @brief cleanup socket
 *
 */
void AK030::cleanup(int n) {
  if (debug) Serial.printf("* cleanup socket %d\n", n);
  snprintf(cmd_buffer, sizeof(cmd_buffer), "AT%%SOCKETCMD=\"DEACTIVATE\",%d", n);
  send_at_cmd();
  wait_at_cmd_result();
  snprintf(cmd_buffer, sizeof(cmd_buffer), "AT%%SOCKETCMD=\"DELETE\",%d", n);
  send_at_cmd();
  wait_at_cmd_result();
}

bool AK030::_get_local_ip_address() {
  memset(m_ip_address, 0, sizeof(m_ip_address));

  send_at_cmd("AT+CGPADDR");
  wait_at_cmd_result();
  if (is_at_cmd_ng()) {
    if (debug) Serial.println("* _get_local_ip_address() failed");
    m_cmd_ok = false;
    return false;
  }
  // +CGPADDR: 1,"10.184.120.27"
  if (strstr(cmd_buffer, "+CGPADDR: 1,")) {
    int i = 0;
    char *p = strchr(cmd_buffer, '\"');
    if (!p) {
      if (debug) Serial.println("* _get_local_ip_address() failed 2");
      m_cmd_ok = false;
      return false;
    }
    p++;
    while (*p != '\"' && i < sizeof(m_ip_address)) {
      m_ip_address[i++] = *p++;
    }
    if (debug) Serial.printf("* _get_local_ip_address(): %s\r\n", m_ip_address);
    m_cmd_ok = true;
    return true;
  } else {
    if (debug) Serial.println("* _get_local_ip_address() failed 3");
    m_cmd_ok = false;
    return false;
  }
}

/**
 * @brief check whether AK-030 is connected to LTE network
 *
 * @return boolean
 */
bool AK030::connected() {
  send_at_cmd("AT%CMATT?");
  wait_at_cmd_result();
  if (is_at_cmd_ng()) {
    m_cmd_ok = false;
    return false;
  }
  if (strstr(cmd_buffer, "%CMATT: 1")) {
    m_cmd_ok = true;
    return true;
  }
  if (strstr(cmd_buffer, "%CMATT: 0")) {
    m_cmd_ok = true;
    return false;
  }
  m_cmd_ok = false;
  return false;
}

/**
 * @brief find ip address for hostname
 *
 * @param hostname
 * @return const char* : ip_address
 */
const char *AK030::dnsLookup(const char *hostname) {
  if (debug) {
    Serial.println();
    Serial.println("*********************");
    Serial.println("* AK030::dns_lookup()");
    Serial.printf("* hostname=%s\n", hostname);
    Serial.println();
  }
  memset(m_dns_lookup, 0, sizeof(m_dns_lookup));
  // AT%DNSRSLV=0,"example.com"
  snprintf(cmd_buffer, sizeof(cmd_buffer), "AT%%DNSRSLV=0,\"%s\"", hostname);
  send_at_cmd();
  wait_at_cmd_result(60);
  if (is_at_cmd_ng()) {
    m_cmd_ok = false;
    return NULL;
  }
  /*
  AT%DNSRSLV=1,"example.com"

  %DNSRSLV:0,"93.184.216.34"
  %DNSRSLV:0,"2606:2800:220:1:248:1893:25C8:1946"
  */
  char *p = strstr(cmd_buffer, "%DNSRSLV:0,\"");
  if (!p) {
    if (debug) Serial.println("* dns_lookup failed");
    m_cmd_ok = false;
    return NULL;
  }
  p = strchr(p, '\"');
  p++;
  int i = 0;
  while (*p != '\"' && i < sizeof(m_dns_lookup)) {
    m_dns_lookup[i++] = *p++;
  }
  m_cmd_ok = true;
  return m_dns_lookup;
}

// install credential file
void AK030::installCertificate(const char *fname, int num, const char *str) {
  if (debug) {
    Serial.println();
    Serial.println("******************");
    Serial.println("* AK030::installCertificate()");
    Serial.printf("* fname=%s, num=%d\n", fname, num);
    Serial.println();
  }
  // snprintf(cmd_buffer, sizeof(cmd_buffer), "AT%%CERTCMD=\"WRITE\",\"%s\",0,\r\n\"%s\"", fname, str);
  snprintf(cmd_buffer, sizeof(cmd_buffer), "AT%%CERTCMD=\"WRITE\",\"%s\",0,\"%s\"", fname, str);
  send_at_cmd();
  wait_at_cmd_result(60);
  if (is_at_cmd_ng()) {
    if (debug) Serial.println("* installCertificate() ERROR: 1");
    m_cmd_ok = false;
    return;
  }
  snprintf(cmd_buffer, sizeof(cmd_buffer), "AT%%CERTCFG=\"ADD\",%d,\"%s\"", num, fname);
  send_at_cmd();
  wait_at_cmd_result(60);
  if (is_at_cmd_ng()) {
    if (debug) Serial.println("* installCertificate() ERROR: 2");
    m_cmd_ok = false;
    return;
  }
  m_cmd_ok = true;
}
// open SSL socket
void AK030::openSSL(const char *ipaddr, int port, int num) {
  if (debug) {
    Serial.println();
    Serial.println("******************");
    Serial.println("* AK030::openSSL()");
    Serial.printf("* ipaddr=%s, port=%d, \n", ipaddr, port);
    Serial.println();
  }
  if (m_socket_opened) {
    if (debug) {
      Serial.println("* cannot open multiple sockets");
    }
    m_cmd_ok = false;
    return;
  }

  m_socket_opened = false;
  // AT%SOCKETCMD="ALLOCATE",0,"TCP","OPEN","%s",%d
  snprintf(cmd_buffer, sizeof(cmd_buffer), "AT%%SOCKETCMD=\"ALLOCATE\",0,\"TCP\",\"OPEN\",\"%s\",%d", ipaddr, port);
  send_at_cmd();
  wait_at_cmd_result(60);
  if (is_at_cmd_ng()) {
    if (debug) Serial.println("* openSSL() ERROR: 1");
    m_cmd_ok = false;
    return;
  }
  // %SOCKETCMD:1
  // OK
  char *p = strstr(cmd_buffer, "%SOCKETCMD:");
  if (!p) {
    if (debug) Serial.println("* openSSL() ERROR: 2");
    m_cmd_ok = false;
    return;
  }
  p = strchr(cmd_buffer, ':');
  m_socket_id = atoi(++p);
  if (debug) {
    Serial.printf("* openSSL() ALLOCATE OK: socket_id=%d\r\n", m_socket_id);
  }

  snprintf(cmd_buffer, sizeof(cmd_buffer), "AT%%SOCKETCMD=\"SSLALLOC\",%d,0,%d", m_socket_id, num);
  send_at_cmd();
  wait_at_cmd_result(60);
  if (is_at_cmd_ng()) {
    if (debug) Serial.println("* openSSL() ERROR: 3");
    cleanup(m_socket_id);
    m_cmd_ok = false;
    return;
  }
  if (debug) Serial.println("* openSSL() SSLALLOC OK");

  snprintf(cmd_buffer, sizeof(cmd_buffer), "AT%%SOCKETCMD=\"ACTIVATE\",%d", m_socket_id);
  send_at_cmd();
  wait_at_cmd_result(60);
  if (is_at_cmd_ng()) {
    if (debug) Serial.println("* openSSL() ERROR: 4");
    cleanup(m_socket_id);
    m_cmd_ok = false;
    return;
  }
  if (debug) Serial.println("* openTcp() ACTIVATE OK");
  m_socket_opened = true;
  m_cmd_ok = true;
  return;
}

/**
 * @brief open tcp socket
 *
 * @param ipaddr
 * @param port
 */
void AK030::openTcp(const char *ipaddr, int port) {
  if (debug) {
    Serial.println();
    Serial.println("******************");
    Serial.println("* AK030::openTcp()");
    Serial.printf("* ipaddr=%s, port=%d\n", ipaddr, port);
    Serial.println();
  }
  if (m_socket_opened) {
    if (debug) {
      Serial.println("* cannot open multiple sockets");
    }
    m_cmd_ok = false;
    return;
  }

  m_socket_opened = false;
  // AT%SOCKETCMD="ALLOCATE",0,"TCP","OPEN","%s",%d
  snprintf(cmd_buffer, sizeof(cmd_buffer), "AT%%SOCKETCMD=\"ALLOCATE\",0,\"TCP\",\"OPEN\",\"%s\",%d", ipaddr, port);
  send_at_cmd();
  wait_at_cmd_result(60);
  if (is_at_cmd_ng()) {
    if (debug) Serial.println("* openTcp() ERROR: 1");
    m_cmd_ok = false;
    return;
  }
  // %SOCKETCMD:1
  // OK
  char *p = strstr(cmd_buffer, "%SOCKETCMD:");
  if (!p) {
    if (debug) Serial.println("* openTcp() ERROR: 2");
    m_cmd_ok = false;
    return;
  }
  p = strchr(cmd_buffer, ':');
  m_socket_id = atoi(++p);
  if (debug) {
    Serial.printf("* openTcp() ALLOCATE OK: socket_id=%d\r\n", m_socket_id);
  }

  snprintf(cmd_buffer, sizeof(cmd_buffer), "AT%%SOCKETCMD=\"ACTIVATE\",%d", m_socket_id);
  send_at_cmd();
  wait_at_cmd_result(60);
  if (is_at_cmd_ng()) {
    if (debug) Serial.println("* openTcp() ERROR: 3");
    cleanup(m_socket_id);
    m_cmd_ok = false;
    return;
  }
  if (debug) Serial.println("* openTcp() ACTIVATE OK");
  m_socket_opened = true;
  m_cmd_ok = true;
  return;
}

/**
 * @brief open udp socket
 *
 * @param ipaddr
 * @param port
 */
void AK030::openUdp(const char *ipaddr, int port) {
  if (debug) {
    Serial.println();
    Serial.println("******************");
    Serial.println("* AK030::openUdp()");
    Serial.printf("* ipaddr=%s, port=%d\n", ipaddr, port);
    Serial.println();
  }
  if (m_socket_opened) {
    if (debug) {
      Serial.println("* cannot open multiple sockets");
    }
    m_cmd_ok = false;
    return;
  }

  m_socket_opened = false;
  // AT%SOCKETCMD="ALLOCATE",0,"TCP","OPEN","%s",%d
  snprintf(cmd_buffer, sizeof(cmd_buffer), "AT%%SOCKETCMD=\"ALLOCATE\",0,\"UDP\",\"OPEN\",\"%s\",%d", ipaddr, port);
  send_at_cmd();
  wait_at_cmd_result(60);
  if (is_at_cmd_ng()) {
    if (debug) Serial.println("* openUdp() ERROR: 1");
    m_cmd_ok = false;
    return;
  }
  // %SOCKETCMD:1
  // OK
  char *p = strstr(cmd_buffer, "%SOCKETCMD:");
  if (!p) {
    if (debug) Serial.println("* openUdp() ERROR: 2");
    m_cmd_ok = false;
    return;
  }
  p = strchr(cmd_buffer, ':');
  m_socket_id = atoi(++p);
  if (debug) {
    Serial.printf("* openUdp() ALLOCATE OK: socket_id=%d\r\n", m_socket_id);
  }

  // AT%SOCKETCMD="SETOPT",1,3600,1
  snprintf(cmd_buffer, sizeof(cmd_buffer), "AT%%SOCKETCMD=\"SETOPT\",%d,3600,1", m_socket_id);
  send_at_cmd();
  wait_at_cmd_result(60);
  if (is_at_cmd_ng()) {
    if (debug) Serial.println("* openUdp() ERROR: 3");
    cleanup(m_socket_id);
    m_cmd_ok = false;
    return;
  }

  snprintf(cmd_buffer, sizeof(cmd_buffer), "AT%%SOCKETCMD=\"ACTIVATE\",%d", m_socket_id);
  send_at_cmd();
  wait_at_cmd_result(60);
  if (is_at_cmd_ng()) {
    if (debug) Serial.println("* openUdp() ERROR: 4");
    cleanup(m_socket_id);
    m_cmd_ok = false;
    return;
  }
  if (debug) Serial.println("* openUdp() ACTIVATE OK");
  m_socket_opened = true;
  m_cmd_ok = true;
  return;
}

/**
 * @brief wait for socket event
 *
 * @param tmout
 */
void AK030::waitEvent(int tmout) {
  if (debug) {
    Serial.println();
    Serial.println("*******************");
    Serial.println("* waitEvent()");
  }

  m_event_data_received = false;
  m_event_socket_terminated = false;

  int cnt = 0;
  memset(cmd_buffer, 0, sizeof(cmd_buffer));
  unsigned long tm = millis();
  bool prev_lf = true;
  while (1) {
    if (Serial2.available()) {
      int ch = Serial2.read();
      if (ch >= 0) {
        if (cnt >= sizeof(cmd_buffer)) {
          // TODO : Error処理
          m_at_cmd_result = AK030_AT_CMD_OVERFLOW;
          m_cmd_ok = false;
          return;
        }
        cmd_buffer[cnt++] = ch;
        if (debug) {
          if (prev_lf) Serial.print("|");
          Serial.write(ch);
          if (ch == '\n') {
            prev_lf = true;
          } else {
            prev_lf = false;
          }
        }
        tm = millis();
        // %SOCKETCMD:1,socket_id
        if (strstr((const char *)cmd_buffer, "%SOCKETEV:")) {
          char *p = strchr(cmd_buffer, ':');
          // p points to ":1,3\r\n"
          if (p && p[0] == ':' && p[2] == ',' && p[4] == '\r' && p[5] == '\n') {
            switch (p[1]) {
              case '1':
                m_event_data_received = true;
                m_cmd_ok = true;
                if (debug) Serial.println("* => data_received\n");
                break;
              case '3':
                m_event_socket_terminated = true;
                m_cmd_ok = true;
                if (debug) Serial.println("* => terminated\n");
                break;
              default:
                if (debug) {
                  Serial.printf("* => unknown event: %c\n\n", p[5]);
                }
                m_cmd_ok = false;
                break;
            }  // switch
            return;
          }
        }
      }
    }
    if (millis() - tm > tmout * 1000) {
      if (debug) Serial.println("* => none\n");
      // m_at_cmd_result = AK030_AT_CMD_TIMEOUT;
      m_cmd_ok = true;
      break;
    }
  }
}

/**
 * @brief send data to socket
 *
 * @param data
 * @param length
 */
void AK030::send(const char *data, int length) {
  if (debug) {
    Serial.println();
    Serial.println("************");
    Serial.println("* send()");
    Serial.println();
  }
  if (length == 0) {
    length = strlen(data);
  }
  memset(cmd_buffer, 0, sizeof(cmd_buffer));
  // AT%SOCKETDATA="SEND",m_socket_id,length,
  int len = snprintf(cmd_buffer, sizeof(cmd_buffer), "AT%%SOCKETDATA=\"SEND\",%d,%d,\"", m_socket_id, length);
  unsigned char *c = (unsigned char *)data;
  char *d = cmd_buffer + len;
  for (int i = 0; i < length; i++) {
    sprintf(d, "%02X", *c);
    d++;
    d++;
    c++;
  }
  *d++ = '"';
  *d++ = '\r';
  *d++ = '\n';
  if (debug) {
    Serial.print("<< ");
    Serial.println(cmd_buffer);
  }
  Serial2.print(cmd_buffer);
  wait_at_cmd_result(120);
  if (is_at_cmd_ng()) {
    if (debug) Serial.println("* send() => ERROR");
    m_cmd_ok = false;
    return;
  }
  if (debug) {
    Serial.println("* send() => OK");
  }
  m_cmd_ok = true;
}

uint8_t decode_hh(const char *ch) {
  unsigned int n;
  char c = toupper(ch[0]);
  if (c >= '0' && c <= '9') {
    n = c - '0';
  } else {
    n = c - 'A' + 10;
  }
  n = n << 4;
  c = toupper(ch[1]);
  if (c >= '0' && c <= '9') {
    n += c - '0';
  } else {
    n += c - 'A' + 10;
  }
  if (0) Serial.printf("%02x => %c\n", n, n);
  return n;
}

/**
 * @brief receive data from tcp socket
 *
 * @param data
 * @param max_size
 * @param received_size
 */
void AK030::_receive(char *data, int max_size, int *received_size, int *more_bytes) {
  // AT%SOCKETCMD="RECEIVE",m_socket_id,max_size
  snprintf(cmd_buffer, sizeof(cmd_buffer), "AT%%SOCKETDATA=\"RECEIVE\",%d,%d", m_socket_id, MAX_DATA_LEN);
  send_at_cmd();
  wait_at_cmd_result(60);
  if (is_at_cmd_ng()) {
    Serial.println();
    Serial.println("* _receive() => ERROR");
    m_cmd_ok = false;
    return;
  }
  // %SOCKETDATA:socket_id,length,more_flag,"hex_data"
  char *p = strstr(cmd_buffer, "%SOCKETDATA:");
  if (!p) {
    if (debug) {
      Serial.println("* _receive() => RECEIVE ERROR");
    }
    m_cmd_ok = false;
    return;
  }
  p = strchr(cmd_buffer, ':');
  int socket_id = atoi(++p);
  p = strchr(p, ',');
  int length = atoi(++p);
  p = strchr(p, ',');
  int more = atoi(++p);
  p = strchr(p, '\"');
  p++;

  if (debug) {
    Serial.printf("* _receive() => socket_id=%d len=%d more=%d\n", socket_id, length, more);
  }
  unsigned char *d = (unsigned char *)data;
  memset(d, 0, max_size);
  int cnt = 0;
  for (int i = 0; i < length; i++) {
    d[i] = decode_hh(p);
    // if (debug) {
    //   Serial.write(p[0]);
    //   Serial.write(p[1]);
    //   Serial.println();
    // }
    p += 2;
    ++cnt;
    if (cnt >= max_size) {
      if (debug) {
        Serial.println("* _receive() => overflow");
      }
      *received_size = cnt;
      *more_bytes = more;
      m_cmd_ok = false;
      return;
    }
  }
  // assert *p == '"';
  *received_size = length;
  *more_bytes = more;
  if (debug) {
    Serial.print("* _receive() => data: ");
    Serial.println(data);
  }
  m_cmd_ok = true;
}

// receive data
void AK030::receive(char *data, int max_size, int *received_size) {
  int cnt = 0;
  int left_size = max_size;
  int len;
  int more;
  while (1) {
    _receive(data + cnt, left_size, &len, &more);
    if (!m_cmd_ok) {
      return;
    }
    cnt += len;
    left_size -= len;
    if (more == 0) {
      break;
    }
  }
  *received_size = cnt;
  m_cmd_ok = true;
}

/**
 * @brief close socket
 *
 */
void AK030::close() {
  snprintf(cmd_buffer, sizeof(cmd_buffer), "AT%%SOCKETCMD=\"DEACTIVATE\",%d", m_socket_id);
  send_at_cmd();
  wait_at_cmd_result();
  if (is_at_cmd_ng()) {
    if (debug) Serial.println("* close(): DEACTIVATE ERROR");
    m_cmd_ok = false;
  } else {
    m_cmd_ok = true;
  }
  snprintf(cmd_buffer, sizeof(cmd_buffer), "AT%%SOCKETCMD=\"DELETE\",%d", m_socket_id);
  send_at_cmd();
  wait_at_cmd_result();
  if (is_at_cmd_ng()) {
    if (debug) Serial.println("* close(): DELETE ERROR");
    m_cmd_ok = false;
  } else {
    m_cmd_ok = true;
  }
  m_socket_opened = false;
}

int AK030::getRSSI() {
  snprintf(cmd_buffer, sizeof(cmd_buffer), "AT%%MEAS=\"8\"");
  send_at_cmd();
  wait_at_cmd_result(100, true);

  if (is_at_cmd_ng()) {
    if (debug) Serial.println("* getRSSI(): ERROR 1");
    m_cmd_ok = false;
    delay(1000 * 3);
    return -999;
  }

  char *p;
  p = strstr(cmd_buffer, "RSSI = N/A");
  if (p != NULL) {
    if (debug) Serial.println("* getRSSI(): ERROR 2");
    m_cmd_ok = false;
    return -999;
  }
  p = strstr(cmd_buffer, "RSSI = ");
  if (p == NULL) {
    if (debug) Serial.println("* getRSSI(): ERROR 3");
    m_cmd_ok = false;
    return -999;
  }
  m_cmd_ok = true;
  return atoi(p + sizeof("RSSI = ") - 1);
}