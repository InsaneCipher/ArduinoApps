/* 
   Rhythm Tiles Game
   -----------------
   A fast-paced rhythm game where the player uses a joystick to move a 2x2 cursor 
   and "catch" falling tiles before they reach the top. Each caught tile increases 
   your score, and the tiles fall faster as time goes on. Missing tiles reduces your HP. 
   The game ends when your HP reaches zero.

   Hardware Required:
   - Arduino Uno (or compatible board)
   - 8x8 NeoPixel LED matrix
   - Joystick module (X-axis only in current implementation)
   - 16x2 I2C LCD display
   - Power supply capable of running the LED matrix and Arduino
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
uint8_t brightness = 20;
unsigned long currentTime;
unsigned long beginTime = millis();
unsigned long endTime = 0;
unsigned long lastSpawnTime = 0;
unsigned long lastTileMoveTime = 0;
unsigned long lastMoveTime = 0;
unsigned long lastLCDUpdateTime = 0;
unsigned long score = 0;
unsigned long highscore = 0;
uint8_t blinks = 0;
uint8_t grid[8][8];
uint8_t tileCounter = 0;
uint8_t tileLength = 0;
uint8_t maxHP = 200;
uint8_t HP = maxHP;

const uint8_t MIN_SPEED = 50;
const uint8_t SPEED_DECREASE = 10;
const uint8_t BASE_TILE_SPEED = 500;
int tileMoveSpeed = BASE_TILE_SPEED;

// Define Types
const uint8_t EMPTY = 0;
const uint8_t TILE_A = 1;
const uint8_t TILE_B = 2;
const uint8_t TILE_C = 3;

// Cursor position
uint8_t cursorX = 0;
uint8_t cursorY = 0;
int xValue = 0;

// ========== Initialization ==========
void initGame() {
  // Clear the global grid
  for (int row = 0; row < 8; row++) {
    for (int col = 0; col < 8; col++) {
      grid[row][col] = EMPTY;
    }
  }

  score = 0;
  tileMoveSpeed = BASE_TILE_SPEED;
  HP = maxHP;
  beginTime = millis();
  lastTileMoveTime = 0;
  lastSpawnTime = 0;
  tileCounter = 0;
  tileLength = 0;

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
        led(row, col, 0, brightness, brightness/2);
      }

      // Draw Tile A
      if (grid[row][col] == TILE_A) {
        led(row, col, brightness, 0, brightness/2);
      }

      // Draw Tile B
      if (grid[row][col] == TILE_B) {
        led(row, col, brightness, 0, 0);
      }

      // Draw Tile C
      if (grid[row][col] == TILE_C) {
        led(row, col, brightness*3, brightness/2.5, 0);
      }

      // Draw Cursor
      led(cursorY, cursorX, 0, brightness, 0);
      led(cursorY + 1, cursorX, 0, brightness, 0);
      led(cursorY, cursorX + 1, 0, brightness, 0);
      led(cursorY + 1, cursorX + 1, 0, brightness, 0);
    }
  }

  strip.show();
}

// ========== LCD Display ==========
void startLCD() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Rhythm Tiles");
  lcd.setCursor(0, 1);
  lcd.print("Score: " + String(score));
}

void updateLCD() {
  lcd.setCursor(0, 0);
  lcd.print("Score: ");
  lcd.print(score);
  lcd.print("     "); // pad with spaces to erase old digits

  lcd.setCursor(0, 1);
  unsigned int time = (currentTime - beginTime) / 1000;
  String whitespace = "";
  if (HP/2 < 100) {
    whitespace += " ";
    if (HP/2 < 10) {
      whitespace += " ";
    }
  }


  if (time < 10000) {
    whitespace += " ";
    if (time < 1000) {
      whitespace += " ";
      if (time < 100) {
        whitespace += " ";
        if (time < 10) {
          whitespace += " ";
        }
      }
    }
  }

  lcd.print("T: ");
  lcd.print(time);
  lcd.print("s " + whitespace);

  lcd.print("HP:");
  lcd.print(HP / 2);
  lcd.print("   "); // pad with spaces
}


void gameoverLCD() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Gameover!");
  delay(2000);

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Time Survived:");
  lcd.setCursor(0, 1);
  float time = (endTime - beginTime) / 100.0;
  lcd.print(String(time) + " Seconds");
  delay(2000);

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Score: ");
  lcd.setCursor(0, 1);
  lcd.print(String(score));
  delay(2000);

  lcd.clear();
  lcd.setCursor(0, 0);

  if (score <= highscore) {
    lcd.print("Highscore: ");
  } else {
    lcd.print("New Highscore!!!");
    highscore = score;
  }

  lcd.setCursor(0, 1);
  lcd.print(String(highscore));
  delay(2000);
}

void blink(uint8_t times, uint8_t delayTime) {
  for (int i = 0; i < times; i++) {
    strip.clear();
    for (int row = 0; row < 8; row++) {
      for (int column = 0; column < 8; column++) {
        led(row, column, brightness, 0, 0);
      }
    }
    strip.show();
    delay(delayTime);
    strip.clear();
    strip.show();
    delay(delayTime);
  }
}


void spawnTiles() {
  static uint8_t col = 0;
  static uint8_t tileType = 0;

  // Spawn New Tiles
  if (tileCounter >= tileLength && currentTime - lastSpawnTime >= 1000) {
    lastSpawnTime = currentTime;

    if (tileMoveSpeed >= MIN_SPEED) {
      tileMoveSpeed -= SPEED_DECREASE;
    }

    tileType = random(TILE_A, TILE_C+1);

    if (tileType == TILE_A) {
      tileLength = random(4,6)*2;
    }

    if (tileType == TILE_B) {
      tileLength = random(2,3)*2;
    }

    if (tileType == TILE_C) {
      tileLength = 2;
    }

    col = random(0, 3) * 2;
    tileCounter = 0; 
  }

  // Spawn Tiles
  if (tileCounter < tileLength) {
    if (grid[7][col] == EMPTY) {
      grid[7][col] = tileType;
      grid[7][col+1] = tileType;
      tileCounter++;
      lastSpawnTime = currentTime;
    }
  }
}

void moveTiles() {
  // Move Tiles
  if (currentTime - lastTileMoveTime >= tileMoveSpeed) {
    lastTileMoveTime = currentTime;
    for (int row = 0; row < 8; row++) {
      for (int col = 0; col < 8; col++) {
        uint8_t tile = grid[row][col];

        if (tile == TILE_A || tile == TILE_B || tile == TILE_C) {
          if (row > 0) {
            grid[row - 1][col] = tile;
            grid[row][col] = EMPTY;
          } else {
            grid[row][col] = EMPTY;
            HP -= 1;
            updateLCD();
          }
        }
      }
    }
  }

  int timeMP = (currentTime - beginTime) / 1000;

  if (grid[cursorY][cursorX] == TILE_A || grid[cursorY][cursorX] == TILE_B || grid[cursorY][cursorX] == TILE_C) {
    uint8_t tileType = grid[cursorY][cursorX];
    grid[cursorY][cursorX] = EMPTY;
    grid[cursorY + 1][cursorX] = EMPTY;
    grid[cursorY][cursorX + 1] = EMPTY;
    grid[cursorY + 1][cursorX + 1] = EMPTY;
    score += (2*tileType)*timeMP;
    updateLCD();
  }

  if (grid[cursorY + 1][cursorX] == TILE_A || grid[cursorY + 1][cursorX] == TILE_B || grid[cursorY + 1][cursorX] == TILE_C) {
    uint8_t tileType = grid[cursorY + 1][cursorX];
    grid[cursorY + 1][cursorX] = EMPTY;
    grid[cursorY + 1][cursorX + 1] = EMPTY;
    score += (1*tileType)*timeMP;
    updateLCD();
  }
}

// ========== Game Logic ==========

void gameover()
{
  endTime = currentTime;
  blink(4, 500);
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
  xValue = analogRead(VRX_PIN);
  currentTime = millis();

  updateLeds();
  spawnTiles();
  moveTiles();

  // Death
  if (HP <= 0) {
    gameover();
  }

  // Update LCD
  if (currentTime - lastLCDUpdateTime >= 1000) {
    lastLCDUpdateTime = currentTime;
    updateLCD();
  }

  // Handle cursor movement
  unsigned long currentTime = millis();
  if (currentTime - lastMoveTime >= MOVE_INTERVAL) {
    lastMoveTime = currentTime;

    if (xValue > 512 + DEADZONE) {
      if (cursorX > 1) {
        cursorX -= 2;
      } else {
        //cursorX = 6;
      }
    } else if (xValue < 512 - DEADZONE) {
      if (cursorX < 6) {
        cursorX += 2;
      } else {
        //cursorX = 0;
      }
    }

    if (xValue < 512 + DEADZONE && xValue > 512 - DEADZONE) {
      lastMoveTime = 0;
    }
  }
}
