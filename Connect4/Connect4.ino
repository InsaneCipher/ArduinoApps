/*
 * Connect 4 on Arduino with NeoPixel LED Matrix and I2C LCD
 * ---------------------------------------------------------
 * Hardware Requirements:
 * - Arduino Uno/Nano or compatible board
 * - 8x8 WS2812B (NeoPixel) LED matrix for game board display
 * - Joystick module (VRX -> A1, VRY -> A0) for cursor movement
 * - Push button (built into joystick or separate) for placing pieces
 * - 16x2 I2C LCD (address 0x27) for game messages
 * - Power supply capable of driving 64 NeoPixels (5V, ~2A recommended)
 * - Optional: Serial monitor for debug output
 *
 * Libraries Used:
 * - Adafruit_NeoPixel (control WS2812B LEDs)
 * - LiquidCrystal_I2C (control 16x2 LCD)
 * - Wire (I2C communication)
 *
 * Description:
 * - Implements a 2-player Connect 4 game.
 * - Players take turns moving a cursor left/right using the joystick and 
 *   dropping pieces into the LED matrix grid.
 * - The game detects vertical, horizontal, and diagonal wins.
 * - LCD displays the game title and win/draw messages.
 * - NeoPixel LEDs show the game board, cursor highlight, and blinking win/draw animations.
 * - Includes gravity simulation so pieces "fall" to the lowest empty position.
 * - Resets automatically after win or draw.
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
#define DEADZONE 100        // Joystick deadzone
#define BLINK_DELAY 500   // Delay between blinks
#define MOVE_INTERVAL 200

// ========== Objects ==========
Adafruit_NeoPixel strip(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);
LiquidCrystal_I2C lcd(0x27, 16, 2);

// ========== Globals ==========
uint8_t brightness = 5;
unsigned long currentTime;
unsigned long lastMoveTime;
unsigned long lastDropTime;
uint8_t dropCooldown = 100;
uint8_t blinks = 0;
uint8_t grid[8][8];

// Define Types
const uint8_t EMPTY = 0;
const uint8_t A = 1;
const uint8_t B = 2;
const uint8_t H = 0;
const uint8_t V = 1;
const uint8_t DownH = 2;
const uint8_t UpH = 3;
uint8_t winner = A;
uint8_t winRow = 0;
uint8_t winCol = 0;
uint8_t winDir = H;

uint8_t turn = A;
uint8_t placed = 0;

// Cursor position
uint8_t cursorX = 0;
uint8_t cursorY = 7;
int xValue = 0;
bool checkJoy = false;


// ========== Initialization ==========
void initGame() {
  // Clear the global grid
  for (int row = 0; row < 8; row++) {
    for (int col = 0; col < 8; col++) {
      grid[row][col] = EMPTY;
    }
  }

  placed = 0;

  startLCD();
  strip.show();
  updateLeds();
}

void CheckFreeMemory() {
  extern int __heap_start, *__brkval;
  int v;
  int freeMem = (int) &v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval);
  Serial.print("Free memory: ");
  Serial.println(freeMem);
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

      // Draw Empty
      if (grid[row][col] == EMPTY) {
        led(row, col, 0, 0, 0);
      }

      // Draw A
      if (grid[row][col] == A) {
        led(row, col, brightness, 0, 0);
      }

      // Draw B
      if (grid[row][col] == B) {
        led(row, col, brightness, brightness/2, 0);
      }

      // Draw Cursor
      if (turn == A) {
        led(cursorY, cursorX, brightness*8, 0, 0);
      } else {
        led(cursorY, cursorX, brightness*6, brightness*3, 0);
      }
    }
  }

  strip.show();
}

// ========== LCD Display ==========
void startLCD() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Connect 4");
}

void updateLCD() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Connect 4");
}


void gameoverLCD(bool win) {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Gameover!");
  lcd.setCursor(0, 1);
  if (win) {
    lcd.print("Player " + String(winner) + " Wins!");
  } else {
    lcd.print("Draw!");
  }
}

void blink(uint8_t times, uint8_t delayTime) {
  for (int i = 0; i < times; i++) {
    strip.clear();
    for (int row = 0; row < 8; row++) {
      for (int column = 0; column < 8; column++) {
        led(row, column, 0, brightness, 0);
      }
    }
    strip.show();
    delay(delayTime);
    strip.clear();
    strip.show();
    delay(delayTime);
  }
}

void blinkWin(uint8_t delayTime) {

  // Winner Colour
  int red = brightness;
  int green = 0;
  if (winner == B) {
    red = brightness;
    green = brightness/2;
  }

  for (int i = 0; i < 4; i++) {
    strip.clear();
    if (winDir == V) {
      led(winRow, winCol, red, green, 0);
      led(winRow+1, winCol, red, green, 0);
      led(winRow+2, winCol, red, green, 0);
      led(winRow+3, winCol, red, green, 0);
    }

    if (winDir == H) {
      led(winRow, winCol, red, green, 0);
      led(winRow, winCol+1, red, green, 0);
      led(winRow, winCol+2, red, green, 0);
      led(winRow, winCol+3, red, green, 0);
    }

    if (winDir == UpH) {
      led(winRow, winCol, red, green, 0);
      led(winRow+1, winCol+1, red, green, 0);
      led(winRow+2, winCol+2, red, green, 0);
      led(winRow+3, winCol+3, red, green, 0);
    }

    if (winDir == DownH) {
      led(winRow, winCol, red, green, 0);
      led(winRow-1, winCol-1, red, green, 0);
      led(winRow-2, winCol-2, red, green, 0);
      led(winRow-3, winCol-3, red, green, 0);
    }
    strip.show();
    delay(delayTime);
    strip.clear();
    strip.show();
    delay(delayTime);
  }
}


void blinkDraw(uint8_t delayTime) {
  for (int i = 0; i < 4; i++) {
    updateLeds();
    delay(delayTime);
    strip.clear();
    strip.show();
    delay(delayTime);
  }
}

// ========== Game Logic ==========

void gameover(bool win)
{
  if (win) {
    // Check Winner
    if (turn == A) {
      winner = B;
    } else {
      winner = A;
    }

    gameoverLCD(win);
    blinkWin(200);
  } else {
    gameoverLCD(win);
    blinkDraw(200);
  }
  initGame();
}

void movePiece() {
  if (currentTime - lastDropTime >= dropCooldown) {
    for (int row = 0; row < 8; row++) {
      for (int col = 0; col < 8; col++) {
        if (row > 0 && grid[row-1][col] == EMPTY) {
          grid[row-1][col] = grid[row][col];
          grid[row][col] = EMPTY;
        }
      }
    }
  }
}

bool checkFallen() {
  for (int row = 0; row < 8; row++) {
    for (int col = 0; col < 8; col++) {
      if (row > 0 && grid[row][col] != EMPTY && grid[row-1][col] == EMPTY) {
        return false;
      }
    }
  }

  return true;
}

void checkWin() {
  if (checkFallen()) {
    for (int row = 0; row < 8; row++) {
      for (int col = 0; col < 8; col++) {
        int p = grid[row][col];

        // Only check pieces
        if (p != A && p != B) continue;

        // Vertical Check
        if (row <= 4) { 
          if (grid[row+1][col] == p && grid[row+2][col] == p && grid[row+3][col] == p) {
            winRow = row;
            winCol = col;
            winDir = V;
            gameover(true);
          }
        }

        // Horizontal Check
        if (col <= 4) { 
          if (grid[row][col+1] == p && grid[row][col+2] == p && grid[row][col+3] == p) {
            winRow = row;
            winCol = col;
            winDir = H;
            gameover(true);
          }
        }

        // Diagonal Up Check
        if (col <= 4 && row <= 4) { 
          if (grid[row+1][col+1] == p && grid[row+2][col+2] == p && grid[row+3][col+3] == p) {
            winRow = row;
            winCol = col;
            winDir = UpH;
            gameover(true);
          }
        }

        // Diagonal Down Check
        if (col >= 4 && row >= 4) { 
          if (grid[row-1][col-1] == p && grid[row-2][col-2] == p && grid[row-3][col-3] == p) {
            winRow = row;
            winCol = col;
            winDir = DownH;
            gameover(true);
          }
        }
      }
    }

    // Check for draw
    if (placed >= 64) {
      gameover(false);
    }
  }
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
  xValue = analogRead(VRX_PIN);
  int buttonState = digitalRead(2);
  currentTime = millis();
  updateLeds();
  movePiece();
  checkWin();


  // Handle shooting
  if (buttonState == LOW && !checkJoy) {
    checkJoy = true;
    Serial.println("Button Pressed");

    if (grid[cursorY][cursorX] == EMPTY) {
      grid[cursorY][cursorX] = turn;

       placed++;

      // Change turns
      if (turn == A) {
        turn = B;
      } else {
        turn = A;
      }

      updateLeds();
      updateLCD();
    }
  } else if (buttonState == HIGH && checkJoy == 1) {
    checkJoy = false;
  }

  // Handle cursor movement
  unsigned long currentTime = millis();
  if (currentTime - lastMoveTime >= MOVE_INTERVAL) {
    lastMoveTime = currentTime;

    if (xValue > 512 + DEADZONE) {
      if (cursorX > 0) {
        cursorX--;
      }
    } else if (xValue < 512 - DEADZONE) {
      if (cursorX < 7) {
        cursorX++;
      }
    }
  }
}
