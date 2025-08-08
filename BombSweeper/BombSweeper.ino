/*
  Two-Player Bomb Sweeper Game
  ----------------------------
  This is a two-player bomb-clearing game using an 8x8 NeoPixel LED matrix and joystick input.
  Players take turns moving a 2x2 cursor around the grid to uncover tiles and score points.
  Avoid bombs! Hitting one halves your score. The game ends when the entire grid is cleared.

  Hardware Required:
  - Arduino-compatible board
  - 8x8 NeoPixel LED matrix (WS2812)
  - Analog joystick (X and Y to A1 and A0)
  - Pushbutton (connected to digital pin 2 with INPUT_PULLUP)
  - I2C LCD display (16x2, address 0x27)
  - External power recommended for LED matrix

  Controls:
  - Move the joystick to shift the 2x2 cursor.
  - Press the button to clear a tile.
  - Scores and results are shown on the LCD.
*/

// ========== Libraries ==========
#include <Adafruit_NeoPixel.h>
#include <LiquidCrystal_I2C.h>
#include <Wire.h>

// ========== Constants & Pins ==========
#define VRX_PIN A1          // Joystick X-axis
#define VRY_PIN A0         // Joystick Y-axis
#define LED_PIN 6           // NeoPixel data pin
#define LED_COUNT 64        // Total number of LEDs
#define DEADZONE 90        // Joystick deadzone
#define BLINK_DELAY 500   // Delay between blinks
#define MOVE_INTERVAL 150
#define P1 0
#define P2 1

// ========== Objects ==========
Adafruit_NeoPixel strip(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);
LiquidCrystal_I2C lcd(0x27, 16, 2);

// ========== Globals ==========
unsigned long scoreP1 = 0;
unsigned long scoreP2 = 0;
byte turn = P1;
uint8_t brightness = 8;
unsigned long currentTime;
unsigned long lastMoveTime;
byte bombCount = random(1, 5);
byte clearedCount = 0;

const uint8_t EMPTY = 0;
const uint8_t BOMB = 1;
const uint8_t CLEARED_P1 = 2;
const uint8_t CLEARED_P2 = 3;
const uint8_t CLEARED_BOMB = 4;
const uint8_t CURSOR = 5;

uint8_t blinks = 0;
uint8_t grid[8][8];

// Cursor position
uint8_t cursorX = 4;
uint8_t cursorY = 4;
bool checkJoy = false;
int xValue = 0;
int yValue = 0;

// ========== Initialization ==========
void initGame() {

  // Clear the global grid
  for (int row = 0; row < 8; row++) {
    for (int col = 0; col < 8; col++) {
      grid[row][col] = EMPTY;
    }
  }

  while (bombCount > 0) {
    byte row = random(0, 3)*2;
    byte col = random(0, 3)*2;
    if (grid[row][col] != BOMB) {
      grid[row][col] = BOMB;
      grid[row+1][col] = BOMB;
      grid[row][col+1] = BOMB;
      grid[row+1][col+1] = BOMB;
      bombCount--;
    }
  }

  scoreP1 = 0;
  scoreP2 = 0;
  turn = P1;
  clearedCount = 0;
  startLCD();
  strip.show();
  cursorX = 4;
  updateLeds();
}

// ========== Setup Functions ==========

void startLCD() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Bomb Sweeper");
  lcd.setCursor(0, 1);
  lcd.print("Click to Start!");
}

// ========== LED Functions ==========
void led(uint8_t row, uint8_t column, uint8_t r, uint8_t g, uint8_t b) {
  if ((row % 2) == 0) {
    strip.setPixelColor(row * 8 + column, strip.Color(r, g, b));
  }
  else {
    strip.setPixelColor(row * 8 + (7 - column), strip.Color(r, g, b));
  }
}

void updateLeds() {
  strip.clear();

  for (int row = 0; row < 8; row++) {
    for (int col = 0; col < 8; col++) {
      // Draw Hidden
      if (grid[row][col] != CLEARED_P1 && grid[row][col] != CLEARED_P2 && grid[row][col] != BOMB) {
        led(row, col, 0, 0, 0);
      }

      // Draw Bomb
      if (grid[row][col] == CLEARED_BOMB) {
        led(row, col, brightness, 0, 0);
      }

      // Draw Cleared
      if (grid[row][col] == CLEARED_P1) {
        led(row, col, brightness/2, 0, brightness/2);
      }
      if (grid[row][col] == CLEARED_P2) {
        led(row, col, 0, 0, brightness/2);
      }

      // Draw Cursor
      if (turn == P1) {
        led(cursorY, cursorX, brightness * 2, 0, brightness * 2);
        led(cursorY + 1, cursorX, brightness * 2, 0, brightness * 2);
        led(cursorY, cursorX + 1, brightness * 2, 0, brightness * 2);
        led(cursorY + 1, cursorX + 1, brightness * 2, 0, brightness * 2);
      } else {
        led(cursorY, cursorX, 0, 0, brightness * 2);
        led(cursorY + 1, cursorX, 0, 0, brightness * 2);
        led(cursorY, cursorX + 1, 0, 0, brightness * 2);
        led(cursorY + 1, cursorX + 1, 0, 0, brightness * 2);
      }
    }
  }

  strip.show();
}

// ========== LCD Display ==========
void updateLCD() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("P1 Score: " + String(scoreP1));
  lcd.setCursor(0, 1);
  lcd.print("P2 Score: " + String(scoreP2));
}

void gameoverLCD() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Gameover!");
  lcd.setCursor(0, 1);
  if (scoreP1 > scoreP2) {
    lcd.print("Player 1 WINS!");
  }
  if (scoreP2 > scoreP1) {
    lcd.print("Player 2 WINS!");
  }
  if (scoreP1 == scoreP2) {
    lcd.print("DRAW!  (WTF)");
  }
  delay(2000);
  initGame();
}

void blink(uint8_t times, uint8_t delayTime, uint8_t r, uint8_t g, uint8_t b) {
  for (int i = 0; i < times; i++) {
    strip.clear();
    for (int row = 0; row < 8; row++) {
      for (int column = 0; column < 8; column++) {
        led(row, column, r, g, b);
      }
    }
    strip.show();
    delay(delayTime);
    strip.clear();
    strip.show();
    delay(delayTime);
  }
}

// ========== Game Logic ==========

void gameover()
{
  for (int i = 0; i < LED_COUNT; i++)
    {
      strip.setPixelColor(i, strip.Color(0, brightness, 0));
      strip.show();
      delay(40);
    }
  if (scoreP1 > scoreP2) {
    blink(4, 500, brightness*2, 0, brightness*2);
  }

  if (scoreP2 > scoreP1) {
    blink(4, 500, 0, 0, brightness*2);
  }

  if (scoreP2 == scoreP1) {
    blink(4, 500, 0, 0, 0);
  }
  gameoverLCD();
  initGame();
}

// ========== Arduino Core ==========
void setup() {
  strip.begin();
  lcd.begin();
  Serial.begin(9600);
  pinMode(2, INPUT_PULLUP);
  randomSeed(analogRead(2));
  initGame();
}

void loop() {
  updateLeds();

  xValue = analogRead(VRX_PIN);
  yValue = analogRead(VRY_PIN);
  int buttonState = digitalRead(2);

  // Handle shooting
  if (buttonState == LOW && !checkJoy) {
    checkJoy = true;
    Serial.println("Button Pressed");

    if (grid[cursorY][cursorX] != CLEARED_P1 && grid[cursorY][cursorX] != CLEARED_P2 && grid[cursorY][cursorX] != CLEARED_BOMB) {
      if (grid[cursorY][cursorX] == BOMB) {
        blink(2, BLINK_DELAY, brightness, 0, 0);

        if (turn == P1) {
          scoreP1 = scoreP1/2;
        } else {
          scoreP2 = scoreP2/2;
        }
        grid[cursorY][cursorX] = CLEARED_BOMB;
        grid[cursorY + 1][cursorX] = CLEARED_BOMB;
        grid[cursorY][cursorX + 1] = CLEARED_BOMB;
        grid[cursorY + 1][cursorX + 1] = CLEARED_BOMB;
      } else {
        if (turn == P1) {
          grid[cursorY][cursorX] = CLEARED_P1;
          grid[cursorY + 1][cursorX] = CLEARED_P1;
          grid[cursorY][cursorX + 1] = CLEARED_P1;
          grid[cursorY + 1][cursorX + 1] = CLEARED_P1;
        } else {
          grid[cursorY][cursorX] = CLEARED_P2;
          grid[cursorY + 1][cursorX] = CLEARED_P2;
          grid[cursorY][cursorX + 1] = CLEARED_P2;
          grid[cursorY + 1][cursorX + 1] = CLEARED_P2;
        }

        if (turn == P1) {
          scoreP1 += 10*clearedCount;
        } else {
          scoreP2 += 10*clearedCount;
        }
      }

      clearedCount += 4;
      updateLeds();
      updateLCD();

      // Change turns
      if (turn == P1) {
        turn = P2;
      } else {
        turn = P1;
      }
    }
  } else if (buttonState == HIGH && checkJoy == 1) {
    checkJoy = false;
  }

  // Handle cursor movement
  unsigned long currentTime = millis();
  if (currentTime - lastMoveTime >= MOVE_INTERVAL) {
    lastMoveTime = currentTime;

    if (xValue > 512 + DEADZONE) {
      if (cursorX > 1) {
        cursorX -= 2;
      }
      else {
        cursorX = 6;
      }
    } else if (xValue < 512 - DEADZONE) {
      if (cursorX < 6) {
        cursorX += 2;
      }
      else {
        cursorX = 0;
      }
    }

    if (yValue > 512 + DEADZONE) {
      if (cursorY > 1) {
        cursorY -= 2;
      }
      else {
        cursorY = 6;
      }
    } else if (yValue < 512 - DEADZONE) {
      if (cursorY < 6) {
        cursorY += 2;
      }
      else {
        cursorY = 0;
      }
    }
  }

  if (clearedCount >= 64)
  {
    gameover();
  }
}
