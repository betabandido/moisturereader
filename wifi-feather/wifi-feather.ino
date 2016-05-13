#include <ESP8266WiFi.h>
#include <PrintEx.h>

#include "ssid-info.h"

PrintEx serial = Serial;

static void enable_wifi() {
}

static void disable_wifi() {
}

static void connect_wifi() {
  serial.printf("Connecting to: %s\n", ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    serial.print(".");
  }
  serial.print(" connected\n");
}

void setup() {
  Serial.begin(115200);
  delay(100);
}

void loop() {
}

