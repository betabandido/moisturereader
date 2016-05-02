#include <SoftwareSerial.h>

#include "Adafruit_BLE.h"
#include "Adafruit_BluefruitLE_UART.h"

#include "BluefruitConfig.h"

//#define FACTORYRESET_ENABLE         1
#define FACTORYRESET_ENABLE         0
#define MINIMUM_FIRMWARE_VERSION    "0.6.6"
#define MODE_LED_BEHAVIOUR          "MODE"

#define TIME_BETWEEN_READINGS 30 * 60 * 1000UL
#define WARMUP_TIME 10 * 1000UL
#define NUM_SAMPLES 30
#define DELAY_BETWEEN_SAMPLES 500

const int sensor_pin = A0;
const int enable_sensor_pin = 7;

SoftwareSerial bluefruitSS = SoftwareSerial(BLUEFRUIT_SWUART_TXD_PIN,
                                            BLUEFRUIT_SWUART_RXD_PIN);

Adafruit_BluefruitLE_UART ble(bluefruitSS,
                              BLUEFRUIT_UART_MODE_PIN,
                              BLUEFRUIT_UART_CTS_PIN,
                              BLUEFRUIT_UART_RTS_PIN);

/** Prints an error and ends the execution. */
void error(const __FlashStringHelper* err) {
  Serial.println(err);
  while (1);
}

/** Waits until making a connection with the host. */
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

/** Setup method. */
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
  
  pinMode(enable_sensor_pin, OUTPUT);
}

/** Reads samples from the sensor.
 *  
 *  @param samples The array where to store the samples.
 */
void read_sensor(int samples[]) {
  Serial.println(F("-- Enabling sensor and waiting for warm-up --"));
  digitalWrite(enable_sensor_pin, HIGH);
  delay(WARMUP_TIME);
  
  for (int i = 0; i < NUM_SAMPLES; i++) {
    delay(DELAY_BETWEEN_SAMPLES);
    samples[i] = analogRead(sensor_pin);
    Serial.print(F("Sensor value: "));
    Serial.println(samples[i]);
  }
  Serial.println(F("-- Disabling sensor --"));
  digitalWrite(enable_sensor_pin, LOW);
}

/** Sends a packet containing a sample to the host.
 *  
 *  @param sample The sample to send.
 */
void send_packet(int sample) {
  Serial.print(F("Sending sample: "));
  Serial.println(sample);

  ble.print("AT+BLEUARTTX=");
  ble.print("B"); // packet begin
  ble.print(sample);
  ble.println("E"); // packet end
  if (!ble.waitForOK())
    Serial.println(F("Failed to send?"));
}

/** Loop method. */
void loop() {
  if (!ble.isConnected()) {
    Serial.println(F("Disconnected!"));
    connect_to_host();
  }

  int samples[NUM_SAMPLES];
  read_sensor(samples);

  for (int i = 0; i < NUM_SAMPLES; i++)
    send_packet(samples[i]);

  delay(TIME_BETWEEN_READINGS);
}

