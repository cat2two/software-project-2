// ============================================================
//  적외선 거리센서 자동 Curve Fitting 프로그램
//  - 시리얼 모니터: 보드레이트 1000000, 줄 끝 "새 줄(NL)" 또는 "Both NL & CR"
//  - 순서: (1) fitting 방식/차수 입력 -> (2) 탐구공 위치 0,5,...,30cm 마다 엔터
//          -> (3) 측정 완료 후 fitting -> (4) 바로 쓸 수 있는 함수 코드 출력
// ============================================================

#define PIN_IR A0

// ---- 측정 설정 ----
#define N_SAMPLES   10      // 한 위치에서 반복 측정 횟수
#define FILTER_POS  0.5     // 0.5 = 중앙값
#define POS_START   0       // cm
#define POS_END     30      // cm
#define POS_STEP    5       // cm
#define N_POINTS    ((POS_END - POS_START) / POS_STEP + 1)   // 7개

#define MAX_DEG     5       // 다항식 최대 차수

// ---- fitting 방식 ----
#define M_POLY   1          // d = c0 + c1*v + c2*v^2 + ...
#define M_POWER  2          // d = A * v^B
#define M_EXP    3          // d = A * exp(B*v)
#define M_INV    4          // d = A + B / v

int   model, degree;
float volt[N_POINTS];       // 필터링된 센서 값 (0~1023)
float dist[N_POINTS];       // 탐구공 위치 (cm)

// fitting 결과
float coef[MAX_DEG + 1];    // 다항식 계수 (정규화된 변수 x 기준)
float xm, xs;               // 정규화: x = (v - xm) / xs
float pA, pB;               // 비선형 모델 계수

// ---- 함수 선언 ----
bool  do_fit();
void  skip_msg(int i);
bool  polyfit(const float *x, const float *y, int n, int deg, float *c, float *pxm, float *pxs);
float predict(float v);
void  print_result();
void  print_quality();
void  print_sci(float v);
int   readLine(char *buf, int len);
int   readInt(const __FlashStringHelper *prompt, int lo, int hi);
unsigned int ir_sensor_filtered(unsigned int n, float position);

// ------------------------------------------------------------
void setup()
{
  Serial.begin(1000000);
}

void loop()
{
  Serial.println(F("\n===== IR sensor curve fitting ====="));

  // ---------- 1. 설정 입력 ----------
  Serial.println(F("Fitting model: 1) Polynomial  2) Power A*v^B  3) Exponential A*exp(B*v)  4) Inverse A+B/v"));
  model = readInt(F("Select model (1-4): "), 1, 4);

  degree = 1;
  if (model == M_POLY) {
    int maxd = (N_POINTS - 1 < MAX_DEG) ? N_POINTS - 1 : MAX_DEG;
    degree = readInt(F("Polynomial degree (1-5): "), 1, maxd);
    if (degree >= 5)
      Serial.println(F("(Warning: degree 5 may be numerically unstable with float)"));
  }

  // ---------- 2. 데이터 수집 ----------
  char buf[8];
  for (int i = 0; i < N_POINTS; i++) {
    int pos = POS_START + i * POS_STEP;
    Serial.print(F("Place the target at "));
    Serial.print(pos);
    Serial.println(F(" cm, then press Enter..."));

    readLine(buf, sizeof(buf));                         // 엔터 대기
    unsigned int v = ir_sensor_filtered(N_SAMPLES, FILTER_POS);

    dist[i] = pos;
    volt[i] = v;
    Serial.print(F("  -> "));  Serial.print(pos);
    Serial.print(F(" cm : ")); Serial.println(v);
  }

  // ---------- 3. Curve fitting ----------
  Serial.println(F("\nFitting..."));
  if (!do_fit()) {
    Serial.println(F("Fitting failed (check data / model choice). Try again."));
    return;
  }

  // ---------- 4. 결과 출력 ----------
  print_result();
  print_quality();
  Serial.println(F("\n(Restarting...)"));
}

// ============================================================
//  Fitting 루틴
// ============================================================
bool do_fit()
{
  float X[N_POINTS], Y[N_POINTS];
  int n = 0;

  for (int i = 0; i < N_POINTS; i++) {
    float v = volt[i], d = dist[i];
    switch (model) {
      case M_POLY:  X[n] = v;        Y[n] = d;      break;
      case M_POWER: if (v <= 0 || d <= 0) { skip_msg(i); continue; }
                    X[n] = log(v);   Y[n] = log(d);  break;
      case M_EXP:   if (d <= 0)           { skip_msg(i); continue; }
                    X[n] = v;        Y[n] = log(d);  break;
      case M_INV:   if (v <= 0)           { skip_msg(i); continue; }
                    X[n] = 1.0 / v;  Y[n] = d;       break;
    }
    n++;
  }
  if (n < degree + 1) return false;

  if (!polyfit(X, Y, n, degree, coef, &xm, &xs)) return false;

  if (model != M_POLY) {
    // 1차 fitting 결과를 y = intercept + slope * X 로 환산
    float slope     = coef[1] / xs;
    float intercept = coef[0] - slope * xm;
    switch (model) {
      case M_POWER: pA = exp(intercept); pB = slope; break;
      case M_EXP:   pA = exp(intercept); pB = slope; break;
      case M_INV:   pA = intercept;      pB = slope; break;
    }
  }
  return true;
}

void skip_msg(int i)
{
  Serial.print(F("  (point "));
  Serial.print(dist[i], 0);
  Serial.println(F(" cm excluded: not usable for this model)"));
}

// 최소제곱 다항식 fitting (정규방정식 + 가우스 소거법)
// x를 [-1, 1]로 정규화하여 수치 안정성 확보
bool polyfit(const float *x, const float *y, int n, int deg,
             float *c, float *pxm, float *pxs)
{
  float mn = x[0], mx = x[0];
  for (int i = 1; i < n; i++) {
    if (x[i] < mn) mn = x[i];
    if (x[i] > mx) mx = x[i];
  }
  float m_ = (mn + mx) / 2.0;
  float s_ = (mx - mn) / 2.0;
  if (s_ < 1e-9) return false;
  *pxm = m_;  *pxs = s_;

  int m = deg + 1;
  float S[2 * MAX_DEG + 1], T[MAX_DEG + 1];
  for (int k = 0; k <= 2 * deg; k++) S[k] = 0;
  for (int k = 0; k <= deg; k++)     T[k] = 0;

  for (int i = 0; i < n; i++) {
    float t = (x[i] - m_) / s_;
    float p = 1.0;
    for (int k = 0; k <= 2 * deg; k++) {
      S[k] += p;
      if (k <= deg) T[k] += p * y[i];
      p *= t;
    }
  }

  float A[MAX_DEG + 1][MAX_DEG + 2];
  for (int i = 0; i < m; i++) {
    for (int j = 0; j < m; j++) A[i][j] = S[i + j];
    A[i][m] = T[i];
  }

  // 가우스 소거 (부분 피벗)
  for (int col = 0; col < m; col++) {
    int piv = col;
    for (int r = col + 1; r < m; r++)
      if (fabs(A[r][col]) > fabs(A[piv][col])) piv = r;
    if (fabs(A[piv][col]) < 1e-12) return false;
    if (piv != col)
      for (int j = 0; j <= m; j++) { float tmp = A[col][j]; A[col][j] = A[piv][j]; A[piv][j] = tmp; }
    for (int r = col + 1; r < m; r++) {
      float f = A[r][col] / A[col][col];
      for (int j = col; j <= m; j++) A[r][j] -= f * A[col][j];
    }
  }
  for (int i = m - 1; i >= 0; i--) {
    float sum = A[i][m];
    for (int j = i + 1; j < m; j++) sum -= A[i][j] * c[j];
    c[i] = sum / A[i][i];
  }
  return true;
}

// 현재 모델로 거리 예측
float predict(float v)
{
  switch (model) {
    case M_POLY: {
      float x = (v - xm) / xs, r = 0;
      for (int i = degree; i >= 0; i--) r = r * x + coef[i];
      return r;
    }
    case M_POWER: return pA * pow(v, pB);
    case M_EXP:   return pA * exp(pB * v);
    case M_INV:   return pA + pB / v;
  }
  return 0;
}

// ============================================================
//  출력
// ============================================================
void print_result()
{
  Serial.println(F("\n// ===== Copy the function below into your sketch ====="));
  Serial.println(F("float volt_to_distance(unsigned int a_value)"));
  Serial.println(F("{"));

  switch (model) {
    case M_POLY:
      Serial.print(F("  float x = (a_value - "));  print_sci(xm);
      Serial.print(F(") / "));                     print_sci(xs);
      Serial.println(F(";"));
      Serial.print(F("  return "));
      for (int i = 0; i <= degree; i++) {
        print_sci(coef[i]);
        if (i < degree) Serial.print(F(" + x*("));
      }
      for (int i = 0; i < degree; i++) Serial.print(')');
      Serial.println(';');
      break;

    case M_POWER:
      Serial.print(F("  return ")); print_sci(pA);
      Serial.print(F(" * pow(a_value, ")); print_sci(pB); Serial.println(F(");"));
      break;

    case M_EXP:
      Serial.print(F("  return ")); print_sci(pA);
      Serial.print(F(" * exp(")); print_sci(pB); Serial.println(F(" * a_value);"));
      break;

    case M_INV:
      Serial.print(F("  return ")); print_sci(pA);
      Serial.print(F(" + ("));      print_sci(pB); Serial.println(F(") / a_value;"));
      break;
  }
  Serial.println(F("}"));
}

void print_quality()
{
  float mean = 0, ssr = 0, sst = 0;
  for (int i = 0; i < N_POINTS; i++) mean += dist[i];
  mean /= N_POINTS;

  Serial.println(F("\n// cm   sensor   fitted   error"));
  for (int i = 0; i < N_POINTS; i++) {
    float f = predict(volt[i]);
    ssr += (dist[i] - f) * (dist[i] - f);
    sst += (dist[i] - mean) * (dist[i] - mean);
    Serial.print(F("// "));  Serial.print(dist[i], 0);
    Serial.print(F("    "));  Serial.print(volt[i], 0);
    Serial.print(F("    "));  Serial.print(f, 2);
    Serial.print(F("    "));  Serial.println(f - dist[i], 2);
  }
  Serial.print(F("// RMSE = ")); Serial.print(sqrt(ssr / N_POINTS), 3);
  Serial.print(F(" cm,  R^2 = ")); Serial.println(sst > 0 ? 1.0 - ssr / sst : 0, 5);
}

// 과학적 표기법(예: 1.720000E-3)으로 출력 -> 그대로 C 코드로 사용 가능
void print_sci(float v)
{
  if (v == 0) { Serial.print(F("0.0")); return; }
  if (v < 0)  { Serial.print('-'); v = -v; }
  int e = (int)floor(log10(v));
  float m = v / pow(10.0, e);
  if (m >= 9.9999995) { m /= 10.0; e++; }      // 반올림 보정
  if (m < 1.0)        { m *= 10.0; e--; }
  Serial.print(m, 6);
  Serial.print('E');
  Serial.print(e);
}

// ============================================================
//  시리얼 입력 도우미
// ============================================================
int readLine(char *buf, int len)
{
  int n = 0;
  while (true) {
    while (Serial.available() == 0) ;
    char c = Serial.read();
    if (c == '\n' || c == '\r') break;
    if (n < len - 1) buf[n++] = c;
  }
  delay(30);                                    // CR+LF 둘 다 오는 경우 정리
  while (Serial.available() > 0) {
    char c = Serial.peek();
    if (c == '\n' || c == '\r') Serial.read(); else break;
  }
  buf[n] = 0;
  return n;
}

int readInt(const __FlashStringHelper *prompt, int lo, int hi)
{
  char buf[12];
  while (true) {
    Serial.print(prompt);
    readLine(buf, sizeof(buf));
    int v = atoi(buf);
    Serial.println(v);
    if (v >= lo && v <= hi) return v;
    Serial.print(F("  Invalid. Enter a value between "));
    Serial.print(lo); Serial.print(F(" and ")); Serial.println(hi);
  }
}

// ============================================================
//  센서 측정 (제공된 예제의 필터 함수)
// ============================================================
int compare(const void *a, const void *b)
{
  return (*(unsigned int *)a - *(unsigned int *)b);
}

unsigned int ir_sensor_filtered(unsigned int n, float position)
{
  // n번 측정 후 정렬하여 position 백분위 값을 반환 (0.5 = 중앙값)
  if ((n == 0) || (n > 100) || (position < 0.0) || (position > 1))
    return 0;
  if (position == 1.0)
    position = 0.999;

  unsigned int *ir_val = (unsigned int *)malloc(sizeof(unsigned int) * n);
  if (ir_val == NULL)
    return 0;

  for (unsigned int i = 0; i < n; i++)
    ir_val[i] = analogRead(PIN_IR);

  qsort(ir_val, n, sizeof(unsigned int), compare);
  unsigned int ret_val = ir_val[(unsigned int)(n * position)];
  free(ir_val);
  return ret_val;
}
