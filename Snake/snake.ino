/*
  Snake Game for Arduino - 8x8 LED Matrix + LCD + Joystick

  Description:
  -------------
  Classic snake game implementation using an 8x8 NeoPixel LED grid for graphics,
  a joystick for input, and a 16x2 I2C LCD display to show the score and messages.
  The snake moves around the grid, eating apples and growing longer.
  If it hits itself, the game ends and restarts after a short delay.

  Hardware Required:
  -------------------
  - Arduino (Uno, Nano, etc.)
  - 8x8 NeoPixel LED Matrix (WS2812 or similar)
  - 16x2 I2C LCD Display
  - Joystick module (analog X/Y + button)
  - Optional: Pushbutton connected to pin 2 (or built-in on joystick)

  Pin Connections:
  -----------------
  - LED Matrix Data Pin: D6
  - Joystick X-axis: A1
  - Joystick Y-axis: A0
  - Joystick Button: D2
  - LCD SDA/SCL: Default I2C pins (A4/A5 on Uno)

  Notes:
  ------
  - Controls: Move using the joystick. Press the button to interact (if needed).
  - Score is shown on the LCD. Snake grows each time it eats an apple.
  - Game resets automatically after a game over.
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
#define BASE_SPEED 400    // Base speed of snake
#define MAX_SPEED 200    // Max speed of snake
#define SPEED_INCREMENT 4    // Max speed of snake

// ========== Objects ==========
Adafruit_NeoPixel strip(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);
LiquidCrystal_I2C lcd(0x27, 16, 2);

// ========== Globals ==========
int xValue = 0, yValue = 0;
int checkJoy = 0;
uint8_t leds[8][8];
uint8_t brightness = 10;
unsigned long lastMoveTime = 0;
unsigned long lastBlinkTime = 0;

// Cursor position
uint8_t cursorX = 4;
uint8_t cursorY = 4;
uint8_t cursorPos = 0;
int move_interval = BASE_SPEED;

const byte EMPTY = 0;
const byte APPLE = 1;
const byte SNAKE = 2;
const byte GAMEOVER = 3;

bool appleExists = false;
int score = 0;
const byte MAX_SNAKE_LENGTH = 64;
uint8_t snakeLength = 1;
bool ateApple = false;
int steps = 0;
uint8_t blinks = 0;

struct Position {
  uint8_t x;
  uint8_t y;
};

// Snake direction
byte dx = 0;
byte dy = 0;
Position snakeBody[MAX_SNAKE_LENGTH];
int headIndex = cursorX + cursorY*8;                // Start of head
int tailIndex = (cursorX + cursorY*8) - 1;                // Start of tail

// ========== Initialization ==========
void initGame() {
  startLCD();
  strip.show();
  score = 0;
  steps = 0;
  snakeLength = 1;
  appleExists = false;
  cursorX = 4;
  cursorY = 4;
  dx = 0;
  dy = 0;
  move_interval = BASE_SPEED;

  snakeBody[0].x = cursorX;
  snakeBody[0].y = cursorY;
  headIndex = 0;
  tailIndex = 0;

  leds[cursorY][cursorX] = SNAKE;
  setupLeds();
  updateLeds();
}

// ========== Setup Functions ==========
void setupLeds() {
  for (int row = 0; row < 8; row++) {
    for (int column = 0; column < 8; column++) {
      leds[row][column] = 0; // Default to empty
    }
  }
}

void startLCD() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Snake Game!");
  lcd.setCursor(0, 1);
  lcd.print("Score: " + String(score));
}

// ========== LED Functions ==========
void led(uint8_t row, uint8_t column, uint8_t r, uint8_t g, uint8_t b)
{
  if ((row % 2) == 0)
  {
    strip.setPixelColor(row * 8 + column, strip.Color(r, g, b));
  }
  else
  {
    strip.setPixelColor(row * 8 + (7 - column), strip.Color(r, g, b));
  }
  //strip.show();
}

void updateLeds()
{
  strip.clear();
  int i = tailIndex;
  while (i != (headIndex + 1) % MAX_SNAKE_LENGTH) {
    Position pos = snakeBody[i];

    if (i == headIndex) {
      // Head (brighter)
      led(pos.y, pos.x, brightness + 4, brightness + 4, 1);
    } else {
      // Body
      led(pos.y, pos.x, 0, brightness, 0);
    }

    i = (i + 1) % MAX_SNAKE_LENGTH;
  }


  // Draw apple
  for (int row = 0; row < 8; row++) {
    for (int col = 0; col < 8; col++) {
      if (leds[row][col] == APPLE) {
        led(row, col, brightness + 2, 0, 0);  // Red apple
      }
    }
  }

  strip.show();
}

// ========== LCD Display ==========
void updateLCD() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Snake Game!");
  lcd.setCursor(0, 1);
  lcd.print("Score: " + String(score));
}

void gameoverLCD() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Gameover!");
  lcd.setCursor(0, 1);
  lcd.print("Score: " + String(score));
  Serial.println("Score: " + String(score));
  delay(2000);
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

void gameover()
{
  blink(4, 500);
  gameoverLCD();
  initGame();
}

void moveSnake() {
  // Calculate new head position
  Position newHead = {
    (snakeBody[headIndex].x + dx + 8) % 8,
    (snakeBody[headIndex].y + dy + 8) % 8
  };

  cursorX = newHead.x;
  cursorY = newHead.y;

  // Check Apple
  ateApple = false;

  if (leds[cursorY][cursorX] == APPLE) {
    leds[cursorY][cursorX] = EMPTY;
    score += 50;
    updateLCD();
    appleExists = false;
    ateApple = true;
    if (move_interval > MAX_SPEED + SPEED_INCREMENT) {
      move_interval -= SPEED_INCREMENT;
    }
  }

  // Check for collision with self
  if (leds[newHead.y][newHead.x] == SNAKE) {
    gameover();
    return;
  }

  // Advance head
  headIndex = (headIndex + 1) % MAX_SNAKE_LENGTH;
  snakeBody[headIndex] = newHead;
  leds[newHead.y][newHead.x] = SNAKE;

  if (!ateApple) {
    Position tail = snakeBody[tailIndex];
    leds[tail.y][tail.x] = EMPTY;
    tailIndex = (tailIndex + 1) % MAX_SNAKE_LENGTH;
  } else {
    snakeLength++;
    blinks = 2;
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
  updateLeds();
  Serial.println("Blinks: " + String(blinks));
  xValue = analogRead(VRX_PIN);
  yValue = analogRead(VRY_PIN);

  // Handle cursor movement
  unsigned long currentTime = millis();

  if (xValue < 512 - DEADZONE) {
    dx = 1;
    dy = 0;
  } else if (xValue > 512 + DEADZONE) {
    dx = -1;
    dy = 0;
  } else if (yValue > 512 + DEADZONE) {
    dx = 0;
    dy = -1;
  } else if (yValue < 512 - DEADZONE) {
    dx = 0;
    dy = 1;
  }

  if (currentTime - lastMoveTime >= move_interval) {
    lastMoveTime = currentTime;
    moveSnake();
  }

  // Blink head
  if (blinks > 0 && currentTime - lastBlinkTime >= 300) {
      lastBlinkTime = currentTime;
      if (blinks % 2 == 0) {
        led(snakeBody[headIndex].y, snakeBody[headIndex].x, 255, 255, 0);
        strip.show();
      }
      blinks--;
  }

  // Spawn Apple
  while (!appleExists) {
    byte ySpawn = random(0, 8);
    byte xSpawn = random(0, 8);

    if (leds[ySpawn][xSpawn] == EMPTY) {
      leds[ySpawn][xSpawn] = APPLE;
      appleExists = true;
    }
  }
}
