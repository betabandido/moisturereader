#include <ESP8266WiFi.h>
#include <ntpclient.h>
#include <PrintEx.h>
#include <Statistic.h>
#include <TimeLib.h>
#include <WiFiUdp.h>

#include "api-key.h"
#include "persistent_queue.h"
#include "ssid-info.h"
#include "ThingSpeak.h"
#include "util.h"

static constexpr unsigned ENABLE_SENSOR_PIN = 14;

static constexpr unsigned CONNECTION_TIMEOUT = 5000;

static constexpr size_t MAX_PENDING_MESSAGES = 4;
static constexpr size_t SAMPLES_PER_MESSAGE = 8;

static constexpr unsigned SENSOR_WARMUP_DELAY = 1000;
static constexpr unsigned DELAY_BETWEEN_SAMPLES = 500;

// Sleep time: 30s (measured in us)
static constexpr unsigned SLEEP_TIME = 30 * 1000 * 1000;

static constexpr unsigned short local_port = 8888;

struct message {
  time_t time;
  float sample_mean;
  float sample_stdev;
};

typedef persistent_queue<message> pending_messages_queue;

static pending_messages_queue* pending_messages;

static WiFiUDP Udp;
// TODO make ntp_client a template capable of handling both WiFiUDP and EthernetUDP
static ntp_client ntp(Udp);

static PrintEx serial = Serial;

void setup() {
  Serial.begin(115200);
  delay(100);

  Serial.println("\nStarting");

  pinMode(ENABLE_SENSOR_PIN, OUTPUT);
  digitalWrite(ENABLE_SENSOR_PIN, LOW);

  EEPROM.begin(pending_messages_queue::storage_size(MAX_PENDING_MESSAGES));
//  delete_messages();
  pending_messages = new pending_messages_queue(0, MAX_PENDING_MESSAGES);

  connect_wifi();

  Serial.println("Syncing with NTP clock");
  Udp.begin(local_port);
  setSyncProvider([]() { return ntp.get_time(); });
  bool success = wait_for_condition(
    []() { return timeStatus() != timeNotSet; },
    CONNECTION_TIMEOUT);
  if (!success)
    fatal_error("Error connecting to NTP server; restarting");
    
  print_date();
}

void loop() {
  read_sensor();
  send_messages();

  Serial.println("Storing data to EEPROM");
  EEPROM.commit();

  Serial.println("Going to sleep");
  ESP.deepSleep(SLEEP_TIME);
  Serial.println(">>> This should not print");
}

void read_sensor() {
  digitalWrite(ENABLE_SENSOR_PIN, HIGH);
  delay(SENSOR_WARMUP_DELAY);
  
  message msg;
  msg.time = now();

  Statistic stat;
  for (unsigned short i = 0; i < SAMPLES_PER_MESSAGE; i++) {
    delay(DELAY_BETWEEN_SAMPLES);
    stat.add(analogRead(A0));
  }

  digitalWrite(ENABLE_SENSOR_PIN, LOW);

  msg.sample_mean = stat.average();
  msg.sample_stdev = stat.pop_stdev();

  pending_messages->push(msg);
}

static void connect_wifi() {  
  serial.printf("Connecting to: %s\n", ssid);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  bool success = wait_for_condition(
      []() { return WiFi.status() == WL_CONNECTED; },
      CONNECTION_TIMEOUT);
  if (!success)
    fatal_error("Error connecting to AP; restarting");
    
  serial.print(" connected\n");
  serial.printf("Address: %s\n", WiFi.localIP().toString().c_str());
}

static void delete_messages() {
  auto size = pending_messages_queue::storage_size(MAX_PENDING_MESSAGES);
  for (size_t i = 0; i < size; i++)
    EEPROM.write(i, 0);
  EEPROM.commit();
  Serial.println("Data was deleted");
  while (true)
    delay(100);
}

static void send_messages() {
  serial.printf("Attempting to send %d messages\n",
      pending_messages->size());

  while (!pending_messages->empty()) {
    auto& msg = pending_messages->front();
    if (!send_message(msg)) {
      Serial.println("Error sending message");
      break;
    }
    pending_messages->pop();
  }
}

static bool send_message(const message& msg) {
  WiFiClient client;

  ThingSpeak ts(client, API_KEY);
  ts.set_time(msg.time);
  ts.set_field(1, String(msg.sample_mean));
  ts.set_field(2, String(msg.sample_stdev));

  return ts.commit();
}

static void print_date() {
  serial.printf("%d:%02d:%02d %02d-%02d-%04d\n",
      hour(),
      minute(),
      second(),
      day(),
      month(),
      year());
}

