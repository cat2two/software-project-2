#include <Servo.h>

// Arduino pin assignment
#define PIN_LED   9   // LED active-low
#define PIN_TRIG  12  // sonar sensor TRIGGER
#define PIN_ECHO  13  // sonar sensor ECHO
#define PIN_SERVO 10  // servo motor

// configurable parameters for sonar
#define SND_VEL 346.0     // sound velocity at 24 celsius degree (unit: m/sec)
#define INTERVAL 25       // sampling interval (unit: msec)
#define PULSE_DURATION 10 // ultra-sound Pulse Duration (unit: usec)
#define _DIST_MIN 180.0   // minimum distance to be measured (unit: mm)
#define _DIST_MAX 360.0   // maximum distance to be measured (unit: mm)

#define TIMEOUT ((INTERVAL / 2) * 1000.0) // maximum echo waiting time (unit: usec)
#define SCALE (0.001 * 0.5 * SND_VEL)     // coefficient to convert duration to distance

#define _EMA_ALPHA 0.3    // EMA weight of new sample (range: 0 to 1)
                          // 1이면 EMA 필터 효과 없음. 실험으로 조정할 것!

// duty duration for myservo.writeMicroseconds()
// NEEDS TUNING (servo by servo)
#define _DUTY_MIN 500   // servo 0 degree
#define _DUTY_NEU 1500  // servo 90 degree
#define _DUTY_MAX 2500  // servo 180 degree

// global variables
float dist_ema, dist_prev;          // unit: mm
unsigned long last_sampling_time;   // unit: ms

Servo myservo;

void setup() {
  pinMode(PIN_LED, OUTPUT);
  pinMode(PIN_TRIG, OUTPUT);
  pinMode(PIN_ECHO, INPUT);
  digitalWrite(PIN_TRIG, LOW);
  digitalWrite(PIN_LED, HIGH);      // LED 꺼짐 (active-low)

  myservo.attach(PIN_SERVO);
  myservo.writeMicroseconds(_DUTY_NEU);

  // 초기값 설정
  dist_prev = _DIST_MIN;
  dist_ema = dist_prev;

  Serial.begin(57600);
}

void loop() {
  float dist_raw, dist_filtered;

  if (millis() < last_sampling_time + INTERVAL)
    return;

  // 거리 측정
  dist_raw = USS_measure(PIN_TRIG, PIN_ECHO);

  // 범위 필터
  if ((dist_raw == 0.0) || (dist_raw > _DIST_MAX) || (dist_raw < _DIST_MIN)) {
    dist_filtered = dist_prev;        // 범위 밖: 이전 값 유지
    digitalWrite(PIN_LED, HIGH);      // LED 꺼짐
  } else {
    dist_filtered = dist_raw;         // 범위 안
    dist_prev = dist_raw;
    digitalWrite(PIN_LED, LOW);       // LED 켜짐 (active-low)
  }

  // EMA 필터
  dist_ema = _EMA_ALPHA * dist_filtered + (1.0 - _EMA_ALPHA) * dist_ema;

  // 거리에 비례하여 서보 제어 (18cm → 0도, 36cm → 180도)
  float ratio = (dist_ema - _DIST_MIN) / (_DIST_MAX - _DIST_MIN);
  ratio = constrain(ratio, 0.0, 1.0);
  int duty = _DUTY_MIN + (int)(ratio * (_DUTY_MAX - _DUTY_MIN));
  myservo.writeMicroseconds(duty);

  // 시리얼 플로터 출력
  Serial.print("Min:");    Serial.print(_DIST_MIN);
  Serial.print(",dist:");  Serial.print(dist_raw);
  Serial.print(",ema:");   Serial.print(dist_ema);
  Serial.print(",Servo:"); Serial.print(myservo.read());
  Serial.print(",Max:");   Serial.print(_DIST_MAX);
  Serial.println("");

  last_sampling_time += INTERVAL;
}

// get a distance reading from USS. return value is in millimeter.
float USS_measure(int TRIG, int ECHO)
{
  digitalWrite(TRIG, HIGH);
  delayMicroseconds(PULSE_DURATION);
  digitalWrite(TRIG, LOW);

  return pulseIn(ECHO, HIGH, TIMEOUT) * SCALE; // unit: mm
}
