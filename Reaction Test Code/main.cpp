// Reaction Speed Tester — ESP32, single 7-segment digit (direct GPIO), 5 chase LEDs, button
// No shift register, no buzzer. Digit shown one place value at a time (e.g. 0.43s -> "0" "4" "3")

#include <Arduino.h>

// Segment pins
#define SEG_A 14
#define SEG_B 12
#define SEG_C 18
#define SEG_D 19
#define SEG_E 21
#define SEG_F 22
#define SEG_G 25
#define SEG_DP 26

// LED pins (chase)
const int ledPins[5] = {27, 32, 33, 13, 5};

#define BTN_PIN 4

const int segPins[8] = {SEG_A, SEG_B, SEG_C, SEG_D, SEG_E, SEG_F, SEG_G, SEG_DP};

// segment patterns for digits 0-9: order = a,b,c,d,e,f,g,dp
const bool digitPatterns[10][8] = {
  {1,1,1,1,1,1,0,0}, // 0
  {0,1,1,0,0,0,0,0}, // 1
  {1,1,0,1,1,0,1,0}, // 2
  {1,1,1,1,0,0,1,0}, // 3
  {0,1,1,0,0,1,1,0}, // 4
  {1,0,1,1,0,1,1,0}, // 5
  {1,0,1,1,1,1,1,0}, // 6
  {1,1,1,0,0,0,0,0}, // 7
  {1,1,1,1,1,1,1,0}, // 8
  {1,1,1,1,0,1,1,0}  // 9
};

enum State { IDLE, WAITING, GO, RESULT };
State state = IDLE;

unsigned long stateStart = 0;
unsigned long goTime = 0;
unsigned long randomDelay = 0;
int reactionMs = 0;

int resultDigits[3];
int digitIndex = 0;
unsigned long digitShownAt = 0;
const int DIGIT_HOLD_MS = 600;

void clearSegments() {
  for (int i = 0; i < 8; i++) digitalWrite(segPins[i], LOW);
}

void showDigit(int d) {
  if (d < 0 || d > 9) { clearSegments(); return; }
  for (int i = 0; i < 8; i++) {
    digitalWrite(segPins[i], digitPatterns[d][i] ? HIGH : LOW);
  }
}

void clearLEDs() {
  for (int i = 0; i < 5; i++) digitalWrite(ledPins[i], LOW);
}

void chaseAnimation(unsigned long elapsed) {
  clearLEDs();
  int idx = (elapsed / 150) % 5;
  digitalWrite(ledPins[idx], HIGH);
}

bool buttonPressed() {
  static bool lastState = HIGH;
  bool reading = digitalRead(BTN_PIN);
  bool pressed = (lastState == HIGH && reading == LOW);
  lastState = reading;
  return pressed;
}

void setup() {
  Serial.begin(115200);
  pinMode(BTN_PIN, INPUT_PULLUP);

  for (int i = 0; i < 8; i++) pinMode(segPins[i], OUTPUT);
  for (int i = 0; i < 5; i++) pinMode(ledPins[i], OUTPUT);

  clearSegments();
  clearLEDs();

  randomSeed(analogRead(0));
  state = IDLE;
  stateStart = millis();
}

void loop() {
  unsigned long now = millis();

  switch (state) {
    case IDLE:
      clearLEDs();
      clearSegments();
      if (buttonPressed()) {
        randomDelay = random(1500, 4000);
        stateStart = now;
        state = WAITING;
      }
      break;

    case WAITING:
      chaseAnimation(now - stateStart);
      if (buttonPressed()) {
        for (int i = 0; i < 5; i++) digitalWrite(ledPins[i], HIGH);
        delay(400);
        clearLEDs();
        state = IDLE;
      } else if (now - stateStart >= randomDelay) {
        state = GO;
        goTime = now;
        clearLEDs();
        for (int i = 0; i < 5; i++) digitalWrite(ledPins[i], HIGH);
      }
      break;

    case GO:
      if (buttonPressed()) {
        reactionMs = now - goTime;
        clearLEDs();

        int val = reactionMs % 1000;
        resultDigits[0] = val / 100;
        resultDigits[1] = (val / 10) % 10;
        resultDigits[2] = val % 10;

        digitIndex = 0;
        digitShownAt = now;
        showDigit(resultDigits[0]);
        state = RESULT;
      }
      break;

    case RESULT:
      if (now - digitShownAt >= DIGIT_HOLD_MS) {
        digitIndex++;
        if (digitIndex < 3) {
          showDigit(resultDigits[digitIndex]);
          digitShownAt = now;
        } else {
          clearSegments();
          if (now - digitShownAt >= 1500) {
            state = IDLE;
          }
        }
      }
      break;
  }
}