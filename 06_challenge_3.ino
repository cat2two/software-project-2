const int LED_PIN = 7;

unsigned long period = 100;  // PWM 주기 (μs)

// PWM 주기 설정 함수
void set_period(int p) {
  period = p;
}

// PWM 한 주기를 실행하는 함수
void set_duty(int duty) {
  if (duty <= 0) {
    digitalWrite(LED_PIN, HIGH); // LED 완전히 OFF
    delayMicroseconds(period);
    return;
  }

  if (duty >= 100) {
    digitalWrite(LED_PIN, LOW);  // LED 완전히 ON
    delayMicroseconds(period);
    return;
  }

  unsigned long onTime = period * duty / 100;
  unsigned long offTime = period - onTime;

  digitalWrite(LED_PIN, LOW);
  delayMicroseconds(onTime);

  digitalWrite(LED_PIN, HIGH);
  delayMicroseconds(offTime);
}

void triangle_wave() {
  // 목표: 밝아지기(101단계) + 어두워지기(100단계) = 201단계를 1초에 맞추기
  // 한 단계당 목표 시간
  double targetStepTime = 1000000.0 / 201.0;  // 약 4975us

  // 이 period로 목표 단계 시간을 채우려면 set_duty()를 몇 번 반복해야 하는지 계산
  int cyclesPerLevel = round(targetStepTime / period);

  if (cyclesPerLevel >= 1) {
    // period가 충분히 작음 -> 101단계(duty 1씩 증가) 그대로 사용, 반복 횟수로 시간 조절
    for (int duty = 0; duty <= 100; duty++) {
        //Serial.println(duty);  //duty값 시리얼 모니터로 확인하려고 넣음
      for (int c = 0; c < cyclesPerLevel; c++) {
        set_duty(duty);
      }
    }
    for (int duty = 99; duty >= 0; duty--) {
      //Serial.println(duty);  //duty값 시리얼 모니터로 확인하려고 넣음  
      for (int c = 0; c < cyclesPerLevel; c++) {
        set_duty(duty);
      }
    }
  } else {
    // period가 너무 커서 반복 횟수를 1보다 줄일 수 없음
    // -> 대신 duty 증가폭을 키워서 전체 단계 수를 줄임
    unsigned long totalCycles = round(1000000.0 / period);  // 1초 동안 가능한 총 주기 수
    unsigned long halfCycles = totalCycles / 2;              // 밝아지기/어두워지기 각각의 단계 수
    if (halfCycles < 1) halfCycles = 1;

    double dutyStep = 100.0 / halfCycles;  // duty 증가폭 (1보다 커짐)

    // 밝아지기: 0 -> 100
    for (unsigned long i = 0; i <= halfCycles; i++) {
      int duty = round(i * dutyStep);
      if (duty > 100) duty = 100;
      //Serial.println(duty); //duty값 시리얼 모니터로 확인하려고 넣음   
      set_duty(duty);
    }

    // 어두워지기: 100 -> 0
    for (unsigned long i = halfCycles; i > 0; i--) {
      int duty = round((i - 1) * dutyStep);
     // Serial.println(duty); //duty값 시리얼 모니터로 확인하려고 넣음  
      set_duty(duty);
    }
  }
}

void setup() {
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, HIGH);  // LED OFF

  //Serial.begin(9600);  //duty값 시리얼 모니터로 확인하려고 넣음  
}

void loop() {
  triangle_wave();
}

/* 1초를 목표로 하지 않고 duty값을 1씩 부드럽게 올렸을 때 코드(10ms일때 2초 걸림)
const int LED_PIN = 7;

unsigned long period = 10000;  // PWM 주기 (μs)

// PWM 주기 설정 함수
void set_period(int p) {
  period = p;
}

// PWM 한 주기를 실행하는 함수
void set_duty(int duty) {
  if (duty <= 0) {
    digitalWrite(LED_PIN, HIGH);// LED 완전히 OFF
    delayMicroseconds(period);
    return;
  }

  if (duty >= 100) {
    digitalWrite(LED_PIN, LOW);   // LED 완전히 ON
    delayMicroseconds(period);
    return;
  }

  unsigned long onTime = period * duty / 100;
  unsigned long offTime = period - onTime;

  digitalWrite(LED_PIN, LOW);
  delayMicroseconds(onTime);

  digitalWrite(LED_PIN, HIGH);
  delayMicroseconds(offTime);
}
void triangle_wave() {
  const unsigned long stepTime = 5000;

  // 밝아지기
  for (int duty = 0; duty <= 100; duty++) {
    unsigned long startTime = micros();
   
    while (micros() - startTime < stepTime) {
      set_duty(duty);
    }
  }

  // 어두워지기
  for (int duty = 99; duty >= 0; duty--) {
    unsigned long startTime = micros();
 
    while (micros() - startTime < stepTime) {
      set_duty(duty);
    }
  }

}
void setup() {
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, HIGH);  // LED OFF


}


void loop() {
  triangle_wave();
 
}
const int LED_PIN = 7;

unsigned long period = 10000;  // PWM 주기 (μs)

// PWM 주기 설정 함수
void set_period(int p) {
  period = p;
}

// PWM 한 주기를 실행하는 함수
void set_duty(int duty) {
  if (duty <= 0) {
    digitalWrite(LED_PIN, HIGH);// LED 완전히 OFF
    delayMicroseconds(period);
    return;
  }

  if (duty >= 100) {
    digitalWrite(LED_PIN, LOW);   // LED 완전히 ON
    delayMicroseconds(period);
    return;
  }

  unsigned long onTime = period * duty / 100;
  unsigned long offTime = period - onTime;

  digitalWrite(LED_PIN, LOW);
  delayMicroseconds(onTime);

  digitalWrite(LED_PIN, HIGH);
  delayMicroseconds(offTime);
}
void triangle_wave() {
  const unsigned long stepTime = 5000;

  // 밝아지기
  for (int duty = 0; duty <= 100; duty++) {
    unsigned long startTime = micros();
   
    while (micros() - startTime < stepTime) {
      set_duty(duty);
    }
  }

  // 어두워지기
  for (int duty = 99; duty >= 0; duty--) {
    unsigned long startTime = micros();
 
    while (micros() - startTime < stepTime) {
      set_duty(duty);
    }
  }

}
void setup() {
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, HIGH);  // LED OFF


}


void loop() {
  triangle_wave();
 
} */
