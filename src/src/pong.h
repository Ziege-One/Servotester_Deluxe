/*
  A simple "1972 Atari Pong" game. Makes your servo tester a little bit different ;-)
  This code is based on: https://github.com/eholk/Arduino-Pong/blob/master/pong.ino
  Modified by TheDIYGuy999
*/

#ifndef pong_h
#define pong_h

#include "Arduino.h"

uint8_t PADDLE_DELAY = 15; // 15 Speed of CPU paddle movement
uint8_t BALL_DELAY = 7;    // 7 Speed of ball movement
uint8_t FRAME_RATE = 20;   // Refresh display every 20ms = 50Hz

const uint8_t PADDLE_HEIGHT = 14; // 14

const uint8_t half_paddle = PADDLE_HEIGHT / 2;

uint8_t ball_x = 64, ball_y = 32;        // 64, 32
uint8_t ball_dir_x = -1, ball_dir_y = 1; // 1, 1

uint8_t new_x;
uint8_t new_y;

unsigned long ball_update;
unsigned long paddle_update;

const uint8_t CPU_X = 12; // 12
int8_t cpu_y = 16;

const uint8_t PLAYER_X = 115; // 115
int8_t player_y = 16;

uint8_t game_over_difference = 10; // The game is over after this point difference is reached!

uint8_t cpu_points = 0;
uint8_t player_points = 0;

boolean cpu_won = false;
boolean player_won = false;

//
// =======================================================================================================
// DISPLAY UPDATE SUBFUNCTION
// =======================================================================================================
//

void displayUpdate()
{

  static unsigned long lastDisplay;
  if (millis() - lastDisplay >= FRAME_RATE)
  {
    lastDisplay = millis();

    // clear screen ----
    display.clear();
    display.setFont(ArialMT_Plain_10);
    display.setTextAlignment(TEXT_ALIGN_CENTER);
    // display.drawFrame(0, 0, 128, 64); // only for screen offset test!
    display.drawCircle(new_x, new_y, 1);                         // Ball
    display.drawVerticalLine(CPU_X, cpu_y, PADDLE_HEIGHT);       // CPU paddle
    display.drawVerticalLine(PLAYER_X, player_y, PADDLE_HEIGHT); // Player paddle

    display.drawVerticalLine(64, 0, 3); // Vertical dashed line segments
    display.drawVerticalLine(64, 6, 3);
    display.drawVerticalLine(64, 12, 3);
    display.drawVerticalLine(64, 18, 3);
    display.drawVerticalLine(64, 24, 3);
    display.drawVerticalLine(64, 30, 3);
    display.drawVerticalLine(64, 36, 3);
    display.drawVerticalLine(64, 42, 3);
    display.drawVerticalLine(64, 48, 3);
    display.drawVerticalLine(64, 54, 3);
    display.drawVerticalLine(64, 60, 3);

    display.drawString(44, 0, String(cpu_points));    // CPU points counter
    display.drawString(84, 0, String(player_points)); // Player points counter

    // Game over window
    if (cpu_won || player_won)
    {
      display.setColor(BLACK);
      display.fillRect(22, 12, 84, 45); // Clear area behind window
      display.setColor(WHITE);
      display.drawRect(22, 12, 84, 45); // Draw window frame

      display.drawString(64, 18, "GAME OVER"); // Game over

      display.drawString(64, 38, "Press Encoder!"); // Press button "Back" to restart
    }
    if (cpu_won)
    {
      display.drawString(64, 28, "YOU LOST"); // You lost
    }
    if (player_won)
    {
      display.drawString(64, 28, "YOU WON"); // You won
    }

    // show display queue ----
    display.display();
  }
}

//
// =======================================================================================================
// PONG GAME
// =======================================================================================================
//

void pong(bool paddleUp, bool paddleDown, bool reset, uint8_t paddleSpeed)
{
  unsigned long time = millis();

  static boolean center;

  // Restart game ----------------------------------------------------------
  if (reset)
  {
    cpu_won = false;
    player_won = false;
    cpu_points = 0;
    player_points = 0;
  }

  // Ball update -----------------------------------------------------------

  if (PONG_BALL_RATE == 1)
    BALL_DELAY = 20;
  if (PONG_BALL_RATE == 2)
    BALL_DELAY = 15;
  if (PONG_BALL_RATE == 3)
    BALL_DELAY = 10;
  if (PONG_BALL_RATE == 4)
    BALL_DELAY = 5;

  static unsigned long lastBall;
  if (millis() - lastBall >= BALL_DELAY && !cpu_won && !player_won)
  {
    lastBall = millis();

    new_x = ball_x + ball_dir_x;
    new_y = ball_y + ball_dir_y;

    // Counter
    if (new_x > 54 && new_x < 74)
      center = true;
    if (center && new_x < CPU_X)
    {
      player_points++, center = false; // Count CPU points
      beepDuration = 10;
    }
    if (center && new_x > PLAYER_X)
    {
      cpu_points++, center = false; // Count Player points
      beepDuration = 10;
    }

    if (cpu_points - player_points >= game_over_difference)
    {
      cpu_won = true; // Game over, you lost
      beepDuration = 300;
    }
    if (player_points - cpu_points >= game_over_difference)
    {
      player_won = true; // Game over, you won
      beepDuration = 300;
    }

    // Check if we hit the vertical walls
    if (new_x == 1 || new_x == 126)
    {
      ball_dir_x = -ball_dir_x;
      new_x += ball_dir_x + ball_dir_x;
    }

    // Check if we hit the horizontal walls
    if (new_y == 1 || new_y == 62)
    {
      ball_dir_y = -ball_dir_y;
      new_y += ball_dir_y + ball_dir_y;
    }

    // Check if we hit the CPU paddle
    if (new_x == CPU_X && new_y >= cpu_y && new_y <= cpu_y + PADDLE_HEIGHT)
    {
      ball_dir_x = -ball_dir_x;
      new_x += ball_dir_x + ball_dir_x;
    }

    // Check if we hit the player paddle
    if (new_x == PLAYER_X && new_y >= player_y && new_y <= player_y + PADDLE_HEIGHT)
    {
      ball_dir_x = -ball_dir_x;
      new_x += ball_dir_x + ball_dir_x;
    }

    ball_x = new_x;
    ball_y = new_y;
  }

  // Paddle update -----------------------------------------------------------
  static unsigned long lastPaddle;
  if (millis() - lastPaddle >= PADDLE_DELAY && !cpu_won && !player_won)
  {
    lastPaddle = millis();

    // CPU paddle control----
    if (cpu_y + half_paddle > ball_y)
    {
      cpu_y -= 1;
    }
    if (cpu_y + half_paddle < ball_y)
    {
      cpu_y += 1;
    }
    if (cpu_y < 1)
      cpu_y = 1;
    if (cpu_y + PADDLE_HEIGHT > 63)
      cpu_y = 63 - PADDLE_HEIGHT;
  }

  // Player paddle control----
  if (paddleUp)
  {
    player_y += paddleSpeed * 3;
  }
  if (paddleDown)
  {
    player_y -= paddleSpeed * 3;
  }
  player_y = constrain(player_y, 0, (63 - PADDLE_HEIGHT));

  // Display update ------------------------------------------------------------
  displayUpdate();
}

#endif
