#include <SoftwareSerial.h>

#include "Adafruit_BLE.h"
#include "Adafruit_BluefruitLE_UART.h"

#include "BluefruitConfig.h"

//#define FACTORYRESET_ENABLE         1
#define FACTORYRESET_ENABLE         0
#define MINIMUM_FIRMWARE_VERSION    "0.6.6"
#define MODE_LED_BEHAVIOUR          "MODE"

#define NUM_SAMPLES 50
const int sensor_pin = A0;
const int enable_sensor_pin = 7;

SoftwareSerial bluefruitSS = SoftwareSerial(BLUEFRUIT_SWUART_TXD_PIN,
                                            BLUEFRUIT_SWUART_RXD_PIN);

Adafruit_BluefruitLE_UART ble(bluefruitSS,
                              BLUEFRUIT_UART_MODE_PIN,
                              BLUEFRUIT_UART_CTS_PIN,
                              BLUEFRUIT_UART_RTS_PIN);
                      
void error(const __FlashStringHelper* err) {
  Serial.println(err);
  while (1);
}                              

void connect_to_host() {
  ble.reset();

  ble.echo(false);

  Serial.println(F("Requesting Bluefruit info:"));
  ble.info();

  ble.verbose(false);

  Serial.println(F("Waiting for connection"));

  while (!ble.isConnected())
    delay(500);

  Serial.println(F("Connected"));

  if (ble.isVersionAtLeast(MINIMUM_FIRMWARE_VERSION)) {
    Serial.println(F("******************************"));
    Serial.println(F("Change LED activity to " MODE_LED_BEHAVIOUR));
    ble.sendCommandCheckOK("AT+HWModeLED=" MODE_LED_BEHAVIOUR);
    Serial.println(F("******************************"));
  }
}

void setup() {
  while (!Serial);
  delay(500);
  
  Serial.begin(115200);

  Serial.print(F("Initializing the Bluefruit LE module: "));
  if (!ble.begin(VERBOSE_MODE))
    error(F("Couldn't find Bluefruit"));
  Serial.println(F("OK!"));

  if (FACTORYRESET_ENABLE) {
    Serial.println(F("Performing a factory reset: "));
    if (!ble.factoryReset()) {
      error(F("Couldn't factory reset"));
    }
  }

  connect_to_host();
  
  pinMode(enable_sensor_pin, OUTPUT);
}

int read_sensor(int samples[]) {
  Serial.println(F("-- Enabling sensor --"));
  digitalWrite(enable_sensor_pin, HIGH);
  for (int i = 0; i < NUM_SAMPLES; i++) {
    delay(500);
    samples[i] = analogRead(sensor_pin);
    Serial.print(F("Sensor value: "));
    Serial.println(samples[i]);
  }
  Serial.println(F("-- Disabling sensor --"));
  digitalWrite(enable_sensor_pin, LOW);
}

void send_packet(int sample) {
  Serial.print(F("Sending sample: "));
  Serial.println(sample);

  ble.print("AT+BLEUARTTX=");
  ble.print("B");
  ble.print(sample);
  ble.println("E");
  if (!ble.waitForOK())
    Serial.println(F("Failed to send?"));
}

void loop() {
  int samples[NUM_SAMPLES];
  read_sensor(samples);

  for (int i = 0; i < NUM_SAMPLES; i++)
    send_packet(samples[i]);

  delay(30 * 1000);

  if (!ble.isConnected()) {
    Serial.println(F("Disconnected!"));
    connect_to_host();
  }
}

