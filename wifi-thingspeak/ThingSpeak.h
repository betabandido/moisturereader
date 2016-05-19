#ifndef THINGSPEAK_H
#define THINGSPEAK_H

#include <Client.h>
#include <TimeLib.h>

/** Class to upload data to a channel in ThinkSpeak. */
class ThingSpeak {
public:
  /** Constructor.
   *  
   *  @param client The network client to use to send the data
   *                (e.g., WiFiClient, EthernetClient).
   *  @param api_key API key for the channel.              
   */
  ThingSpeak(Client& client, const String& api_key);

  /** Sets the timestamp for the data.
   *  
   *  @param time The time when the data was recorded.
   */
  void set_time(time_t time);

  /** Sets a field's value.
   *  
   *  @param id Field's identifier.
   *  @param value Field's value.
   */
  void set_field(unsigned id, const String& value);

  /** Commits the data to ThinkSpeak.
   *  
   *  @return True if the operation was successful; false otherwise.
   */
  bool commit();

private:
  /** Client to communicate with ThinkSpeak. */
  Client& client_;

  /** Channel API key. */
  const String& api_key_;

  /** POST data to send. */
  String post_str_;
};

#endif // THINGSPEAK_H

