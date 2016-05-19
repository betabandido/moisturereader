#include <ESP8266WiFi.h>
#include <ntpclient.h>
#include <PrintEx.h>
#include <TimeLib.h>
#include <WiFiUdp.h>

#include "persistent_vector.h"
#include "ssid-info.h"
#include "util.h"

PrintEx serial = Serial;

static constexpr unsigned CONNECTION_TIMEOUT = 5000;
static constexpr char const* HOST = "192.168.1.2";
static constexpr unsigned PORT = 20001;

static constexpr size_t MAX_PENDING_MESSAGES = 4;
static constexpr size_t SAMPLES_PER_MESSAGE = 8;

// Sleep time: 30s (measured in us)
static constexpr unsigned SLEEP_TIME = 30 * 1000 * 1000;

struct message {
  time_t time;
  unsigned short samples[SAMPLES_PER_MESSAGE];
};

typedef persistent_vector<message> pending_messages_queue;
static pending_messages_queue* pending_messages;

static WiFiUDP Udp;
static constexpr unsigned short local_port = 8888;

static ntp_client ntp(Udp);

void setup() {
  Serial.begin(115200);
  delay(100);

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
  message msg;
  msg.time = now();
  for (unsigned short i = 0; i < SAMPLES_PER_MESSAGE; i++)
    msg.samples[i] = i;

  pending_messages->push_back(msg);

  send_messages();

  Serial.println("Storing data to EEPROM");
  EEPROM.commit();

  ESP.deepSleep(SLEEP_TIME);
  Serial.println(">>> This should not print");
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

static WiFiClient connect_client() {
  WiFiClient client;
  if (!client.connect(HOST, PORT))
    fatal_error("Connection to host failed");
  return client;
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

  // XXX Add pop_front() ???
  // XXX Implement a circular queue ???
  while (!pending_messages->empty()) {
    auto last_idx = pending_messages->size() - 1;
    auto& msg = (*pending_messages)[last_idx];
    serial.printf("About to send msg %d (idx: %d)\n", msg.time, last_idx);
    // TODO use error code
    send_message(msg);
    pending_messages->pop_back();
  } 
}

static void send_message(const message& msg) {
  // TODO use the same connection for all messages
  auto client = connect_client();
  client.print("sensor-id: ");
  serial.printf("msg.time=%d\n", msg.time);
  client.println(msg.time);

//      serial.printf("time: %d\n", msg.time);
//    for (size_t s = 0; s < SAMPLES_PER_MESSAGE; s++)
//      serial.printf("sample%d: %d\n", s, msg.samples[s]);
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

