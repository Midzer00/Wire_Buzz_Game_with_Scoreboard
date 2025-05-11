#include <LiquidCrystal.h>
LiquidCrystal lcd(2, 3, 4, 5, 6, 7);

const int player1Btn = A2, player2Btn = A1, resetBtn = A0;
const int touchWire = 12, startWire = 10, endWire = 11;
const int redLED = 8, greenLED = 9, buzzerPin = 13;

int currentPlayer = 0, player1Score = 0, player2Score = 0, touchCount = 0;
unsigned long startTime, lastUpdateTime = 0;
bool gameRunning = false, justReset = false;

const int maxScore = 100, penaltyPerTouch = 5;
const unsigned long maxTime = 60000; // 60 seconds

const int Notes[8][12] = {
  {33,35,37,39,41,44,46,49,52,55,58,62}, {65,69,73,78,82,87,92,98,104,110,117,123},
  {131,139,147,156,165,175,185,196,208,220,233,247}, {262,277,294,311,330,349,370,392,415,440,466,494},
  {523,554,587,622,659,698,740,784,831,880,932,988}, {1047,1109,1175,1245,1319,1397,1480,1568,1661,1760,1865,1976},
  {2093,2217,2349,2489,2637,2794,2960,3136,3322,3520,3729,3951}, {4186,4435,4699,4978,5274,5588,5920,6272,6645,7040,7459,7902}
};

#define NOTE_C 0
#define NOTE_CS 1
#define NOTE_D 2
#define NOTE_DS 3
#define NOTE_E 4
#define NOTE_F 5
#define NOTE_FS 6
#define NOTE_G 7
#define NOTE_GS 8
#define NOTE_A 9
#define NOTE_AS 10
#define NOTE_B 11

int doomStep = 0;
unsigned long lastNoteTime = 0;
int melodySpeed = 64;

void setup() {
  lcd.begin(16, 2);
  pinMode(player1Btn, INPUT_PULLUP);
  pinMode(player2Btn, INPUT_PULLUP);
  pinMode(resetBtn, INPUT_PULLUP);
  pinMode(touchWire, INPUT_PULLUP);
  pinMode(startWire, INPUT_PULLUP);
  pinMode(endWire, INPUT_PULLUP);
  pinMode(redLED, OUTPUT);
  pinMode(greenLED, OUTPUT);
  pinMode(buzzerPin, OUTPUT);
  playStartupLEDAnimation();
  lcd.setCursor(0, 0); lcd.print("Buzz Wire Game");
  lcd.setCursor(0, 1); lcd.print("P1/P2 to Start");
}

void loop() {
  if (justReset) { waitUntilAllButtonsReleased(); justReset = false; }

  if (digitalRead(resetBtn) == LOW) { delay(100);
    if (digitalRead(resetBtn) == LOW) { resetGame(); return; } }

  if (!gameRunning && currentPlayer == 0) {
    if (digitalRead(player1Btn) == LOW) { delay(100);
      if (digitalRead(player1Btn) == LOW) { currentPlayer = 1; waitForStartWire(); } }
    else if (digitalRead(player2Btn) == LOW) { delay(100);
      if (digitalRead(player2Btn) == LOW) { currentPlayer = 2; waitForStartWire(); } }
  }

  if (!gameRunning && currentPlayer != 0 && digitalRead(startWire) == LOW) {
    delay(100);
    if (digitalRead(startWire) == LOW) { waitUntilAllButtonsReleased(); startRun(); }
  }

  if (gameRunning) {
    if (millis() - lastUpdateTime > 200) { updateGameDisplay(); lastUpdateTime = millis(); }
    playE1M1MelodyStep();
    if (digitalRead(touchWire) == LOW) { handleTouch(); delay(200); }
    if (digitalRead(endWire) == LOW) endRun();
    if (millis() - startTime > maxTime) timeOut();
  }
}

void waitForStartWire() {
  lcd.clear();
  lcd.setCursor(0, 0); lcd.print("Player "); lcd.print(currentPlayer); lcd.print(" Ready");
  lcd.setCursor(0, 1); lcd.print("Touch to Begin   ");
}

void startRun() {
  touchCount = 0; gameRunning = true; startTime = millis(); doomStep = 0; lastNoteTime = millis();
  digitalWrite(greenLED, HIGH); digitalWrite(redLED, LOW);
  updateGameDisplay();
}

void updateGameDisplay() {
  lcd.setCursor(0, 0); lcd.print("Player "); lcd.print(currentPlayer); lcd.print(" GO!      ");
  lcd.setCursor(0, 1); lcd.print("Time:"); lcd.print((millis() - startTime) / 1000);
  lcd.print("s T:"); lcd.print(touchCount); lcd.print("   ");
}

void handleTouch() {
  touchCount++;
  digitalWrite(redLED, HIGH);
  tone(buzzerPin, 300, 100);
  delay(100);
  digitalWrite(redLED, LOW);
}

int calculateScore(unsigned long elapsedTime) {
  int elapsedSeconds = elapsedTime / 1000;
  int timePenalty = (elapsedSeconds > 20) ? (elapsedSeconds - 20) * 2 : 0;
  int penalty = touchCount * penaltyPerTouch;
  int score = maxScore - (timePenalty + penalty);
  return score < 0 ? 0 : score;
}

void endRun() {
  gameRunning = false;
  noTone(buzzerPin); digitalWrite(greenLED, LOW); digitalWrite(redLED, LOW);
  int score = calculateScore(millis() - startTime);
  if (currentPlayer == 1) player1Score += score;
  else player2Score += score;
  lcd.clear();
  lcd.setCursor(0, 0); lcd.print("Player "); lcd.print(currentPlayer); lcd.print(": +"); lcd.print(score);
  lcd.setCursor(0, 1); lcd.print("Touches: "); lcd.print(touchCount); lcd.print("   ");
  if (score == 0) playGameOverSound();
  else playSuccessSound();
  delay(2000);
  showScores();
  waitUntilAllButtonsReleased();
  currentPlayer = 0;
}

void timeOut() {
  gameRunning = false;
  noTone(buzzerPin); digitalWrite(greenLED, LOW); digitalWrite(redLED, LOW);
  lcd.clear(); lcd.setCursor(0, 0); lcd.print(" TIME'S UP!     ");
  lcd.setCursor(0, 1); lcd.print("Score: 0        ");
  playGameOverSound();
  delay(2000);
  showScores();
  waitUntilAllButtonsReleased();
  currentPlayer = 0;
}

void showScores() {
  lcd.clear();
  lcd.setCursor(0, 0); lcd.print("P1:"); lcd.print(player1Score); lcd.print(" P2:"); lcd.print(player2Score);
  lcd.setCursor(0, 1); lcd.print("P1/P2 = Play    ");
}

void resetGame() {
  player1Score = player2Score = touchCount = currentPlayer = 0;
  gameRunning = false; justReset = true;
  noTone(buzzerPin);
  digitalWrite(greenLED, LOW); digitalWrite(redLED, LOW);
  lcd.clear(); lcd.setCursor(0, 0); lcd.print("Resetting...   ");
  delay(1000);
  lcd.setCursor(0, 0); lcd.print("Buzz Wire Game");
  lcd.setCursor(0, 1); lcd.print("P1/P2 to Start");
}

void waitUntilAllButtonsReleased() {
  while (digitalRead(player1Btn) == LOW || digitalRead(player2Btn) == LOW || digitalRead(resetBtn) == LOW || digitalRead(startWire) == LOW) delay(50);
}

void playNote(int octave, int note, int duration) {
  tone(buzzerPin, Notes[octave][note]); delay(duration); noTone(buzzerPin);
}

void noteDoomBase(int octave, int speed) {
  playNote(octave - 1, NOTE_E, speed / 2); delay(speed / 2);
  playNote(octave - 1, NOTE_E, speed);
}

void playE1M1MelodyStep() {
  static unsigned long lastStepTime = 0;
  if (millis() - lastStepTime < melodySpeed) return;
  switch (doomStep) {
    case 0: playNote(3, NOTE_B, melodySpeed); break;
    case 1: playNote(3, NOTE_G, melodySpeed); break;
    case 2: playNote(3, NOTE_E, melodySpeed); break;
    case 3: playNote(3, NOTE_C, melodySpeed); break;
    case 4: playNote(3, NOTE_E, melodySpeed); break;
    case 5: playNote(3, NOTE_G, melodySpeed); break;
    case 6: playNote(3, NOTE_B, melodySpeed); break;
    case 7: playNote(3, NOTE_G, melodySpeed); break;
    case 8: playNote(3, NOTE_B, melodySpeed); break;
    case 9: playNote(3, NOTE_G, melodySpeed); break;
    case 10: playNote(3, NOTE_E, melodySpeed); break;
    case 11: playNote(3, NOTE_G, melodySpeed); break;
    case 12: playNote(3, NOTE_B, melodySpeed); break;
    case 13: playNote(3, NOTE_G, melodySpeed); break;
    case 14: playNote(3, NOTE_B, melodySpeed); break;
    case 15: playNote(4, NOTE_E, melodySpeed); break;
    case 16: noteDoomBase(3, 128); break;
    case 17: playNote(3, NOTE_E, 128); break;
    case 18: noteDoomBase(3, 128); break;
    case 19: playNote(3, NOTE_D, 128); break;
    case 20: noteDoomBase(3, 128); break;
    case 21: playNote(3, NOTE_C, 128); break;
    case 22: noteDoomBase(3, 128); break;
    case 23: playNote(3, NOTE_AS, 128); break;
    case 24: noteDoomBase(3, 128); break;
    case 25: playNote(3, NOTE_B, 128); break;
    case 26: playNote(3, NOTE_C, 128); break;
    case 27: noteDoomBase(3, 128); break;
    case 28: playNote(3, NOTE_E, 128); break;
    case 29: noteDoomBase(3, 128); break;
    case 30: playNote(3, NOTE_D, 128); break;
    case 31: noteDoomBase(3, 128); break;
    case 32: playNote(2, NOTE_AS, 256); break;
    case 33: delay(400); break;
    default: doomStep = -1; break;
  }
  doomStep++;
  lastStepTime = millis();
}

void playStartupLEDAnimation() {
  for (int i = 0; i < 2; i++) {
    digitalWrite(greenLED, HIGH); digitalWrite(redLED, LOW); delay(200);
    digitalWrite(greenLED, LOW); digitalWrite(redLED, HIGH); delay(200);
    digitalWrite(greenLED, HIGH); digitalWrite(redLED, HIGH); delay(200);
    digitalWrite(greenLED, LOW); digitalWrite(redLED, LOW); delay(200);
  }
}

void playGameOverSound() {
  tone(buzzerPin, 220, 300); delay(350);
  tone(buzzerPin, 196, 300); delay(350);
  tone(buzzerPin, 174, 500); delay(500);
  noTone(buzzerPin);
}

void playSuccessSound() {
  tone(buzzerPin, 523, 150); delay(200);
  tone(buzzerPin, 659, 150); delay(200);
  tone(buzzerPin, 784, 300); delay(300);
  noTone(buzzerPin);
}