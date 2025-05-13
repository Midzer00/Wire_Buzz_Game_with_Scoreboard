#include <LiquidCrystal.h>           // Pull in the LCD library so we can chat with the little screen
LiquidCrystal lcd(2, 3, 4, 5, 6, 7); // Tell the LCD which Arduino pins we're using

const int player1Btn = A2, player2Btn = A1, resetBtn = A0; // Buttons for Player 1, Player 2, and the Reset key
const int touchWire = 12, startWire = 10, endWire = 11;    // Wires that sense touching the maze, the start point, and the end point
const int redLED = 8, greenLED = 9, buzzerPin = 13;        // LEDs for feedback and a handy buzzer for sound

int currentPlayer = 0, player1Score = 0, player2Score = 0, touchCount = 0; // Keep track of who’s up and the running scores
unsigned long startTime, lastUpdateTime = 0;                               // Timing helpers so we can measure runs and refresh the LCD politely
bool gameRunning = false, justReset = false;                               // Flags that say “the race is on” or “we just smashed reset”

const int maxScore = 100, penaltyPerTouch = 5; // Everyone starts at 100, each bump costs 5 points
const unsigned long maxTime = 60000;           // One minute max to finish the wire (in milliseconds)

// A handy lookup table of pitch values (Hz) for every note in 8 octaves
const int Notes[8][12] = {
    {33, 35, 37, 39, 41, 44, 46, 49, 52, 55, 58, 62},                         // Octave 0
    {65, 69, 73, 78, 82, 87, 92, 98, 104, 110, 117, 123},                     // Octave 1
    {131, 139, 147, 156, 165, 175, 185, 196, 208, 220, 233, 247},             // Octave 2
    {262, 277, 294, 311, 330, 349, 370, 392, 415, 440, 466, 494},             // Octave 3
    {523, 554, 587, 622, 659, 698, 740, 784, 831, 880, 932, 988},             // Octave 4
    {1047, 1109, 1175, 1245, 1319, 1397, 1480, 1568, 1661, 1760, 1865, 1976}, // Octave 5
    {2093, 2217, 2349, 2489, 2637, 2794, 2960, 3136, 3322, 3520, 3729, 3951}, // Octave 6
    {4186, 4435, 4699, 4978, 5274, 5588, 5920, 6272, 6645, 7040, 7459, 7902}  // Octave 7
}; // End of pitch table

// Friendly names for each semitone so we don’t have to remember numbers
#define NOTE_C 0   // C
#define NOTE_CS 1  // C♯/D♭
#define NOTE_D 2   // D
#define NOTE_DS 3  // D♯/E♭
#define NOTE_E 4   // E
#define NOTE_F 5   // F
#define NOTE_FS 6  // F♯/G♭
#define NOTE_G 7   // G
#define NOTE_GS 8  // G♯/A♭
#define NOTE_A 9   // A
#define NOTE_AS 10 // A♯/B♭
#define NOTE_B 11  // B

int doomStep = 0;               // Which note in the Doom riff are we on?
unsigned long lastNoteTime = 0; // When did we play the last note?
int melodySpeed = 64;           // Delay between melody notes (ms)

void setup()
{
  lcd.begin(16, 2);                  // Fire up the LCD (16 columns × 2 rows)
  pinMode(player1Btn, INPUT_PULLUP); // Player 1 button with the built‑in pull‑up so it idles HIGH
  pinMode(player2Btn, INPUT_PULLUP); // Player 2 button ditto
  pinMode(resetBtn, INPUT_PULLUP);   // Reset button ditto
  pinMode(touchWire, INPUT_PULLUP);  // Maze wire senses LOW when you touch it
  pinMode(startWire, INPUT_PULLUP);  // Start pad
  pinMode(endWire, INPUT_PULLUP);    // Finish pad
  pinMode(redLED, OUTPUT);           // Red warning light
  pinMode(greenLED, OUTPUT);         // Green “go” light
  pinMode(buzzerPin, OUTPUT);        // Tiny speaker
  playStartupLEDAnimation();         // Quick light show so we know we’re alive
  lcd.setCursor(0, 0);
  lcd.print("Buzz Wire Game"); // Splash screen line 1
  lcd.setCursor(0, 1);
  lcd.print("P1/P2 to Start"); // Splash screen line 2
}

void loop()
{
  if (justReset)
  {                                // Did we just reset?
    waitUntilAllButtonsReleased(); // Chill until buttons are no longer held
    justReset = false;             // Reset handled
  }

  if (digitalRead(resetBtn) == LOW)
  {             // Someone hammering the reset key?
    delay(100); // Debounce
    if (digitalRead(resetBtn) == LOW)
    {
      resetGame();
      return;
    } // Yup, do it
  }

  if (!gameRunning && currentPlayer == 0)
  { // Idle state, no player chosen yet
    if (digitalRead(player1Btn) == LOW)
    { // Player 1 pressed their button
      delay(100);
      if (digitalRead(player1Btn) == LOW)
      {
        currentPlayer = 1;
        waitForStartWire();
      } // Lock in Player 1
    }
    else if (digitalRead(player2Btn) == LOW)
    { // Player 2 pressed theirs
      delay(100);
      if (digitalRead(player2Btn) == LOW)
      {
        currentPlayer = 2;
        waitForStartWire();
      } // Lock in Player 2
    }
  }

  if (!gameRunning && currentPlayer != 0 && digitalRead(startWire) == LOW)
  {             // Player ready and touching the start pad
    delay(100); // Debounce
    if (digitalRead(startWire) == LOW)
    {
      waitUntilAllButtonsReleased();
      startRun();
    } // They’re off!
  }

  if (gameRunning)
  { // Active game loop
    if (millis() - lastUpdateTime > 200)
    { // Time to refresh the LCD?
      updateGameDisplay();
      lastUpdateTime = millis();
    }
    playE1M1MelodyStep(); // Keep the background music going
    if (digitalRead(touchWire) == LOW)
    { // Did they bump the wire?
      handleTouch();
      delay(200); // Record it and give them a moment
    }
    if (digitalRead(endWire) == LOW)
      endRun(); // Reached the finish pad: stop the clock
    if (millis() - startTime > maxTime)
      timeOut(); // Took too long: auto‑timeout
  }
}

void waitForStartWire()
{
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Player ");
  lcd.print(currentPlayer);
  lcd.print(" Ready"); // Show who’s up
  lcd.setCursor(0, 1);
  lcd.print("Touch to Begin   "); // Ask them to tap the start pad
}

void startRun()
{
  touchCount = 0;
  gameRunning = true; // Reset touch count and mark game as live
  startTime = millis();
  doomStep = 0; // Note the start time and reset the music
  lastNoteTime = millis();
  digitalWrite(greenLED, HIGH); // Green means go
  digitalWrite(redLED, LOW);
  updateGameDisplay(); // First stats update
}

void updateGameDisplay()
{
  lcd.setCursor(0, 0);
  lcd.print("Player ");
  lcd.print(currentPlayer);
  lcd.print(" GO!      "); // Top line
  lcd.setCursor(0, 1);
  lcd.print("Time:");
  lcd.print((millis() - startTime) / 1000); // Elapsed seconds
  lcd.print("s T:");
  lcd.print(touchCount);
  lcd.print("   "); // Touch count
}

void handleTouch()
{
  touchCount++;               // Another bump on the wire
  digitalWrite(redLED, HIGH); // Flash red LED
  tone(buzzerPin, 300, 100);  // Short warning beep
  delay(100);                 // Let the beep play
  digitalWrite(redLED, LOW);  // LED off again
}

int calculateScore(unsigned long elapsedTime)
{
  int elapsedSeconds = elapsedTime / 1000;                                 // ms → s
  int timePenalty = (elapsedSeconds > 20) ? (elapsedSeconds - 20) * 2 : 0; // 2 points per second after 20s
  int penalty = touchCount * penaltyPerTouch;                              // 5 points per touch
  int score = maxScore - (timePenalty + penalty);                          // Take it off the top
  return score < 0 ? 0 : score;                                            // Never negative
}

void endRun()
{
  gameRunning = false;
  noTone(buzzerPin);
  digitalWrite(greenLED, LOW);
  digitalWrite(redLED, LOW);                        // Quiet and lights off
  int score = calculateScore(millis() - startTime); // Work out the score
  if (currentPlayer == 1)
    player1Score += score;
  else
    player2Score += score; // Add to the right total
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Player ");
  lcd.print(currentPlayer);
  lcd.print(": +");
  lcd.print(score); // Show gain
  lcd.setCursor(0, 1);
  lcd.print("Touches: ");
  lcd.print(touchCount);
  lcd.print("   "); // Show mishaps
  if (score == 0)
    playGameOverSound();
  else
    playSuccessSound();          // Sound effects
  delay(2000);                   // Pause to let it sink in
  showScores();                  // Overall score board
  waitUntilAllButtonsReleased(); // Wait for calm
  currentPlayer = 0;             // Back to idle mode
}

void timeOut()
{
  gameRunning = false; // Stop the clock
  noTone(buzzerPin);
  digitalWrite(greenLED, LOW);
  digitalWrite(redLED, LOW); // Lights out
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(" TIME'S UP!     "); // Big timeout message
  lcd.setCursor(0, 1);
  lcd.print("Score: 0        "); // Zero points, sorry
  playGameOverSound();           // Sad trombone (kind of)
  delay(2000);                   // Give them a moment
  showScores();                  // Refresh totals
  waitUntilAllButtonsReleased(); // All clear
  currentPlayer = 0;             // Ready for next player
}

void showScores()
{
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("P1:");
  lcd.print(player1Score);
  lcd.print(" P2:");
  lcd.print(player2Score); // Scoreboard
  lcd.setCursor(0, 1);
  lcd.print("P1/P2 = Play    "); // Hint to start again
}

void resetGame()
{
  player1Score = player2Score = touchCount = currentPlayer = 0; // Wipe everything clean
  gameRunning = false;
  justReset = true; // Flag that a reset happened
  noTone(buzzerPin);
  digitalWrite(greenLED, LOW);
  digitalWrite(redLED, LOW);
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Resetting...   "); // Little wait message
  delay(1000);
  lcd.setCursor(0, 0);
  lcd.print("Buzz Wire Game"); // Back to splash
  lcd.setCursor(0, 1);
  lcd.print("P1/P2 to Start");
}

void waitUntilAllButtonsReleased()
{
  while (digitalRead(player1Btn) == LOW || digitalRead(player2Btn) == LOW || digitalRead(resetBtn) == LOW || digitalRead(startWire) == LOW)
    delay(50); // Spin until buttons/wire go HIGH
}

void playNote(int octave, int note, int duration)
{
  tone(buzzerPin, Notes[octave][note]);
  delay(duration);
  noTone(buzzerPin); // Simple helper to play a single note
}

void noteDoomBase(int octave, int speed)
{
  playNote(octave - 1, NOTE_E, speed / 2);
  delay(speed / 2);                    // Quick pluck on the low E
  playNote(octave - 1, NOTE_E, speed); // Longer growl of low E
}

void playE1M1MelodyStep()
{
  static unsigned long lastStepTime = 0; // Remember when we last fired a note
  if (millis() - lastStepTime < melodySpeed)
    return; // Too soon? bail out
  switch (doomStep)
  {
  case 0:
    playNote(3, NOTE_B, melodySpeed);
    break; // B
  case 1:
    playNote(3, NOTE_G, melodySpeed);
    break; // G
  case 2:
    playNote(3, NOTE_E, melodySpeed);
    break; // E
  case 3:
    playNote(3, NOTE_C, melodySpeed);
    break; // C
  case 4:
    playNote(3, NOTE_E, melodySpeed);
    break; // E
  case 5:
    playNote(3, NOTE_G, melodySpeed);
    break; // G
  case 6:
    playNote(3, NOTE_B, melodySpeed);
    break; // B
  case 7:
    playNote(3, NOTE_G, melodySpeed);
    break; // G
  case 8:
    playNote(3, NOTE_B, melodySpeed);
    break; // B
  case 9:
    playNote(3, NOTE_G, melodySpeed);
    break; // G
  case 10:
    playNote(3, NOTE_E, melodySpeed);
    break; // E
  case 11:
    playNote(3, NOTE_G, melodySpeed);
    break; // G
  case 12:
    playNote(3, NOTE_B, melodySpeed);
    break; // B
  case 13:
    playNote(3, NOTE_G, melodySpeed);
    break; // G
  case 14:
    playNote(3, NOTE_B, melodySpeed);
    break; // B
  case 15:
    playNote(4, NOTE_E, melodySpeed);
    break; // High E
  case 16:
    noteDoomBase(3, 128);
    break; // Chug chug
  case 17:
    playNote(3, NOTE_E, 128);
    break; // E
  case 18:
    noteDoomBase(3, 128);
    break; // Chug chug
  case 19:
    playNote(3, NOTE_D, 128);
    break; // D
  case 20:
    noteDoomBase(3, 128);
    break; // Chug chug
  case 21:
    playNote(3, NOTE_C, 128);
    break; // C
  case 22:
    noteDoomBase(3, 128);
    break; // Chug chug
  case 23:
    playNote(3, NOTE_AS, 128);
    break; // A♯
  case 24:
    noteDoomBase(3, 128);
    break; // Chug chug
  case 25:
    playNote(3, NOTE_B, 128);
    break; // B
  case 26:
    playNote(3, NOTE_C, 128);
    break; // C
  case 27:
    noteDoomBase(3, 128);
    break; // Chug chug
  case 28:
    playNote(3, NOTE_E, 128);
    break; // E
  case 29:
    noteDoomBase(3, 128);
    break; // Chug chug
  case 30:
    playNote(3, NOTE_D, 128);
    break; // D
  case 31:
    noteDoomBase(3, 128);
    break; // Chug chug
  case 32:
    playNote(2, NOTE_AS, 256);
    break; // Deep A♯
  case 33:
    delay(400);
    break; // Dramatic pause
  default:
    doomStep = -1;
    break; // Reset sequence
  }
  doomStep++;              // Move on to the next step
  lastStepTime = millis(); // Note when we played
}

void playStartupLEDAnimation()
{
  for (int i = 0; i < 2; i++)
  { // Two rounds of blink‑blink
    digitalWrite(greenLED, HIGH);
    digitalWrite(redLED, LOW);
    delay(200); // Green on
    digitalWrite(greenLED, LOW);
    digitalWrite(redLED, HIGH);
    delay(200); // Red on
    digitalWrite(greenLED, HIGH);
    digitalWrite(redLED, HIGH);
    delay(200); // Both on
    digitalWrite(greenLED, LOW);
    digitalWrite(redLED, LOW);
    delay(200); // Both off
  }
}

void playGameOverSound()
{
  tone(buzzerPin, 220, 300);
  delay(350); // Waah (220 Hz)
  tone(buzzerPin, 196, 300);
  delay(350); // Waah lower
  tone(buzzerPin, 174, 500);
  delay(500); // Waah lowest
  noTone(buzzerPin);
}
