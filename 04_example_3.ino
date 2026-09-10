#define PIN_LED 13 //별명을 지어줌 
unsigned int count, toggle; // 부호 없이 0 부터 출발할게

void setup() {
  // put your setup code here, to run once:
  pinMode(PIN_LED, OUTPUT);
  Serial.begin(115200);
  while(!Serial){
    }
  Serial.println("hello world!");
  count = toggle = 0;
  digitalWrite(PIN_LED, toggle);
}

void loop() {
  // put your main code here, to run repeatedly:
  Serial.println(++count); // ++는 먼저 올리고 나서 프린트 하겠다는 뜻
  toggle = toggle_state(toggle);
  digitalWrite(PIN_LED, toggle);
  delay(1000);
}

int toggle_state(int toggle) {
  return !toggle;
}
