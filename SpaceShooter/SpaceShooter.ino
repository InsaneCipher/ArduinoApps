/*
  Simple 8x8 LED Grid Shooter Game
  --------------------------------
  A basic arcade-style shooting game using an 8x8 NeoPixel LED matrix and a joystick.
  The player moves a cursor and shoots upwards to destroy enemies.

  Hardware Required:
  - Arduino-compatible board
  - 8x8 WS2812 (NeoPixel) LED matrix
  - Joystick module (analog X and Y)
  - I2C LCD display (optional, for score display)
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
#define MOVE_INTERVAL 100
#define RELOAD_TIME 300

// ========== Objects ==========
Adafruit_NeoPixel strip(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);
LiquidCrystal_I2C lcd(0x27, 16, 2);

// ========== Globals ==========
int xValue = 0;
int checkJoy = 0;
uint8_t brightness = 20;
unsigned long currentTime;
unsigned long lastMoveTime = 0;
unsigned long lastShootMoveTime = 0;
unsigned long lastShotTime = 0;
unsigned long lastEnemyShotTime = 0;
int baseEnemyShootDelay = 1000;
int enemyShootDelay = baseEnemyShootDelay;
unsigned long score = 0;
int enemies = 0;
int kills = 0;
int wave = 0;

// Cursor position
uint8_t cursorX = 4;

const uint8_t EMPTY = 0;
const uint8_t BULLET = 1;
const uint8_t ENEMY_BULLET = 2;
const uint8_t ENEMY_A = 3;
const uint8_t ENEMY_B = 4;
const uint8_t ENEMY_C = 5;
const uint8_t ENEMY_D = 6;
const uint8_t CURSOR = 7;

uint8_t blinks = 0;

uint8_t grid[8][8];


// ========== Enemy Waves ==========
const int A = ENEMY_A;
const int B = ENEMY_B;
const int C = ENEMY_C;
const int D = ENEMY_D;

const uint8_t WAVE_1[3][8] = {
  {C,C,C,C,C,C,C,C},
  {A,0,0,A,A,0,0,A},
  {C,A,A,C,C,A,A,C}
};

const uint8_t WAVE_2[3][8] = {
  {A,0,A,0,A,0,A,0},
  {C,0,C,0,C,0,C,0},
  {A,0,C,0,C,0,A,0}
};

const uint8_t WAVE_3[3][8] = {
  {C,A,C,A,C,A,C,A},
  {A,0,A,0,A,0,A,0},
  {B,B,B,B,B,B,B,B}
};

const uint8_t WAVE_4[3][8] = {
  {B,C,B,0,0,B,C,B},
  {D,A,D,0,0,D,A,D},
  {A,B,A,0,0,A,B,A}
};

const uint8_t WAVE_5[3][8] = {
  {C,A,C,0,0,C,A,C},
  {A,D,A,B,B,A,D,A},
  {0,0,0,0,0,0,0,0}
};

const uint8_t WAVE_6[3][8] = {
  {0,A,D,D,D,D,A,0},
  {0,0,A,D,D,A,0,0},
  {0,0,0,A,A,0,0,0}
};

const uint8_t WAVE_7[3][8] = {
  {A,B,A,B,B,A,B,A},
  {B,A,B,A,A,B,A,B},
  {A,B,A,B,B,A,B,A}
};

const uint8_t WAVE_8[3][8] = {
  {C,0,0,0,0,0,0,C},
  {B,C,0,0,0,0,C,B},
  {B,B,C,0,0,C,B,B}
};

const uint8_t WAVE_9[3][8] = {
  {D,A,D,A,D,A,D,A},
  {A,A,A,A,A,A,A,A},
  {0,0,0,0,0,0,0,0}
};

const uint8_t WAVE_10[3][8] = {
  {B,B,B,B,B,B,B,B},
  {A,A,A,A,A,A,A,A},
  {C,B,C,B,C,B,C,B}
};


const uint8_t (*WAVES[])[8] = {
  WAVE_1,
  WAVE_2,
  WAVE_3,
  WAVE_4,
  WAVE_5,
  WAVE_6,
  WAVE_7,
  WAVE_8,
  WAVE_9,
  WAVE_10
};

const byte NUM_WAVES = sizeof(WAVES) / sizeof(WAVES[0]);


// ========== Initialization ==========
void initGame() {
  // Clear the global grid
  for (int row = 0; row < 8; row++) {
    for (int col = 0; col < 8; col++) {
      grid[row][col] = EMPTY;
    }
  }
  enemies = 0;
  score = 0;
  kills = 0;
  startLCD();
  strip.show();
  score = 0;
  wave = 0;
  cursorX = 4;
  updateLeds();
}

void freeMemory() {
  extern int __heap_start, *__brkval;
  int v;
  int freeMem = (int) &v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval);
  Serial.print("Free memory: ");
  Serial.println(freeMem);
}

// ========== Setup Functions ==========

void startLCD() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Space Shooter");
  lcd.setCursor(0, 1);
  lcd.print("Score: " + String(score));
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
      // Draw Enemy
      if (grid[row][col] == ENEMY_A) {
        led(row, col, 0, brightness, 0);
      }

      if (grid[row][col] == ENEMY_B) {
        led(row, col, 0, 0, brightness);
      }

      if (grid[row][col] == ENEMY_C) {
        led(row, col, brightness, 0, brightness);
      }

      if (grid[row][col] == ENEMY_D) {
        led(row, col, brightness*3, brightness/2.5, 0);
      }

      if (grid[row][col] == ENEMY_BULLET) {
        led(row, col, brightness*4, 0, 0);
      }

      // Draw Cursor
      if (col == cursorX && row == 0) {
        led(row, col, 0, brightness, 0);
      }

      // Draw Bullet
      if (grid[row][col] == BULLET) {
        led(row, col, brightness + 1, brightness, 0);
      }
    }
  }

  strip.show();
}

// ========== LCD Display ==========
void updateLCD() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Score: " + String(score));
  lcd.setCursor(0, 1);
  String whitespace = "    ";
  if (wave < 10) {
    whitespace += " ";
  }

  if (kills < 100) {
    if (kills < 10) {
      whitespace += " ";
    }
    whitespace += " ";
  }
  
  // lcd.print("K: " + String(kills) + whitespace + "W: " + String(wave));
  lcd.print("Wave: " + String(wave) + whitespace + String(kills) + "K");
}

void gameoverLCD() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Gameover!");
  lcd.setCursor(0, 1);
  lcd.print("Died Wave: " + String(wave));
  delay(2000);

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Kills: " + String(kills) + "/" + String(enemies));
  lcd.setCursor(0, 1);
  lcd.print("Score: " + String(score));
  delay(2000);

  Serial.println("Score: " + String(score));
  initGame();
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

void setupTargets() {
  int waveIndex = random(NUM_WAVES);
  const uint8_t (*wave)[8] = WAVES[waveIndex];

  for (int row = 0; row < 3; row++) {
    for (int col = 0; col < 8; col++) {
      if (wave[row][col] != 0) {
        if (wave[row][col] == ENEMY_C) {
          enemies += 2;
        } else if (wave[row][col] == ENEMY_D) {
          enemies += 3;
        } else {
          enemies++;
        }
      }

      grid[7 - row][col] = wave[row][col];
    }
  }

  updateLCD();
}


void advanceBullet() {
  if (currentTime - lastShootMoveTime >= 100) {
    lastShootMoveTime = currentTime;
    for (int row = 7; row >= 0; row--) {
      for (int col = 0; col < 8; col++) {
        if (grid[row][col] == BULLET) {
          // Move bullet up
          if (row > 0) {
            int abovePos = grid[row + 1][col];

            // Destroy Bullet
            if (row == 7 && grid[row][col] == BULLET) {
              grid[row][col] = EMPTY;
            }
            // Move Bullet
            if (grid[row + 1][col] == EMPTY) {
              grid[row + 1][col] = BULLET;
              grid[row][col] = EMPTY;
            } else if (abovePos == ENEMY_A || abovePos == ENEMY_B || abovePos == ENEMY_C || abovePos == ENEMY_D) {
              // Add score
              if (abovePos == ENEMY_A) {
                score += 10*wave;

                // Destroy enemy and bullet
                grid[row + 1][col] = EMPTY;
                grid[row][col] = EMPTY;
              }
              if (abovePos == ENEMY_B) {
                score += 20*wave;

                // Destroy enemy and bullet
                grid[row + 1][col] = EMPTY;
                grid[row][col] = EMPTY;
              }
              if (abovePos == ENEMY_C) {
                score += 50*wave;

                // Destroy enemy and bullet
                grid[row + 1][col] = ENEMY_B;
                grid[row][col] = EMPTY;
              }
              if (abovePos == ENEMY_D) {
                score += 100*wave;

                // Destroy enemy and bullet
                grid[row + 1][col] = ENEMY_C;
                grid[row][col] = EMPTY;
              }
              kills++;

              updateLCD();
            } else if (abovePos == ENEMY_BULLET) {
              grid[row + 1][col] = EMPTY;
              grid[row][col] = EMPTY;
              score += 1*wave;
            }
          } else {
            // Remove bullet if it reached the top
            grid[row][col] = EMPTY;
          }
        }
      }
    }

    // Move enemy bullets DOWN — from bottom to top
    for (int row = 0; row < 8; row++) {
      for (int col = 0; col < 8; col++) {
        if (grid[row][col] == ENEMY_BULLET) {
          if (grid[row - 1][col] == EMPTY) {
            grid[row - 1][col] = ENEMY_BULLET;
            grid[row][col] = EMPTY;
          } else if ((row - 1 == 0 && col == cursorX) || (row == 0 && col == cursorX)) {
            gameover();
          } else {
            grid[row][col] = EMPTY; // Hit something or reached bottom
          }
        }
      }
    }

    // Clean up bullets at the bottom row
    for (int col = 0; col < 8; col++) {
      if (grid[7][col] == ENEMY_BULLET) {
        grid[7][col] = EMPTY;
      }
    }
  }
}

bool hasEnemyBelow(int r, int c) {
  for (int row = r - 1; row >= 3; row--) {
    if (grid[row][c] != EMPTY) {
      return true;
    }
  }
  return false;
}

void enemyShoot() {  
  if (currentTime - lastEnemyShotTime >= enemyShootDelay) {
    for (int row = 0; row <= 7; row++) {
      for (int col = 0; col < 8; col++) {
        uint8_t slot = grid[row][col];
        uint8_t fireChance = random(100);

        if (fireChance >= 50) {
          if ((slot == ENEMY_B || slot == ENEMY_C || slot == ENEMY_D) && !hasEnemyBelow(row, col)) {
            if (row > 0 && grid[row - 1][col] == EMPTY) {
              grid[row - 1][col] = ENEMY_BULLET;
            }
          }
        }
      }
    }
    lastEnemyShotTime = currentTime;
  }
}


// ========== Game Logic ==========

void gameover()
{
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
  advanceBullet();
  updateLeds();
  enemyShoot();
  xValue = analogRead(VRX_PIN);

  if (kills >= enemies) {
    setupTargets();
    wave++;

    if (wave > 99) {
      gameover();
    }

    enemyShootDelay = baseEnemyShootDelay -= (50*wave);
    if (enemyShootDelay < 200) {
      enemyShootDelay = 200;
    }
  }

  // Handle cursor movement
  currentTime = millis();
  if (currentTime - lastMoveTime >= MOVE_INTERVAL) {
    lastMoveTime = currentTime;

    if (xValue < (512 - DEADZONE) && cursorX < 7) {
      cursorX++; 
    }
    else if (xValue > (512 + DEADZONE) && cursorX > 0) {
      cursorX--;
    }
  }

  // Handle shooting
  if (currentTime - lastShotTime >= RELOAD_TIME && grid[1][cursorX] == EMPTY) {
    lastShotTime = currentTime;
    grid[1][cursorX] = BULLET;
  }
}
