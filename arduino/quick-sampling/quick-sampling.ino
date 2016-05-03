#define WARMUP_TIME 10 * 1000UL
#define NUM_SAMPLES 30
#define DELAY_BETWEEN_SAMPLES 1000

const int sensor_pin = A0;
const int enable_sensor_pin = 7;

/** Setup method. */
void setup() {
  Serial.begin(115200);
  pinMode(enable_sensor_pin, OUTPUT);
}

/** Reads samples from the sensor. */
void read_samples() {
  unsigned long accum = 0;
  for (int i = 0; i < NUM_SAMPLES; i++) {
    auto sample = analogRead(sensor_pin);
    Serial.print(F("Sensor value: "));
    Serial.println(sample);
    accum += sample;
    delay(DELAY_BETWEEN_SAMPLES);
  }

  auto mean = static_cast<float>(accum) / NUM_SAMPLES;
  Serial.print(F("Mean: "));
  Serial.println(mean);
}

/** Loop method. */
void loop() {
  Serial.println(F("-- Enabling sensor and waiting for warm-up --"));
  digitalWrite(enable_sensor_pin, HIGH);
  delay(WARMUP_TIME);
  
  read_samples();  

  Serial.println(F("-- Disabling sensor --"));
  digitalWrite(enable_sensor_pin, LOW);

  Serial.println(F("Halting"));
  while (true);
}

