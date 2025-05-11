# Buzz Wire Game

An Arduino-based implementation of the classic Buzz Wire game with two-player support, scoring system, and sound effects.

## Simulation
You can view and interact with the circuit simulation on Tinkercad: [Wire Loop Buzz Game with Scoreboard](https://www.tinkercad.com/things/9Tp2xygvUrM-wire-loop-buzz-game-with-scoreboard?sharecode=whqO1mfj3Hv7q8OD0pEfH_AgG6kN3L8t2KjAp4YVpUQ)

## Features

- Two-player gameplay
- LCD display for scores and game status
- LED indicators (red/green)
- Sound effects and background music
- Time-based scoring system
- Touch detection
- Reset functionality

## Hardware Requirements

- Arduino board
- 16x2 LCD display
- 2 LEDs (red and green)
- Buzzer
- 3 buttons (Player 1, Player 2, Reset)
- Wire for the game track
- Resistors and connecting wires

## Pin Configuration

- LCD: Pins 2-7
- Player 1 Button: A2
- Player 2 Button: A1
- Reset Button: A0
- Touch Wire: Pin 12
- Start Wire: Pin 10
- End Wire: Pin 11
- Red LED: Pin 8
- Green LED: Pin 9
- Buzzer: Pin 13

## Game Rules

1. Players take turns to complete the wire track
2. Each touch of the wire results in a penalty
3. Time limit: 60 seconds
4. Score is calculated based on:
   - Time taken
   - Number of touches
   - Maximum score: 100 points

## Setup

1. Connect the hardware according to the pin configuration
2. Upload the code to your Arduino board
3. Press Player 1 or Player 2 button to start
4. Touch the start wire to begin the game
5. Try to complete the track without touching the wire
6. Press reset button to start a new game

   ![image](https://github.com/user-attachments/assets/b7d099e2-c0c8-4e86-ad71-f5c0528f064f)
![image](https://github.com/user-attachments/assets/c0283644-fc09-4c8e-91c4-3a0645858790)


## License

This project is open source and available under the MIT License. 
