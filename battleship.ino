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
#define MOVE_INTERVAL 200   // Cursor update interval (ms)

const uint8_t EMPTY = 0;
const uint8_t TARGET = 1;
const uint8_t HIT = 2;

unsigned long beginTime = millis();
unsigned long endTime = millis();

// ========== Objects ==========
Adafruit_NeoPixel strip(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);
LiquidCrystal_I2C lcd(0x27, 16, 2);

// ========== Globals ==========
int xValue = 0, yValue = 0;
int checkJoy = 0;
int maxAmmo = 32;
int ammo = maxAmmo;
int kills = 0;
int targets = 0;
int basescore = 1000;
const uint8_t maxTargets = 5;

byte grid[64];
uint8_t leds[8][8];
uint8_t brightness = 3;

String xDir = "NONE";
String yDir = "NONE";

unsigned long lastMoveTime = 0;

uint8_t cursorX = 4;
uint8_t cursorY = 4;
uint8_t cursorPos = 0;

// ========== Data Structures ==========
struct Ship {
  uint8_t part1;
  uint8_t part2;
  uint8_t part3;
  uint8_t part4;
  bool destroyed;
  uint8_t size;
};

Ship ships[maxTargets];  // Max 5 ships

// ========== Initialization ==========
void initGame() {
  ammo = maxAmmo;
  kills = 0;
  beginTime = millis();
  lcd.begin();
  startLCD();

  strip.begin();
  strip.show();

  setupLeds();
  updateLeds();
  setupTargets();
}

// ========== Setup Functions ==========
void setupLeds() {
  for (int row = 0; row < 8; row++) {
    for (int column = 0; column < 8; column++) {
      leds[row][column] = 1; // Default to empty
    }
  }
}

void startLCD() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Battleship");
  lcd.setCursor(0, 1);
  lcd.print("Click to shoot!");
}

void setupTargets() {
  for (int i = 0; i < 64; i++) grid[i] = EMPTY;

  int rowSize = 8;
  targets = 0;

  while (targets < maxTargets) {
    int y = random(0, rowSize);
    int x = random(0, rowSize - 2);
    uint8_t shipChance = random(1, 100);
    uint8_t shipSize = 4;
    if (shipChance <= 40) {
      if (shipChance <= 20) {
        shipSize = 2;
      } else {
        shipSize = 3;
      }
    }

    
    if (x > rowSize - shipSize) continue;  // avoid wrap-around
    
    int index = y * rowSize + x;

    // Check if the ship can be placed
    bool canPlace = true;
    for (int i = 0; i < shipSize; i++) {
      if (grid[index + i] != EMPTY) {
        canPlace = false;
        break;
      }
    }
    
    // Place the ship
    if (canPlace) {
      for (int i = 0; i < shipSize; i++) {
        grid[index + i] = TARGET;
      }

      // Store ship info
      ships[targets].part1 = index;
      ships[targets].part2 = (shipSize >= 2) ? index + 1 : 255; // 255 = invalid
      ships[targets].part3 = (shipSize >= 3) ? index + 2 : 255;
      ships[targets].part4 = (shipSize == 4) ? index + 3 : 255;
      ships[targets].destroyed = false;
      ships[targets].size = shipSize;

      targets++;
    }
  }

  Serial.println("Hello");
  for (int i = 0; i < 64; i++) {
    Serial.print(i);
    Serial.print(": ");
    Serial.println(grid[i] == TARGET ? "TARGET" : "EMPTY");
  }
}

// ========== LED Functions ==========
void updateLeds()
{
  strip.clear();
  for (int row = 0; row < 8; row++)
    {
      for (int column = 0; column < 8; column++)
        {
          if ((row % 2) == 0)  // -------------------- even rows
          {
            if (cursorX == column && cursorY == row)
            {
              if (leds[row][column] == 2)  // 2 = hit + cursor -> orange
              {
                strip.setPixelColor(row * 8 + column, strip.Color(brightness + 10, brightness, 0));
              }
              else if (leds[row][column] == 0)  // 3 = missed + cursor -> green
              {
                strip.setPixelColor(row * 8 + column, strip.Color(0, brightness, 0));
                
              }
              else  // cursor -> yellow
              {
                strip.setPixelColor(row * 8 + column, strip.Color(brightness, brightness, 0));
              }
            }
            else if (leds[row][column] == 0)  // 0 = missed -> black
            {
              strip.setPixelColor(row * 8 + column, strip.Color(0, 0, 0));
            }
            else if (leds[row][column] == 1)  // 1 = empty -> blue
            {
              strip.setPixelColor(row * 8 + column, strip.Color(0, 0, brightness));
            }
            else if (leds[row][column] == 2)  // 2 = hit -> red
            {
              strip.setPixelColor(row * 8 + column, strip.Color(brightness, 0, 0));
            }
          }
          else  // ------------------------ odd rows
          {
            if (cursorX == column && cursorY == row)
            {
              if (leds[row][column] == 2)  // 2 = hit + cursor -> orange
              {
                strip.setPixelColor(row * 8 + (7 - column), strip.Color(brightness + 10, brightness, 0));
              }
              else if (leds[row][column] == 0)  // 0 = missed + cursor -> green
              {
                strip.setPixelColor(row * 8 + (7 - column), strip.Color(0, brightness, 0));
              }
              else  // cursor -> yellow
              {
                strip.setPixelColor(row * 8 + (7 - column), strip.Color(brightness, brightness, 0));
              }
            }
            else if (leds[row][column] == 0)
              {
                strip.setPixelColor(row * 8 + (7 - column), strip.Color(0, 0, 0));
              }
            else if (leds[row][column] == 1)  // 1 = empty -> blue
            {
              strip.setPixelColor(row * 8 + (7 - column), strip.Color(0, 0, brightness));
            }
            else if (leds[row][column] == 2)  // 2 = hit -> red
            {
              strip.setPixelColor(row * 8 + (7 - column), strip.Color(brightness, 0, 0));
            }
          }
        }
    }

  strip.show();
}

void moveCursor(String xdir, String ydir) {
  if (xdir == "LEFT" && cursorX > 0) {
    cursorX--;
  } else if (xdir == "RIGHT" && cursorX < 7) {
    cursorX++;
  }

  if (ydir == "UP" && cursorY < 7) {
    cursorY++;
  } else if (ydir == "DOWN" && cursorY > 0) {
    cursorY--;
  }

  cursorPos = cursorX + cursorY * 8;
} 

// ========== LCD Display ==========
void updateLCD(int ammo, uint8_t result) {
  lcd.clear();
  lcd.setCursor(0, 0);

  int k_len = String(kills).length();
  int t_len = String(targets).length();
  String whitespace = "  ";
  if (k_len <= 1) whitespace += " ";
  if (t_len <= 1) whitespace += " ";
  if (ammo <= 9) whitespace += " ";

  lcd.print("A:" + String(ammo) + "/" + String(maxAmmo) + whitespace + "K:" + String(kills) + "/" + String(targets));

  lcd.setCursor(0, 1);
  if (result == TARGET)      lcd.print("Ship Hit!");
  else if (result == EMPTY)  lcd.print("Shot Missed!");
  else if (result == HIT)    lcd.print("Already Hit!");
}

void gameoverLCD(bool success) {
  endTime = millis();
  unsigned int timescore = (endTime - beginTime) / 10;
  unsigned int timeRaw = (endTime - beginTime) / 10;
  float time = timeRaw / 100.0;
  Serial.println("Time: " + String(time) + ", Score: " + String(timescore));
  int score = (basescore * ammo) + (kills * 500) + timescore;
  
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Gameover!");
  lcd.setCursor(0, 1);
  lcd.print(success ? "You WIN :)" : "You LOST :(");
  delay(2000);

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Time Taken: ");
  lcd.setCursor(0, 1);
  lcd.print(String(time) + "s");
  delay(2000);

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Your Score: ");
  lcd.setCursor(0, 1);
  lcd.print(String(score));
  delay(2000);
  initGame();
}

// ========== Arduino Core ==========
void setup() {
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

  if (ammo <= 0 | kills >= targets) {
    gameoverLCD(kills >= targets);
  }

  // Handle shooting
  if (buttonState == LOW && checkJoy == 0) {
    checkJoy = 1;
    Serial.println("Button Pressed: " + String(cursorPos));

    if (ammo > 0) {
      if (grid[cursorPos] != HIT) {
        ammo--;
        if (grid[cursorPos] == TARGET) {
          leds[cursorY][cursorX] = 2;
          updateLeds();
          grid[cursorPos] = HIT;

          // Check ship destruction
          for (int i = 0; i < targets; i++) {
            if (ships[i].destroyed) continue;

            bool isDestroyed = true;

            // Always check part1
            if (grid[ships[i].part1] != HIT) isDestroyed = false;

            // Check part2 only if it's valid
            if (ships[i].size >= 2 && ships[i].part2 != 255 && grid[ships[i].part2] != HIT)
              isDestroyed = false;

            // Check part3 only if it's valid
            if (ships[i].size >= 3 && ships[i].part3 != 255 && grid[ships[i].part3] != HIT)
              isDestroyed = false;

            // Check part4 only if it's valid
            if (ships[i].size == 4 && ships[i].part4 != 255 && grid[ships[i].part4] != HIT)
              isDestroyed = false;

            if (isDestroyed) {
              ships[i].destroyed = true;
              kills++;
              break;
            }
          }


          updateLCD(ammo, TARGET);
        } else {
          updateLCD(ammo, EMPTY);
          grid[cursorPos] = HIT;
          leds[cursorY][cursorX] = 0;
          updateLeds();
        }
      } else {
        updateLCD(ammo, HIT);
      }
    }
  } else if (buttonState == HIGH && checkJoy == 1) {
    checkJoy = 0;
  }

  // Handle cursor movement
  unsigned long currentTime = millis();
  if (currentTime - lastMoveTime >= MOVE_INTERVAL) {
    lastMoveTime = currentTime;

    xDir = (xValue < 512 - DEADZONE) ? "LEFT" :
           (xValue > 512 + DEADZONE) ? "RIGHT" : "NONE";

    yDir = (yValue > 512 + DEADZONE) ? "UP" :
           (yValue < 512 - DEADZONE) ? "DOWN" : "NONE";

    moveCursor(xDir, yDir);
  }
}
