// Arduino pin assignment
#define PIN_LED  9
#define PIN_TRIG 12
#define PIN_ECHO 13

// configurable parameters
#define SND_VEL 346.0     // sound velocity at 24 celsius degree (unit: m/sec)
#define INTERVAL 25       // sampling interval (unit: msec)
#define PULSE_DURATION 10 // ultra-sound Pulse Duration (unit: usec)
#define _DIST_MIN 100     // minimum distance to be measured (unit: mm)
#define _DIST_MAX 300     // maximum distance to be measured (unit: mm)

#define TIMEOUT ((INTERVAL / 2) * 1000.0) // maximum echo waiting time (unit: usec)
#define SCALE (0.001 * 0.5 * SND_VEL)     // coefficent to convert duration to distance

#define _EMA_ALPHA 1.0    // EMA weight of new sample (1 = EMA disabled)

// median filter parameter
#define N 3              // number of samples kept (odd number recommended)

// global variables
unsigned long last_sampling_time;   // unit: msec
float dist_ema;                     // EMA distance

float samples[N];                   // recent N samples (circular buffer)
int sample_idx = 0;                 // next write position
int sample_count = 0;               // number of samples stored so far

void setup() {
  pinMode(PIN_LED, OUTPUT);
  pinMode(PIN_TRIG, OUTPUT);
  pinMode(PIN_ECHO, INPUT);
  digitalWrite(PIN_TRIG, LOW);

  Serial.begin(57600);
}

void loop() {
  float dist_raw, dist_median;

  // wait until next sampling time
  if (millis() < last_sampling_time + INTERVAL)
    return;

  // get a distance reading from the USS
  dist_raw = USS_measure(PIN_TRIG, PIN_ECHO);

  // store new sample in circular buffer
  samples[sample_idx] = dist_raw;
  sample_idx = (sample_idx + 1) % N;
  if (sample_count < N) sample_count++;

  // median of stored samples
  dist_median = median_filter();

  // EMA (alpha = 1.0 -> same as dist_median)
  if (sample_count == 1)
    dist_ema = dist_median;         // initialize with the first value
  else
    dist_ema = _EMA_ALPHA * dist_median + (1.0 - _EMA_ALPHA) * dist_ema;

  // output the read value to the serial port
  Serial.print("Min:");     Serial.print(_DIST_MIN);
  Serial.print(",raw:");    Serial.print(dist_raw);
  Serial.print(",ema:");    Serial.print(dist_ema);
  Serial.print(",median:"); Serial.print(dist_median);
  Serial.print(",Max:");    Serial.print(_DIST_MAX);
  Serial.println("");

  // LED control
  if ((dist_raw < _DIST_MIN) || (dist_raw > _DIST_MAX))
    digitalWrite(PIN_LED, 1);       // LED OFF
  else
    digitalWrite(PIN_LED, 0);       // LED ON

  // update last sampling time
  last_sampling_time += INTERVAL;
}

// return the median of the samples stored in the buffer
float median_filter()
{
  float sorted[N];
  int n = sample_count;

  for (int i = 0; i < n; i++)
    sorted[i] = samples[i];

  // insertion sort
  for (int i = 1; i < n; i++) {
    float key = sorted[i];
    int j = i - 1;
    while (j >= 0 && sorted[j] > key) {
      sorted[j + 1] = sorted[j];
      j--;
    }
    sorted[j + 1] = key;
  }

  if (n % 2 == 1)
    return sorted[n / 2];
  else
    return (sorted[n / 2 - 1] + sorted[n / 2]) / 2.0;
}

// get a distance reading from USS. return value is in millimeter.
float USS_measure(int TRIG, int ECHO)
{
  digitalWrite(TRIG, HIGH);
  delayMicroseconds(PULSE_DURATION);
  digitalWrite(TRIG, LOW);

  return pulseIn(ECHO, HIGH, TIMEOUT) * SCALE; // unit: mm
}
