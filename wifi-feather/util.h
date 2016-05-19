#ifndef UTIL_H
#define UTIL_H

#include <Arduino.h>

template<typename CondFunc>
static bool wait_for_condition(
    CondFunc cond_func,
    unsigned time_out = 0,
    unsigned step_length = 100) {
  auto t0 = millis();
  while (!cond_func()) {
    if (time_out > 0 && (millis() - t0) > time_out)
      return false;  

    delay(step_length);
    Serial.print(".");
  }
  return true;
}

void fatal_error(String msg) {
  Serial.println(msg);
  delay(1000);
  ESP.restart();
}

#endif // UTIL_H

