#include <Servo.h>

// Arduino pin assignment
#define PIN_IR    A0
#define PIN_LED   9
#define PIN_SERVO 10

// configurable parameters for distance (unit: mm)
#define _DIST_MIN 100.0   // 10 cm
#define _DIST_MAX 250.0   // 25 cm

// EMA filter
#define _EMA_ALPHA 0.2    // 0 ~ 1, 작을수록 부드럽지만 반응이 느림

// configurable parameters for Servo
#define _DUTY_MIN 500     // servo 0 degree
#define _DUTY_NEU 1500    // servo 90 degree
#define _DUTY_MAX 2400    // servo 180 degree

// Loop Interval
#define INTERVAL 25       // sampling interval (unit: msec), 20 이상

// global variables
unsigned long last_sampling_time;  // unit: msec

Servo myservo;

float dist_raw;     // range filter를 통과한 거리값
float dist_prev;    // 마지막으로 범위 안에 들어왔던 유효 거리값
float dist_ema;     // EMA 필터 거리값
float duty;         // 서보 duty (usec)

void setup() {
  pinMode(PIN_LED, OUTPUT);
  digitalWrite(PIN_LED, LOW);

  myservo.attach(PIN_SERVO);

  // 초기값 설정
  dist_prev = _DIST_MIN;
  dist_ema = _DIST_MIN;
  duty = _DUTY_MIN;
  myservo.writeMicroseconds(duty);

  Serial.begin(1000000);  // baud rate: 최대

  last_sampling_time = 0;
}

void loop() {
  int a_value;
  float dist_meas;

  // wait until next sampling time.
  if (millis() < (last_sampling_time + INTERVAL))
    return;

  // 1) IR 센서 읽기 및 거리 변환 (mm)
  a_value = analogRead(PIN_IR);
  dist_meas = ((6762.0 / (a_value - 9) - 4.0) * 10.0) - 60.0;

  // 2) 범위 필터: 10cm ~ 25cm 밖이면 이전 유효값 유지, LED 끔
  if ((dist_meas < _DIST_MIN) || (dist_meas > _DIST_MAX)) {
    dist_raw = dist_prev;
    digitalWrite(PIN_LED, LOW);
  } else {
    dist_raw = dist_meas;
    dist_prev = dist_meas;
    digitalWrite(PIN_LED, HIGH);
  }

  // 3) EMA 필터
  dist_ema = _EMA_ALPHA * dist_raw + (1.0 - _EMA_ALPHA) * dist_ema;

  // 4) 거리 -> 서보 duty 변환 (map() 사용 안 함)
  //    10cm -> _DUTY_MIN(0도), 25cm -> _DUTY_MAX(180도)
  duty = _DUTY_MIN +
         (dist_ema - _DIST_MIN) * (float)(_DUTY_MAX - _DUTY_MIN) / (_DIST_MAX - _DIST_MIN);

  // 안전을 위해 duty 범위 제한
  if (duty < _DUTY_MIN) duty = _DUTY_MIN;
  if (duty > _DUTY_MAX) duty = _DUTY_MAX;

  myservo.writeMicroseconds(duty);

  // 5) 시리얼 플로터 출력
  Serial.print("_DUTY_MIN:"); Serial.print(_DUTY_MIN);
  Serial.print(",_DIST_MIN:"); Serial.print(_DIST_MIN);
  Serial.print(",IR:");        Serial.print(a_value);
  Serial.print(",dist_raw:");  Serial.print(dist_raw);
  Serial.print(",ema:");       Serial.print(dist_ema);
  Serial.print(",servo:");     Serial.print(duty);
  Serial.print(",_DIST_MAX:"); Serial.print(_DIST_MAX);
  Serial.print(",_DUTY_MAX:"); Serial.print(_DUTY_MAX);
  Serial.println("");

  // update last sampling time
  last_sampling_time += INTERVAL;
}
