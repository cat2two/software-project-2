#include <Servo.h> // sigmoid 함수  

#define PIN_SERVO 10
#define PIN_TRIG  12  
#define PIN_ECHO  13   

Servo myServo;

// ---- 설정값 ----
const float ANGLE_DOWN = 0.0;           // 차단기 내려간 각도
const float ANGLE_UP   = 90.0;          // 차단기 올라간 각도 (이동 폭 90도)
const unsigned long MOVING_TIME = 3000; // 이동 시간 3초
const unsigned long WAIT_TIME   = 1000; // 감지/해제 후 기다리는 시간 1초
const float DETECT_CM = 20.0;           // 이 거리 이하면 차량 감지
const float K = 8.0;                    // sigmoid 기울기 (작을수록 완만)

// 서보 펄스 폭 범위 (Servo 라이브러리 기본값). 실제 각도가 모자라면 넓혀서 조정
const int PULSE_MIN = 544;
const int PULSE_MAX = 2400;

// ---- 상태 ----
enum State { DOWN, WAIT_UP, MOVING_UP, UP, WAIT_DOWN, MOVING_DOWN };
State state = DOWN;

unsigned long stateStartTime;
unsigned long moveStartTime;
float startAngle = ANGLE_DOWN;
float stopAngle  = ANGLE_UP;

float readDistanceCm() {
  digitalWrite(PIN_TRIG, LOW);
  delayMicroseconds(2);
  digitalWrite(PIN_TRIG, HIGH);
  delayMicroseconds(10);
  digitalWrite(PIN_TRIG, LOW);
  long duration = pulseIn(PIN_ECHO, HIGH, 12000);  // 약 2m까지만 대기
  if (duration == 0) return 999.0;                 // 측정 실패 = 멀리 있음
  return duration * 0.034 / 2.0;
}

// 각도(float) -> 펄스 폭(us). 정수 각도보다 훨씬 촘촘하게 제어됨
void writeAngle(float angle) {
  int us = PULSE_MIN + (int)(angle * (PULSE_MAX - PULSE_MIN) / 180.0);
  myServo.writeMicroseconds(us);
}

// 0~1 -> 0~1, 양 끝이 정확히 0, 1이 되도록 정규화한 sigmoid
float ease(float t) {
  float s  = 1.0 / (1.0 + exp(-K * (t - 0.5)));
  float s0 = 1.0 / (1.0 + exp( K * 0.5));
  float s1 = 1.0 / (1.0 + exp(-K * 0.5));
  return (s - s0) / (s1 - s0);
}

void changeState(State next) {
  state = next;
  stateStartTime = millis();
}

void startMove(float from, float to, State movingState) {
  startAngle = from;
  stopAngle  = to;
  moveStartTime = millis();
  changeState(movingState);
}

// 이동 중 각도 갱신. 이동이 끝나면 true 반환
bool updateMove() {
  unsigned long progress = millis() - moveStartTime;
  if (progress <= MOVING_TIME) {
    float t = (float)progress / MOVING_TIME;
    writeAngle(startAngle + (stopAngle - startAngle) * ease(t));
    return false;
  }
  writeAngle(stopAngle);
  return true;
}

void setup() {
  pinMode(PIN_TRIG, OUTPUT);
  pinMode(PIN_ECHO, INPUT);
  myServo.attach(PIN_SERVO, PULSE_MIN, PULSE_MAX);
  writeAngle(ANGLE_DOWN);
  delay(500);
  stateStartTime = millis();
  Serial.begin(9600);
}

void loop() {
  // 이동 중에는 센서를 읽지 않고 서보만 빠르게 갱신 (끊김 방지)
  if (state == MOVING_UP) {
    if (updateMove()) changeState(UP);
    delay(10);
    return;
  }
  if (state == MOVING_DOWN) {
    if (updateMove()) changeState(DOWN);
    delay(10);
    return;
  }

  bool detected = (readDistanceCm() < DETECT_CM);
  unsigned long elapsed = millis() - stateStartTime;

  switch (state) {
    case DOWN:
      if (detected) changeState(WAIT_UP);
      break;

    case WAIT_UP:   // 감지된 채로 WAIT_TIME 유지되면 올림
      if (!detected) changeState(DOWN);
      else if (elapsed >= WAIT_TIME) {
        Serial.println("raise");
        startMove(ANGLE_DOWN, ANGLE_UP, MOVING_UP);
      }
      break;

    case UP:
      if (!detected) changeState(WAIT_DOWN);
      break;

    case WAIT_DOWN: // 차량이 없는 채로 WAIT_TIME 유지되면 내림
      if (detected) changeState(UP);
      else if (elapsed >= WAIT_TIME) {
        Serial.println("lower");
        startMove(ANGLE_UP, ANGLE_DOWN, MOVING_DOWN);
      }
      break;

    default:
      break;
  }

  delay(30);  // 센서 측정 간격
}






/* cosine 함수   
 #include <Servo.h>

#define PIN_SERVO 10
#define PIN_TRIG  12   
#define PIN_ECHO  13   

Servo myServo;

// ---- 설정값 ----
const float ANGLE_DOWN = 0.0;           // 차단기 내려간 각도
const float ANGLE_UP   = 90.0;          // 차단기 올라간 각도 (이동 폭 90도)
const unsigned long MOVING_TIME = 3000; // 이동 시간 3초
const unsigned long WAIT_TIME   = 1000; // 감지/해제 후 기다리는 시간 1초
const float DETECT_CM = 20.0;           // 이 거리 이하면 차량 감지

// 서보 펄스 폭 범위 (Servo 라이브러리 기본값). 실제 각도가 모자라면 넓혀서 조정
const int PULSE_MIN = 544;
const int PULSE_MAX = 2400;

// ---- 상태 ----
enum State { DOWN, WAIT_UP, MOVING_UP, UP, WAIT_DOWN, MOVING_DOWN };
State state = DOWN;

unsigned long stateStartTime;
unsigned long moveStartTime;
float startAngle = ANGLE_DOWN;
float stopAngle  = ANGLE_UP;

float readDistanceCm() {
  digitalWrite(PIN_TRIG, LOW);
  delayMicroseconds(2);
  digitalWrite(PIN_TRIG, HIGH);
  delayMicroseconds(10);
  digitalWrite(PIN_TRIG, LOW);
  long duration = pulseIn(PIN_ECHO, HIGH, 12000);  // 약 2m까지만 대기
  if (duration == 0) return 999.0;                 // 측정 실패 = 멀리 있음
  return duration * 0.034 / 2.0;
}

// 각도(float) -> 펄스 폭(us). 정수 각도보다 훨씬 촘촘하게 제어됨
void writeAngle(float angle) {
  int us = PULSE_MIN + (int)(angle * (PULSE_MAX - PULSE_MIN) / 180.0);
  myServo.writeMicroseconds(us);
}

// 0~1 -> 0~1, cosine easing: 시작/끝에서 기울기 0
float ease(float t) {
  return 0.5 - 0.5 * cos(PI * t);
}

void changeState(State next) {
  state = next;
  stateStartTime = millis();
}

void startMove(float from, float to, State movingState) {
  startAngle = from;
  stopAngle  = to;
  moveStartTime = millis();
  changeState(movingState);
}

// 이동 중 각도 갱신. 이동이 끝나면 true 반환
bool updateMove() {
  unsigned long progress = millis() - moveStartTime;
  if (progress <= MOVING_TIME) {
    float t = (float)progress / MOVING_TIME;
    writeAngle(startAngle + (stopAngle - startAngle) * ease(t));
    return false;
  }
  writeAngle(stopAngle);
  return true;
}

void setup() {
  pinMode(PIN_TRIG, OUTPUT);
  pinMode(PIN_ECHO, INPUT);
  myServo.attach(PIN_SERVO, PULSE_MIN, PULSE_MAX);
  writeAngle(ANGLE_DOWN);
  delay(500);
  stateStartTime = millis();
  Serial.begin(9600);
}

void loop() {
  // 이동 중에는 센서를 읽지 않고 서보만 빠르게 갱신 (끊김 방지)
  if (state == MOVING_UP) {
    if (updateMove()) changeState(UP);
    delay(10);
    return;
  }
  if (state == MOVING_DOWN) {
    if (updateMove()) changeState(DOWN);
    delay(10);
    return;
  }

  bool detected = (readDistanceCm() < DETECT_CM);
  unsigned long elapsed = millis() - stateStartTime;

  switch (state) {
    case DOWN:
      if (detected) changeState(WAIT_UP);
      break;

    case WAIT_UP:   // 감지된 채로 WAIT_TIME 유지되면 올림
      if (!detected) changeState(DOWN);
      else if (elapsed >= WAIT_TIME) {
        Serial.println("raise");
        startMove(ANGLE_DOWN, ANGLE_UP, MOVING_UP);
      }
      break;

    case UP:
      if (!detected) changeState(WAIT_DOWN);
      break;

    case WAIT_DOWN: // 차량이 없는 채로 WAIT_TIME 유지되면 내림
      if (detected) changeState(UP);
      else if (elapsed >= WAIT_TIME) {
        Serial.println("lower");
        startMove(ANGLE_UP, ANGLE_DOWN, MOVING_DOWN);
      }
      break;

    default:
      break;
  }

  delay(30);  // 센서 측정 간격
} */
