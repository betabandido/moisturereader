#include "ThingSpeak.h"

static constexpr char const* HOST = "api.thingspeak.com";

ThingSpeak::ThingSpeak(Client& client, const String& api_key)
  : client_(client)
  , api_key_(api_key)
  , post_str_(api_key)
{}

void ThingSpeak::set_time(time_t t) {
  // XXX do we need to use two digits for hours/minutes/seconds ?
  post_str_ += "&created_at="
      + String(year(t)) + "-" + String(month(t)) + "-" + String(day(t))
      + "T" + String(hour(t)) + ":" + String(minute(t)) + ":" + String(second(t)) + "Z";
}

void ThingSpeak::set_field(unsigned id, const String& value) {
  post_str_ += "&field" + String(id) + "=" + value;
}
  
bool ThingSpeak::commit() {
  post_str_ += "\r\n\r\n";

  if (!client_.connect(HOST, 80))
    return false;

  client_.print("POST /update HTTP/1.1\n"); 
  client_.print("Host: api.thingspeak.com\n"); 
  client_.print("Connection: close\n"); 
  client_.print("X-THINGSPEAKAPIKEY: " + api_key_ + "\n"); 
  client_.print("Content-Type: application/x-www-form-urlencoded\n"); 
  client_.print("Content-Length: "); 
  client_.print(post_str_.length()); 
  client_.print("\n\n"); 
  client_.print(post_str_);
  client_.stop();

  return true;
}

