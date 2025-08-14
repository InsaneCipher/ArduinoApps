/*
  Cross The Road – Arduino Game

  Description:
  This script implements a simple "Cross the Road" style game on an 8x8 LED grid using NeoPixels.
  The player controls a cursor (character) using a joystick to cross rows of moving vehicles.
  Vehicles (cars) spawn at designated spawn points and move across the grid. The goal is to
  reach the opposite side without colliding with vehicles. The game updates the score as the
  player progresses and uses an LCD to display game information.

  Hardware Required:
  - Arduino board (Uno, Mega, etc.)
  - 8x8 NeoPixel LED matrix
  - Joystick module (X-axis to VRX_PIN, Y-axis to VRY_PIN)
  - 16x2 I2C LCD display
  - Power supply for LEDs (if required)
  - Necessary wiring, resistors, and breadboard connections

  Libraries Used:
  - Adafruit_NeoPixel.h  : Controls the 8x8 NeoPixel LED grid
  - LiquidCrystal_I2C.h : Controls the I2C LCD display
  - Wire.h               : I2C communication

  Features:
  - Vehicles spawn randomly with configurable length and type
  - Cursor movement controlled via analog joystick
  - Game speed and spawn rate increase as the player progresses
  - Score tracking displayed on LCD
  - LED grid shows cars, grass, and cursor
  - Visual effects using blinking LEDs on game over
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
#define MOVE_INTERVAL 300

// ========== Objects ==========
Adafruit_NeoPixel strip(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);
LiquidCrystal_I2C lcd(0x27, 16, 2);

// ========== Globals ==========
uint8_t brightness = 5;
unsigned long currentTime;
unsigned long lastMoveTime;
unsigned long vehicleLastMoveTime;
unsigned long vehicleLastSpawnTime;
unsigned long score = 1;
int steps = 0;
int spawnChance = 990;

const int MIN_SPEED_COOLDOWN = 50;
const int MAX_SPEED_COOLDOWN = 300;
const int MAX_SPAWN_COOLDOWN = 500;
const int MIN_SPAWN_COOLDOWN = 100;
int spawnCooldown = MAX_SPAWN_COOLDOWN;
int vehicleMoveCooldown = MAX_SPEED_COOLDOWN;

uint8_t blinks = 0;
uint8_t grid[8][8];

// Define Types
const uint8_t EMPTY = 0;
const uint8_t GRASS = 1;
const uint8_t WATER = 2;
const uint8_t LOG = 3;
const uint8_t SPAWN = 4;
const uint8_t SPAWN_R = 5;
const uint8_t SPAWN_L = 6;
const uint8_t CAR_A = 7;
const uint8_t CAR_B = 8;



// Cursor position
uint8_t cursorX = 4;
uint8_t cursorY = 0;
int xValue = 0;
int yValue = 0;

// ========== Maps ==========
const int G = GRASS;
const int R = SPAWN_R;
const int L = SPAWN_L;

const uint8_t MAP_1[8][8] = {
  {G,G,G,G,G,G,G,G},
  {0,0,0,0,0,0,0,L},
  {G,G,G,G,G,G,G,G},
  {R,0,0,0,0,0,0,0},
  {G,G,G,G,G,G,G,G},
  {R,0,0,0,0,0,0,0},
  {0,0,0,0,0,0,0,L},
  {G,G,G,G,G,G,G,G}
};

const uint8_t MAP_2[8][8] = {
  {G,G,G,G,G,G,G,G},
  {0,0,0,0,0,0,0,L},
  {0,0,0,0,0,0,0,L},
  {G,G,G,G,G,G,G,G},
  {G,G,G,G,G,G,G,G},
  {R,0,0,0,0,0,0,0},
  {R,0,0,0,0,0,0,0},
  {G,G,G,G,G,G,G,G}
};

const uint8_t MAP_3[8][8] = {
  {G,G,G,G,G,G,G,G},
  {R,0,0,0,0,0,0,0},
  {0,0,0,0,0,0,0,L},
  {G,G,G,G,G,G,G,G},
  {G,G,G,G,G,G,G,G},
  {R,0,0,0,0,0,0,0},
  {0,0,0,0,0,0,0,L},
  {G,G,G,G,G,G,G,G}
};

const uint8_t MAP_4[8][8] = {
  {G,G,G,G,G,G,G,G},
  {R,0,0,0,0,0,0,0},
  {0,0,0,0,0,0,0,L},
  {R,0,0,0,0,0,0,0},
  {G,G,G,G,G,G,G,G},
  {R,0,0,0,0,0,0,0},
  {0,0,0,0,0,0,0,L},
  {G,G,G,G,G,G,G,G}
};

const uint8_t MAP_5[8][8] = {
  {G,G,G,G,G,G,G,G},
  {R,0,0,0,0,0,0,0},
  {0,0,0,0,0,0,0,L},
  {G,G,G,G,G,G,G,G},
  {0,0,0,0,0,0,0,L},
  {R,0,0,0,0,0,0,0},
  {0,0,0,0,0,0,0,L},
  {G,G,G,G,G,G,G,G}
};

const uint8_t MAP_6[8][8] = {
  {G,G,G,G,G,G,G,G},
  {R,0,0,0,0,0,0,0},
  {0,0,0,0,0,0,0,L},
  {G,G,G,G,G,G,G,G},
  {G,G,G,G,G,G,G,G},
  {R,0,0,0,0,0,0,0},
  {0,0,0,0,0,0,0,L},
  {G,G,G,G,G,G,G,G}
};

const uint8_t MAP_7[8][8] = {
  {G,G,G,G,G,G,G,G},
  {R,0,0,0,0,0,0,0},
  {0,0,0,0,0,0,0,L},
  {G,G,G,G,G,G,G,G},
  {G,G,G,G,G,G,G,G},
  {R,0,0,0,0,0,0,0},
  {0,0,0,0,0,0,0,L},
  {G,G,G,G,G,G,G,G}
};

const uint8_t MAP_8[8][8] = {
  {G,G,G,G,G,G,G,G},
  {R,0,0,0,0,0,0,0},
  {0,0,0,0,0,0,0,L},
  {R,0,0,0,0,0,0,0},
  {0,0,0,0,0,0,0,L},
  {R,0,0,0,0,0,0,0},
  {0,0,0,0,0,0,0,L},
  {G,G,G,G,G,G,G,G}
};

const uint8_t MAP_9[8][8] = {
  {G,G,G,G,G,G,G,G},
  {R,0,0,0,0,0,0,0},
  {0,0,0,0,0,0,0,L},
  {0,0,0,0,0,0,0,L},
  {R,0,0,0,0,0,0,0},
  {R,0,0,0,0,0,0,0},
  {0,0,0,0,0,0,0,L},
  {G,G,G,G,G,G,G,G}
};


const uint8_t (*MAPS[])[8] = {
  MAP_1,
  MAP_2,
  MAP_3,
  MAP_4,
  MAP_5,
  MAP_6,
  MAP_7,
  MAP_8,
  MAP_9
};

const byte NUM_MAPS = sizeof(MAPS) / sizeof(MAPS[0]);
const uint8_t (*currentMap)[8] = MAPS[0];
int mapIndex = -1;

// ========== Initialization ==========
void initGame() {
  // Clear the global grid
  for (int row = 0; row < 8; row++) {
    for (int col = 0; col < 8; col++) {
      grid[row][col] = EMPTY;
    }
  }

  vehicleMoveCooldown = MAX_SPEED_COOLDOWN;
  spawnCooldown = MAX_SPAWN_COOLDOWN;
  mapIndex = -1;
  score = 0;
  cursorX = 4;
  cursorY = 0;

  setupMap();
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

      // Draw Car A
      if (grid[row][col] == CAR_A) {
        led(row, col, brightness, 0, 0);
      }

      // Draw Car B
      if (grid[row][col] == CAR_B) {
        led(row, col, brightness, 0, brightness);
      }

      // Draw Grass
      if (grid[row][col] == GRASS) {
        led(row, col, 0, brightness/2, 0);
      }

      // Draw Cursor
      led(cursorY, cursorX, brightness*2, brightness*2, 0);
    }
  }

  strip.show();
}

// ========== LCD Display ==========
void startLCD() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Cross The Road");
}

void updateLCD() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Cross The Road");
  lcd.setCursor(0, 1);
  lcd.print("Score: " + String(score));
}


void gameoverLCD() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Gameover!");
  lcd.setCursor(0, 1);
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

void gameover() {
  gameoverLCD();
  blink(4, 400, brightness, 0, 0);
  initGame();
}

void setupMap() {
  if (mapIndex < NUM_MAPS - 1) {
    mapIndex++;
  } else {
    mapIndex = 0;
  }

  currentMap = MAPS[mapIndex];

  for (int row = 0; row < 8; row++) {
    for (int col = 0; col < 8; col++) {
      if (currentMap[row][col] != SPAWN_R && currentMap[row][col] != SPAWN_L) { 
        grid[row][col] = currentMap[row][col];
      } else {
        grid[row][col] = EMPTY;
      }
    }
  }
}

void spawnVehicles() {
  if (currentTime - vehicleLastSpawnTime >= spawnCooldown) {
    for (int row = 0; row < 8; row++) {
      int tile = currentMap[row][0];  // Check start of row for SPAWN_R
      if (tile == SPAWN_R && grid[row][0] == EMPTY && grid[row][1] == EMPTY && grid[row][2] == EMPTY && grid[row][3] == EMPTY && grid[row][4] == EMPTY) {
        if (random(1000) >= spawnChance) {  // Spawn chance
          vehicleLastSpawnTime = currentTime;

          // Car Type
          int chance = random(100);
          int carType = CAR_A;
          if (chance >= 50) {
            carType = CAR_B;
          }

          // Car Length
          int carLen = 2;
          for (int i = 0; i < carLen; i++) {
            if (grid[row][0+i] == EMPTY) {
              grid[row][0+i] = carType; // Fill to the right
            }
          }
        }
      }

      tile = currentMap[row][7]; // Check end of row for SPAWN_L
      if (tile == SPAWN_L && grid[row][7] == EMPTY && grid[row][6] == EMPTY && grid[row][5] == EMPTY && grid[row][4] == EMPTY && grid[row][3] == EMPTY) {
        if (random(1000) >= spawnChance) {
          vehicleLastSpawnTime = currentTime;
          // Car Type
          int chance = random(100);
          int carType = CAR_A;
          if (chance >= 50) {
            carType = CAR_B;
          }

          // Car Length
          int carLen = 2;
          for (int i = 0; i < carLen; i++) {
            if (grid[row][7-i] == EMPTY) {
              grid[row][7-i] = carType; // Fill to the right
            }
          }
        }
      }
    }
  }
}

void moveVehicles() {
  if (currentTime - vehicleLastMoveTime >= vehicleMoveCooldown) {
    vehicleLastMoveTime = currentTime;

    // Spawn R
    for (int row = 0; row < 8; row++) {
      for (int col = 7; col >= 0; col--) {
        int tile = grid[row][col];
        int nextTile = grid[row][col + 1];
        if (currentMap[row][0] == SPAWN_R) {
          if (col >= 7) {
            grid[row][col] = EMPTY;
          } else if (tile != EMPTY) {
            if (row == cursorY && col == cursorX) {
              gameover();
            } else {
              nextTile == EMPTY && col <= 6;
              grid[row][col + 1] = grid[row][col];
              grid[row][col] = EMPTY;
            }
          }
        }
      }
    }

    // Spawn L
    for (int row = 0; row < 8; row++) {
      for (int col = 0; col < 8; col++) {
        int tile = grid[row][col];
        int nextTile = grid[row][col - 1];
        if (currentMap[row][7] == SPAWN_L) {
          if (col <= 0) {
            grid[row][col] = EMPTY;
          } else if (tile != EMPTY) {
            if (row == cursorY && col == cursorX) {
              gameover();
            } else {
              nextTile == EMPTY && col > 0;
              grid[row][col - 1] = grid[row][col];
              grid[row][col] = EMPTY;
            }
          }
        }
      }
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
  yValue = analogRead(VRY_PIN);
  currentTime = millis();
  updateLeds();
  spawnVehicles();
  moveVehicles();

  int tile = grid[cursorY][cursorX];
  if (tile != EMPTY && tile != GRASS) {
    gameover();
  }

  if (cursorY >= 7) {
    cursorY = 0;
    spawnCooldown -= 100;
    if (spawnCooldown < MIN_SPAWN_COOLDOWN) {
      spawnCooldown = MIN_SPAWN_COOLDOWN;
    }

    vehicleMoveCooldown -= 10;
    if (vehicleMoveCooldown < MIN_SPEED_COOLDOWN) {
      vehicleMoveCooldown = MIN_SPEED_COOLDOWN;
    }
    setupMap();
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

    if (yValue > 512 + DEADZONE) {
      if (cursorY > 0) {
        //cursorY--;
      }
    } else if (yValue < 512 - DEADZONE) {
      if (cursorY < 7) {
        cursorY++;
        steps++;
        score += 2*steps;
        updateLCD();
      }
    }
  }
}
