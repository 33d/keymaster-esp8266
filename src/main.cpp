#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <FS.h>
#include <MFRC522.h>
#include <base64encode.h>

struct AccessPoint {
  const char* ap;
  const char* password;
};
#include "settings"

#define log Serial

File apFile;

MFRC522 mfrc522(D2, D1);
int buzzer = D3;

const char* host = "members.hackadl.org";
const char* fingerprint = "15:15:AA:69:72:8C:09:7C:E5:17:27:38:B3:2A:39:89:BC:65:3C:A9";

// The WL_ constants, for debug output (from wl_definitions.h)
const char* wl_status[] = { "IDLE_STATUS", "NO_SSID_AVAIL", "SCAN_COMPLETED",
  "CONNECTED", "CONNECT_FAILED", "CONNECTION_LOST", "DISCONNECTED" };

void ShowReaderDetails() {
  // Get the MFRC522 software version
  byte v = mfrc522.PCD_ReadRegister(mfrc522.VersionReg);
  Serial.print(F("MFRC522 Software Version: 0x"));
  Serial.print(v, HEX);
  if (v == 0x91)
    Serial.print(F(" = v1.0"));
  else if (v == 0x92)
    Serial.print(F(" = v2.0"));
  else
    Serial.print(F(" (unknown)"));
  Serial.println("");
  // When 0x00 or 0xFF is returned, communication probably failed
  if ((v == 0x00) || (v == 0xFF)) {
    Serial.println(F("WARNING: Communication failure, is the MFRC522 properly connected?"));
  }
}

void setup() {
  Serial.begin(115200);
  Serial.println();

  WiFi.setAutoConnect(false);
  WiFi.setAutoReconnect(false);
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();

  SPI.begin(); // for MFRC522
  delay(50);
  mfrc522.PCD_Init();
  delay(50);
  mfrc522.PCD_SetAntennaGain(0xFF);
  ShowReaderDetails();

  if (!SPIFFS.begin())
    log.println("SPIFFS begin failed!");

  apFile = SPIFFS.open("/ap", "r");

  pinMode(LED_BUILTIN, OUTPUT); // Use the built in LED on the ESP8266
}

void buzzer_success() {
  // TODO check that this success tone range works
  for (size_t i = 2000; i < 3000; i = i+80) {
    tone(buzzer,i,100);
    delay(10);
  }
  for (size_t i = 3000; i > 2000; i = i-80) {
    tone(buzzer,i,100);
    delay(10);
  }
}

void buzzer_failure() {
  tone(buzzer,1000,500);
  delay(200);
  tone(buzzer,500,500);
}

void send_card(uint8_t* id, int idLength) {
  char base64[20];
  uint8_t base64Count;

  if (idLength > 10) {
    log.print(idLength);
    log.print(" <- ID is too long\n");
    return; // avoid overflowing base64
  }

  // Pad the base64 data, so it's a consistent length for Content-Length
  memset(base64, ' ', 20);
  base64Count = base64_encode(id, idLength, base64);

  WiFiClientSecure client;
  if (!client.connect(host, 443)) {
    log.println("Connection failed");
    return;
  }
  if (!client.verify(fingerprint, host)) {
    log.println("Fingerprint doesn't match");
    return;
  }

  client.print("POST /checkin HTTP/1.1\r\n"
           "Host: members.hackadl.org\r\n"
           "Content-Type: application/x-www-form-urlencoded\r\n"
           "Content-Length: 29\r\n"
           "Connection: close\r\n\r\nsite=" siteid "&id=");

  client.write((uint8_t*) ((void*) base64), 20);

  // Buzz success if first line of HTTP response contains 2xx
  if (client.readStringUntil('\n').charAt(9) == '2') {
    buzzer_success();
  } else {
    buzzer_failure();
  }

  String line;
  do {
    line = client.readStringUntil('\n');
    line.trim();
    log.print("Received ");
    log.println(line);
  } while (line.length() > 0);

  client.stop();
}

void check_card_reader() {
  if (!mfrc522.PICC_IsNewCardPresent() || !mfrc522.PICC_ReadCardSerial())
    return;

  tone(buzzer, 1000, 100);

  log.print("Scanned PICC's UID: ");
  for (int i = 0; i < 7; i++)
    log.print(mfrc522.uid.uidByte[i], HEX);
  log.println();
  mfrc522.PICC_HaltA();
  send_card(mfrc522.uid.uidByte, mfrc522.uid.size);
}

void connect_to_next_access_point() {
  static const struct AccessPoint *ap = accessPoints;

  tone(buzzer, 500, 50);
  // The chip resets without this; maybe there's not enough power to begin
  // the wifi scan
  delay(1000);

  WiFi.begin(ap->ap, ap->password);
  ++ap;
  if (ap->ap == 0) // the end of the list
    ap = accessPoints;
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
      digitalWrite(LED_BUILTIN, LOW);
      check_card_reader();
      break;
    case WL_IDLE_STATUS:
    case WL_CONNECT_FAILED:
    case WL_NO_SSID_AVAIL:
      digitalWrite(LED_BUILTIN, HIGH);
      connect_to_next_access_point();
      break;
  }

  delay(200);
}
