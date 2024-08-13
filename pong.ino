#include <LedControl.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

Adafruit_SSD1306 IC2(128, 64, &Wire, -1); 

#define MOVE_INTERVAL 75
#define MAX_DEVICES    1

LedControl lc = LedControl(11, 13, 10, MAX_DEVICES);

#define MAX_VALUE   1024
#define UPPER_LIMIT    8
#define LOWER_LIMIT    0

#define CALIBRATION_SAMPLES 100
#define FILTER_ALPHA        0.1

#define PLAYER_SIZE         3
#define BALLSPEEDTHRESHOLD  3
#define BALL_INI_X          3
#define BALL_INI_Y          2

#define YPINLEFT  A1 // na verdade.. colocado em X
#define YPINRIGHT A0

struct Ball {
    int8_t x;
    int8_t y;
    int8_t vx;
    int8_t vy;
};

struct Player {
    uint8_t y;
    uint8_t score;
};

Player p1 = {3,0};
Player p2 = {3,0};

Ball ball {
     .x = BALL_INI_X,
     .y = BALL_INI_Y,
    .vx = 1,
    .vy = 1
};

uint16_t calibration_l = 0;
uint16_t calibration_r = 0;

float filtered_l = 0;
float filtered_r = 0;

unsigned long last_move_time = 0;

void calibrate_joysticks() {
  long sum_left = 0;
  long sum_right = 0;

  for (int i = 0; i < CALIBRATION_SAMPLES; i++) {
    sum_left += analogRead(YPINLEFT);
    sum_right += analogRead(YPINRIGHT);
    delay(10);
  }

  calibration_l = sum_left / CALIBRATION_SAMPLES;
  calibration_r = sum_right / CALIBRATION_SAMPLES;
}

void read_joystick() {
  int16_t current_left = analogRead(YPINLEFT) - calibration_l;
  int16_t current_right = analogRead(YPINRIGHT) - calibration_r;

  filtered_l = (FILTER_ALPHA * current_left) + ((1 - FILTER_ALPHA) * filtered_l);
  filtered_r = (FILTER_ALPHA * current_right) + ((1 - FILTER_ALPHA) * filtered_r);

  p1.y = map(filtered_l, -MAX_VALUE / 2, MAX_VALUE / 2, LOWER_LIMIT, UPPER_LIMIT - 1);
  p2.y = map(filtered_r, -MAX_VALUE / 2, MAX_VALUE / 2, LOWER_LIMIT, UPPER_LIMIT - 1);

  if (p1.y > UPPER_LIMIT - 1) p1.y = UPPER_LIMIT - 1;
  if (p1.y < LOWER_LIMIT) p1.y = LOWER_LIMIT;
  if (p2.y > UPPER_LIMIT - 1) p2.y = UPPER_LIMIT - 1;
  if (p2.y < LOWER_LIMIT) p2.y = LOWER_LIMIT;
}

void setup_led_matrix(){
  for (uint8_t i = 0; i < MAX_DEVICES; i++) {
    lc.shutdown(i, false);
    lc.setIntensity(i, 8);
    lc.clearDisplay(i);
  }
}

void display_score(){
  IC2.print(p1.score);
  IC2.print(F(" x "));
  IC2.print(p2.score);
  IC2.display();
}

void setup() {
  Serial.begin(9600);
  pinMode(YPINLEFT, INPUT);
  pinMode(YPINRIGHT, INPUT);

  calibrate_joysticks();

  if(!IC2.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { 
    Serial.println(F("SSD1306: erro de memória"));
    for(;;);
  }
  IC2.display();

  IC2.clearDisplay();
  IC2.setTextSize(4);
  IC2.setTextColor(SSD1306_WHITE);
  IC2.setCursor(1,20);
  display_score();
  
  setup_led_matrix();
  delay(2000);
}

void check_player_collision() {
  if (ball.x == LOWER_LIMIT && (ball.y >= p1.y && ball.y <= p1.y + PLAYER_SIZE - 1)) {
    ball.vx *= -1;
    ball.vy = (ball.y - p1.y) - PLAYER_SIZE / 2;
  }
  if (ball.x == UPPER_LIMIT-1 && (ball.y >= p2.y && ball.y <= p2.y + PLAYER_SIZE - 1)) {
    ball.vx *= -1;
    ball.vy = (ball.y - p2.y) - PLAYER_SIZE / 2;
  }
}

void check_wall_collision() {
  if (ball.y == 0 || ball.y == 7) ball.vy *= -1;
}

void check_score(){
  boolean scored = false;

  if (ball.x == UPPER_LIMIT) {
    p1.score++;
    scored = true;
  }

  if (ball.x < LOWER_LIMIT) {
    p2.score++;
    scored = true;
  }

  if (scored){
    ball.x = BALL_INI_X;
    ball.y = BALL_INI_Y;
    IC2.clearDisplay();
    IC2.setCursor(1,20);
    display_score();
    delay(2000);
  }
}


uint8_t ball_speed_counter = 0;

void update_ball_pos() {
  if (ball_speed_counter >= BALLSPEEDTHRESHOLD) {
    lc.setLed(0, ball.x, ball.y, true);
    check_player_collision();
    check_wall_collision();
    ball.x += ball.vx;
    ball.y += ball.vy;
    ball_speed_counter = 0;
  }
  ball_speed_counter++;
}

void loop() {
  unsigned long now = millis();

  if (now - last_move_time > MOVE_INTERVAL) {
    read_joystick();

    for (uint8_t i = 0; i < MAX_DEVICES; i++)
      lc.clearDisplay(i);

    update_ball_pos();

    for (uint8_t i = 0; i < PLAYER_SIZE; i++) {
      lc.setLed(0, 0, p1.y + i, true);
      lc.setLed(0, 7, p2.y + i, true);
    }

    last_move_time = now;
  }

  check_score();
}

