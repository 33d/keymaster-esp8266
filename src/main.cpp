#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <FS.h>

#define log Serial

File apFile;

const char* host = "members.hackadl.org";
const char* fingerprint = "15:15:AA:69:72:8C:09:7C:E5:17:27:38:B3:2A:39:89:BC:65:3C:A9";

// The WL_ constants, for debug output (from wl_definitions.h)
const char* wl_status[] = { "IDLE_STATUS", "NO_SSID_AVAIL", "SCAN_COMPLETED",
  "CONNECTED", "CONNECT_FAILED", "CONNECTION_LOST", "DISCONNECTED" };

void setup() {
  Serial.begin(115200);
  Serial.println();

  WiFi.setAutoConnect(false);
  WiFi.setAutoReconnect(false);
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();

  if (!SPIFFS.begin())
    log.println("SPIFFS begin failed!");

  apFile = SPIFFS.open("/ap", "r");
}

void check_card_reader() {
  static bool runOnce = false;
  while (!runOnce) {
    WiFiClientSecure client;
    if (!client.connect(host, 443)) {
      log.println("Connection failed");
      break;
    }
    if (!client.verify(fingerprint, host)) {
      log.println("Fingerprint doesn't match");
      break;
    }

    client.print("GET / HTTP/1.1\r\n"
             "Host: members.hackadl.org\r\n"
             "Connection: close\r\n\r\n");

    String line;
    do {
      line = client.readStringUntil('\n');
      line.trim();
      log.print("Received ");
      log.println(line);
    } while (line.length() > 0);

    client.stop();

    break;
  }

  runOnce = true;
}

void connect_to_next_access_point() {
  String line;
  int separator;
  while (1) {
    line = apFile.readStringUntil('\n');
    line.trim();
    separator = line.indexOf(';');
    if (separator != -1)
      break;
    apFile.seek(0, SeekSet);
  }
  String ssid = line.substring(0, separator);
  String password = line.substring(separator + 1);
  WiFi.begin(ssid.c_str(), password.c_str());
}

void loop() {
  wl_status_t status = WiFi.status();
  log.print("Wifi: ");
  log.print(WiFi.SSID());
  log.print(", status ");
  log.print(wl_status[status]);
  log.print(", IP ");
  log.print(WiFi.localIP());
  log.println();

  switch (status) {
    case WL_CONNECTED:
      check_card_reader();
      break;
    case WL_IDLE_STATUS:
    case WL_CONNECT_FAILED:
    case WL_NO_SSID_AVAIL:
      connect_to_next_access_point();
      break;
  }

  delay(200);
}
