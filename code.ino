#include <LiquidCrystal.h> // Include the LiquidCrystal library for LCD control
LiquidCrystal lcd(2, 3, 4, 5, 6, 7); // Instantiate the LCD using the chosen digital pins

const int player1Btn = A2, player2Btn = A1, resetBtn = A0; // Analog pins used for the three push-buttons
const int touchWire = 12, startWire = 10, endWire = 11; // Digital pins monitoring the maze wires
const int redLED = 8, greenLED = 9, buzzerPin = 13; // Output pins driving LEDs and the piezo buzzer

int currentPlayer = 0, player1Score = 0, player2Score = 0, touchCount = 0; // Live game counters   
unsigned long startTime, lastUpdateTime = 0; // Timestamps tracking gameplay and screen refreshes
bool gameRunning = false, justReset = false; // Flags keeping tabs on game state

const int maxScore = 100, penaltyPerTouch = 5; // Highest starting score and the cost of each mistake
const unsigned long maxTime = 60000; // 60 seconds // Maximum time allowed for a single run (in ms)

const int Notes[8][12] = { // Lookup table: 8 octaves × 12 semitones of note frequencies
  {33,35,37,39,41,44,46,49,52,55,58,62}, {65,69,73,78,82,87,92,98,104,110,117,123}, // Octaves 0 & 1
  {131,139,147,156,165,175,185,196,208,220,233,247}, {262,277,294,311,330,349,370,392,415,440,466,494}, // Octaves 2 & 3
  {523,554,587,622,659,698,740,784,831,880,932,988}, {1047,1109,1175,1245,1319,1397,1480,1568,1661,1760,1865,1976}, // Octaves 4 & 5
  {2093,2217,2349,2489,2637,2794,2960,3136,3322,3520,3729,3951}, {4186,4435,4699,4978,5274,5588,5920,6272,6645,7040,7459,7902} // Octaves 6 & 7
}; // End of frequency table 🎵

#define NOTE_C 0 // Enum-like indices for each musical note below
#define NOTE_CS 1 // C-sharp
#define NOTE_D 2 // D
#define NOTE_DS 3 // D-sharp
#define NOTE_E 4 // E
#define NOTE_F 5 // F
#define NOTE_FS 6 // F-sharp
#define NOTE_G 7 // G
#define NOTE_GS 8 // G-sharp
#define NOTE_A 9 // A
#define NOTE_AS 10 // A-sharp
#define NOTE_B 11 // B

int doomStep = 0; // Which part of the Doom E1M1 riff we're on
unsigned long lastNoteTime = 0; // When the last note was played
int melodySpeed = 64; // Note duration for most of the melody (ms)

void setup() { // Standard Arduino setup — runs once at boot
  lcd.begin(16, 2); // Fire up the 16×2 LCD
  pinMode(player1Btn, INPUT_PULLUP); // Player 1 button with internal pull-up
  pinMode(player2Btn, INPUT_PULLUP); // Player 2 button with internal pull-up
  pinMode(resetBtn, INPUT_PULLUP); // Reset button with internal pull-up
  pinMode(touchWire, INPUT_PULLUP); // Maze wire normally HIGH
  pinMode(startWire, INPUT_PULLUP); // Start wire normally HIGH
  pinMode(endWire, INPUT_PULLUP); // End wire normally HIGH
  pinMode(redLED, OUTPUT); // Red LED indicates mistakes
  pinMode(greenLED, OUTPUT); // Green LED shows successful status
  pinMode(buzzerPin, OUTPUT); // Piezo buzzer output
  playStartupLEDAnimation(); // Fun little LED dance on power-up ✨
  lcd.setCursor(0, 0); lcd.print("Buzz Wire Game"); // Write the game title on row 1
  lcd.setCursor(0, 1); lcd.print("P1/P2 to Start"); // Prompt players on row 2
} // setup ends here 🚀

void loop() { // Main loop — repeats forever
  if (justReset) { waitUntilAllButtonsReleased(); justReset = false; } // Ignore leftover button presses after a reset

  if (digitalRead(resetBtn) == LOW) { delay(100); // Debounce the reset button
    if (digitalRead(resetBtn) == LOW) { resetGame(); return; } } // If still held, perform the reset

  if (!gameRunning && currentPlayer == 0) { // Waiting for someone to claim a turn
    if (digitalRead(player1Btn) == LOW) { delay(100); // Debounce P1
      if (digitalRead(player1Btn) == LOW) { currentPlayer = 1; waitForStartWire(); } }
    else if (digitalRead(player2Btn) == LOW) { delay(100); // Debounce P2
      if (digitalRead(player2Btn) == LOW) { currentPlayer = 2; waitForStartWire(); } }
  }

  if (!gameRunning && currentPlayer != 0 && digitalRead(startWire) == LOW) { // Player touched the start wire
    delay(100); // Cheap debounce
    if (digitalRead(startWire) == LOW) { waitUntilAllButtonsReleased(); startRun(); }
  }

  if (gameRunning) { // Active gameplay region
    if (millis() - lastUpdateTime > 200) { updateGameDisplay(); lastUpdateTime = millis(); } // Refresh LCD ~5×/sec
    playE1M1MelodyStep(); // Advance the Doom soundtrack one beat
    if (digitalRead(touchWire) == LOW) { handleTouch(); delay(200); } // Player goofed — register touch
    if (digitalRead(endWire) == LOW) endRun(); // They made it to the finish!
    if (millis() - startTime > maxTime) timeOut(); // Too slow — time's up
  }
} // loop ends 🔁

void waitForStartWire() { // Prompt player to grab the handle
  lcd.clear();
  lcd.setCursor(0, 0); lcd.print("Player "); lcd.print(currentPlayer); lcd.print(" Ready"); // Show who's up
  lcd.setCursor(0, 1); lcd.print("Touch to Begin   "); // Ask them to touch start wire
} // waitForStartWire ends

void startRun() { // Begin the timer and game stats
  touchCount = 0; gameRunning = true; startTime = millis(); doomStep = 0; lastNoteTime = millis(); // Reset counters
  digitalWrite(greenLED, HIGH); digitalWrite(redLED, LOW); // Green means GO
  updateGameDisplay(); // Show zeroed stats
} // startRun ends

void updateGameDisplay() { // Keep the LCD up-to-date
  lcd.setCursor(0, 0); lcd.print("Player "); lcd.print(currentPlayer); lcd.print(" GO!      "); // Row 1 message
  lcd.setCursor(0, 1); lcd.print("Time:"); lcd.print((millis() - startTime) / 1000); // Seconds elapsed
  lcd.print("s T:"); lcd.print(touchCount); lcd.print("   "); // Touches so far
} // updateGameDisplay ends

void handleTouch() { // Register a collision with the wire
  touchCount++; // Increment penalty counter
  digitalWrite(redLED, HIGH); // Flash red LED
  tone(buzzerPin, 300, 100); // Short buzz
  delay(100); // Give the player a moment to notice
  digitalWrite(redLED, LOW); // Turn red LED off
} // handleTouch ends

int calculateScore(unsigned long elapsedTime) { // Crunch the numbers for final score
  int elapsedSeconds = elapsedTime / 1000; // Convert ms → s
  int timePenalty = (elapsedSeconds > 20) ? (elapsedSeconds - 20) * 2 : 0; // 2-point deduction per second after 20s
  int penalty = touchCount * penaltyPerTouch; // Lost points from touching the wire
  int score = maxScore - (timePenalty + penalty); // Subtract penalties from max
  return score < 0 ? 0 : score; // Never go below zero
} // calculateScore ends

void endRun() { // Handle successful completion
  gameRunning = false; // Stop main game logic
  noTone(buzzerPin); digitalWrite(greenLED, LOW); digitalWrite(redLED, LOW); // Quiet & LEDs off
  int score = calculateScore(millis() - startTime); // Compute score
  if (currentPlayer == 1) player1Score += score; // Update totals
  else player2Score += score;
  lcd.clear();
  lcd.setCursor(0, 0); lcd.print("Player "); lcd.print(currentPlayer); lcd.print(": +"); lcd.print(score); // Show earned points
  lcd.setCursor(0, 1); lcd.print("Touches: "); lcd.print(touchCount); lcd.print("   "); // Show penalties
  if (score == 0) playGameOverSound(); // Sad trombone if zero
  else playSuccessSound(); // Happy tune otherwise
  delay(2000); // Give them time to read
  showScores(); // Display cumulative scores
  waitUntilAllButtonsReleased(); // Make sure buttons released
  currentPlayer = 0; // Ready for next player
} // endRun ends

void timeOut() { // What to do when the timer expires
  gameRunning = false; // Halt gameplay
  noTone(buzzerPin); digitalWrite(greenLED, LOW); digitalWrite(redLED, LOW); // Stop sounds & LEDs
  lcd.clear(); lcd.setCursor(0, 0); lcd.print(" TIME'S UP!     "); // Big timeout notice
  lcd.setCursor(0, 1); lcd.print("Score: 0        "); // Zero points
  playGameOverSound(); // Play defeat jingle
  delay(2000); // Give them a sec
  showScores(); // Back to scoreboard
  waitUntilAllButtonsReleased(); // Debounce
  currentPlayer = 0; // Back to idle
} // timeOut ends

void showScores() { // Summarize cumulative points
  lcd.clear();
  lcd.setCursor(0, 0); lcd.print("P1:"); lcd.print(player1Score); lcd.print(" P2:"); lcd.print(player2Score); // Row 1 scores
  lcd.setCursor(0, 1); lcd.print("P1/P2 = Play    "); // Prompt for next round
} // showScores ends

void resetGame() { // Wipe everything and start over
  player1Score = player2Score = touchCount = currentPlayer = 0; // Zero out all counters
  gameRunning = false; justReset = true; // Mark reset in progress
  noTone(buzzerPin); // Silence buzzer
  digitalWrite(greenLED, LOW); digitalWrite(redLED, LOW); // LEDs off
  lcd.clear(); lcd.setCursor(0, 0); lcd.print("Resetting...   "); // Display reset message
  delay(1000); // Brief pause
  lcd.setCursor(0, 0); lcd.print("Buzz Wire Game"); // Restore title
  lcd.setCursor(0, 1); lcd.print("P1/P2 to Start"); // Restore prompt
} // resetGame ends

void waitUntilAllButtonsReleased() { // Busy-wait until no inputs are held
  while (digitalRead(player1Btn) == LOW || digitalRead(player2Btn) == LOW || digitalRead(resetBtn) == LOW || digitalRead(startWire) == LOW) delay(50); // Check every 50 ms
} // waitUntilAllButtonsReleased ends

void playNote(int octave, int note, int duration) { // Utility to sound a specific note
  tone(buzzerPin, Notes[octave][note]); delay(duration); noTone(buzzerPin); // Beep for 'duration' ms
} // playNote ends

void noteDoomBase(int octave, int speed) { // Simple helper that plays the E note bassline
  playNote(octave - 1, NOTE_E, speed / 2); delay(speed / 2); // Short E
  playNote(octave - 1, NOTE_E, speed); // Long E
} // noteDoomBase ends

void playE1M1MelodyStep() { // Play one incremental piece of the Doom E1M1 riff
  static unsigned long lastStepTime = 0; // Local static structure to keep timing
  if (millis() - lastStepTime < melodySpeed) return; // Only proceed if enough time passed
  switch (doomStep) { // Switch through all 34 steps
    case 0: playNote(3, NOTE_B, melodySpeed); break; // B note
    case 1: playNote(3, NOTE_G, melodySpeed); break; // G note
    case 2: playNote(3, NOTE_E, melodySpeed); break; // E note
    case 3: playNote(3, NOTE_C, melodySpeed); break; // C note
    case 4: playNote(3, NOTE_E, melodySpeed); break; // E again
    case 5: playNote(3, NOTE_G, melodySpeed); break; // G again
    case 6: playNote(3, NOTE_B, melodySpeed); break; // B again
    case 7: playNote(3, NOTE_G, melodySpeed); break; // G again
    case 8: playNote(3, NOTE_B, melodySpeed); break; // Repeat pattern
    case 9: playNote(3, NOTE_G, melodySpeed); break; // ...
    case 10: playNote(3, NOTE_E, melodySpeed); break;
    case 11: playNote(3, NOTE_G, melodySpeed); break;
    case 12: playNote(3, NOTE_B, melodySpeed); break;
    case 13: playNote(3, NOTE_G, melodySpeed); break;
    case 14: playNote(3, NOTE_B, melodySpeed); break;
    case 15: playNote(4, NOTE_E, melodySpeed); break; // Jump up an octave
    case 16: noteDoomBase(3, 128); break; // Bassline helper
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
    case 32: playNote(2, NOTE_AS, 256); break; // Sustained lower A#
    case 33: delay(400); break; // Brief pause
    default: doomStep = -1; break; // Restart riff next time
  }
  doomStep++; // Advance to next note index
  lastStepTime = millis(); // Save timestamp for pacing
} // playE1M1MelodyStep ends

void playStartupLEDAnimation() { // Fun lighting on startup
  for (int i = 0; i < 2; i++) { // Repeat twice
    digitalWrite(greenLED, HIGH); digitalWrite(redLED, LOW); delay(200); // Green flash
    digitalWrite(greenLED, LOW); digitalWrite(redLED, HIGH); delay(200); // Red flash
    digitalWrite(greenLED, HIGH); digitalWrite(redLED, HIGH); delay(200); // Both together
    digitalWrite(greenLED, LOW); digitalWrite(redLED, LOW); delay(200); // All off
  }
} // playStartupLEDAnimation ends

void playGameOverSound() { // Low descending tones for failure
  tone(buzzerPin, 220, 300); delay(350); // A3
  tone(buzzerPin, 196, 300); delay(350); // G3
  tone(buzzerPin, 174, 500); delay(500); // F3
  noTone(buzzerPin); // Silence
} // playGameOverSound ends

void playSuccessSound() { // Quick happy arpeggio for success
  tone(buzzerPin, 523, 150); delay(200); // C5
  tone(buzzerPin, 659, 150); delay(200); // E5
  tone(buzzerPin, 784, 300); delay(300); // G5
  noTone(buzzerPin); // Done!
} // playSuccessSound ends