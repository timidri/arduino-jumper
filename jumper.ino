// #include <LiquidCrystal_I2C.h>
#include <LiquidCrystal.h>
#include <EasyButton.h>
#include <Ticker.h>
#include <EasyBuzzer.h>
#include "pitches.h"

// LiquidCrystal_I2C lcd(0x27, 16, 2);
LiquidCrystal lcd(12, 11, 5, 4, 3, 2);

EasyButton button(6);

byte player[] = {
  B00000,
  B00000,
  B00000,
  B01110,
  B01010,
  B01010,
  B01010,
  B10001
};

byte obstacle[] = {
  B00000,
  B00000,
  B00100,
  B01110,
  B11111,
  B11111,
  B01110,
  B00100
};

byte heart[] = {
  B00000,
  B00000,
  B01010,
  B11111,
  B11111,
  B01110,
  B01110,
  B00100
};

int SOUND_DOWN[][2] = {
  { 500, 30},
  { 300, 40},
  { -1, 0}
};

int SOUND_UP[][2] =   {
  { 300, 30},
  { 500, 40},
  { -1, 0}
};

int SOUND_DIED[][2] =  {
  { NOTE_E4, 30},
  { 0, 10},
  { NOTE_E4, 30},
  { 0, 10},
  { NOTE_E4, 30},
  { NOTE_C4, 90},
  { -1, 0}

};

int SOUND_GAME_OVER[][2] =  {
  { NOTE_DS3, 100},
  { 0, 80},
  { NOTE_D3, 100},
  { 0, 80},
  { NOTE_CS3, 100},
  { 0, 80},
  { NOTE_C3, 240},
  { -1, 0}

};

enum { PLAYER,
       OBSTACLE,
       HEART
     } character;

char framebuf[17];

bool isJumping = false;
int score = 0;
int speed = 1;
int lives = 3;

void updateGround();
void showPlayer();
void land();

#define START_TICK_TIME 500
#define START_LIVES 3
#define MAX_LIVES 6
#define EXTRA_LIFE_SCORE 100

Ticker groundTimer(updateGround, START_TICK_TIME, 0, MICROS);
Ticker playerTimer(showPlayer, 50, 0, MICROS);
Ticker jumpTimer(land, 3 * START_TICK_TIME);

const int buzzer = 9; //buzzer to arduino pin 9
int (*musicToPlay)[2];
int notePlaying = -1;
bool currentlyPlaying = false;

enum { ALIVE, JUMPING, DEAD, GAME_OVER } state;
int gameState = ALIVE;

void setup() {
  lcd.begin(16, 2);
  lcd.createChar(PLAYER, player);
  lcd.createChar(OBSTACLE, obstacle);
  lcd.createChar(HEART, heart);
  Serial.begin(9600);
  button.begin();
  EasyBuzzer.setPin(buzzer);
  // pinMode(buzzer, OUTPUT); // Set buzzer - pin 9 as an output
  EasyBuzzer.setOffDuration(20);
  EasyBuzzer.setPauseDuration(20);
  initGame();
}

void beep(int freq = 440, int duration = 500) {
  EasyBuzzer.singleBeep(freq, duration); // Send 1KHz sound signal...
}

void initGame() {
  Serial.println("start");
  lcd.clear();
  lives = START_LIVES;
  score = 0;
  speed = 1;
  setSpeed(speed);
  initLevel();
}

void initLevel() {
  clearRow(framebuf);
  drawLives();
  jumpTimer.stop();
  groundTimer.start();
  playerTimer.start();
  gameState = ALIVE;
}

void playCB() {
  // int musicLength = sizeof(musicToPlay) / sizeof(musicToPlay[0]);
  if (notePlaying == -1) {
    musicToPlay = NULL;
  }
  // Serial.println("playCB: length: " + String(musicLength));
  currentlyPlaying = false;
}

void playCurrentNote() {
  // Serial.println("CurrentlyPlaying: " + String(currentlyPlaying));
  if (musicToPlay == NULL) return;
  if (currentlyPlaying == true) return;
  // Serial.println("Noteplaying: " + String(notePlaying));
  int freq = musicToPlay[notePlaying][0];
  if (freq == -1) return; // last note played?
  int duration = musicToPlay[notePlaying][1];
  Serial.println("Note: " + String(freq) + " " + String(duration));
  notePlaying++;
  currentlyPlaying = true;
  EasyBuzzer.singleBeep(freq, duration, playCB);
}

void play(int music[][2]) {
  Serial.println("play: " + String(music[0][0]) + "/" + String(music[0][1]));
  musicToPlay = music;
  notePlaying = 0;
  currentlyPlaying = false;
}

void clearRow(char *buf) {
  for (int i = 0; i < 16; i++) {
    buf[i] = ' ';
  }
}

void shiftLeft(char *buf) {
  for (int i = 0; i < 16; i++) {
    buf[i] = buf[i + 1];
  }
}

void showRow(char *buf) {
  lcd.setCursor(0, 1);
  for (int i = 0; i < 16; i++) {
    lcd.write(buf[i]);
  }
}

void drawLives() {
  lcd.setCursor(0, 0);
  lcd.print("      ");
  for (int i = 0; i < lives; i++) {
    lcd.write(HEART);
  }
  for (int i = lives; i < MAX_LIVES; i++) {
    lcd.write(' ');
  }
  Serial.println("lives: " + String(lives));

}

void die() {
  lives--;
  drawLives();
  groundTimer.stop();
  playerTimer.stop();
  if (lives == 0) {
    showGameOver();
  } else {
    showRestart();
  }
}

void showPlayer() {
  bool collision = false;
  if (gameState == JUMPING) {
    lcd.setCursor(0, 1);
    lcd.write(framebuf[0]);
    lcd.setCursor(0, 0);
  } else {
    lcd.setCursor(0, 0);
    lcd.write(' ');
    lcd.setCursor(0, 1);
    if (framebuf[0] != ' ') {
      collision = true;
    }
  }
  if (collision) {
    die();
  } else {
    lcd.write(PLAYER);
  }
}

void showScore() {
  lcd.setCursor(13, 0);
  lcd.print(String(score) + "   ");
}

void setSpeed(int speed) {
  int tickTime = (START_TICK_TIME * pow(0.85, speed - 1));
  groundTimer.interval(tickTime);
  jumpTimer.interval(3 * tickTime);
  Serial.println("speed: " + String(speed));
}

void increaseSpeed() {
  setSpeed(++speed);
}

void displayExplosion() {
  lcd.setCursor(0, 1);
  lcd.write('@');
  play(SOUND_DOWN);
}

void jump() {
  play(SOUND_UP);
  gameState = JUMPING;
  jumpTimer.start();
}

void land() {
  gameState = ALIVE;
  if (framebuf[0] != ' ') {
    displayExplosion();
    framebuf[0] = ' ';
  }
  jumpTimer.stop();
}

void updateGround() {
  int o = random(4);
  framebuf[16] = o == 0 ? char(OBSTACLE) : char(' ');
  shiftLeft(framebuf);
  showRow(framebuf);
  score++;
  showScore();
  if (score % 20 == 0) {
    increaseSpeed();
  }
  if (score % EXTRA_LIFE_SCORE == 0 && lives < MAX_LIVES) {
    lives++;
    drawLives();
  }
}

void showRestart() {
  gameState = DEAD;
  lcd.home();
  lcd.print("Oops!");
  play(SOUND_DIED);
  // while (!button.read()) {
  // EasyBuzzer.update();
  // delay(100);
  // }
}

void showGameOver() {
  gameState = GAME_OVER;
  lcd.home();
  lcd.print("Game over!");
  play(SOUND_GAME_OVER);
  // while (!button.read()) {
  //   EasyBuzzer.update();
  //   delay(100);
  // }
}

void loop() {
  bool wasReleased = button.isReleased();
  bool buttonPressed = button.read();
  switch (gameState) {
    case ALIVE:
      if (wasReleased && buttonPressed) {
        jump();
      }
      break;
    case DEAD:
      if (buttonPressed) {
        initLevel();
      }
      break;
    case GAME_OVER:
      if (buttonPressed) {
        initGame();
      }
      break;
  }

  groundTimer.update();
  playerTimer.update();
  jumpTimer.update();
  EasyBuzzer.update();
  playCurrentNote();
}
