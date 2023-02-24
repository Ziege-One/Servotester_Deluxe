/*
  A simple "Flappy Birds" game. Makes your servo tester a little bit different ;-)
  Original by Volos Projects https://www.youtube.com/watch?v=BzjRDpANjaQ
  Modified by TheDIYGuy999
*/

#include "Arduino.h"
#include "flappyImages.h"

const float birdX = 30;                // Bird x position
float birdY = 22;                      // Bird y position
int score = 0;                         // Game score
int birdDirection = 0;                 // Bird direction 1 = upwards, 0 = downwards
unsigned long buttonPressedMillis = 0; // Button was pressed @ this time
float obstacleXposition[4];            // Obstacle x position
int gapPosition[4];                    // Obstacle gap y positions

const int space = 32;    // horizontal space between obstacles (1/4 of screen width)
const int gapWidth = 30; // vertical opening between obstacles

bool game = false; // false = start screen, true = game
bool buzzer = false;

//
// =======================================================================================================
// SUB FUNCTIONS
// =======================================================================================================
//

// macro for detection of rising edge and debouncing
/*the state argument (which must be a variable) records the current
  and the last 7 reads by shifting one bit to the left at each read.
  If the value is 15(=0b00001111) we have one rising edge followed by
  4 consecutive 1's. That would qualify as a debounced rising edge*/
#define DRE(signal, state) (state = (state << 1) | signal) == B00001111

// Rising state variables for each button
byte button1RisingState;

// Game init sub function
void gameInit()
{
  for (int i = 0; i < 4; i++) // calculate random obstacle gap offsets
  {
    obstacleXposition[i] = 128 + ((i + 1) * space);
    gapPosition[i] = random(8, 32);
    // Serial.println(obstacleXposition[i]);
  }
}

// Game over sub function
void gameOver()
{
  game = false;
  birdY = 22;
  score = 0;
  birdDirection = 0;
  display.clear();
  // buzzer = true;
  gameInit();
}

//
// =======================================================================================================
// FLAPPY BIRDS GAME
// =======================================================================================================
//

void flappyBirds(bool buttonInput)
{
  display.clear();
  display.setTextAlignment(TEXT_ALIGN_LEFT);

  // Start screen ------------
  if (game == false)
  {

    uint8_t birdYpos = 32;
    birdYpos = random(30, 34); // let the bird flutter

    display.setFont(ArialMT_Plain_16);
    display.drawString(0, 4, "Flappy");
    display.drawXbm(0, 0, 128, 64, flappyBackground);
    display.drawXbm(20, birdYpos, 14, 9, bird);
    display.setFont(ArialMT_Plain_10);
    display.drawString(0, 44, "Press to start");

    if (DRE(buttonInput, button1RisingState)) // We need to detect a rising edge here!
    {
      gameInit(); // Init game
      game = true; // Start game
    }
  }

  // Game screen ---------------
  if (game == true)
  {

    if (buttonInput)
    {
      buttonPressedMillis = millis();
      birdDirection = 1; // Bird upwards
    }

    for (int j = 0; j < 4; j++)
    {
      display.setColor(WHITE);
      display.fillRect(obstacleXposition[j], 0, 6, 64); // Draw obstacle bar across entire display height
      display.setColor(BLACK);
      display.fillRect(obstacleXposition[j], gapPosition[j], 6, gapWidth); // then cut out passage gap
    }
    display.setColor(WHITE);

    display.setFont(ArialMT_Plain_10);
    display.drawString(3, 27, String(score)); // display score

    if (birdDirection)
      display.drawXbm(birdX, birdY, 14, 9, bird2); // draw the bird with wings down
    else
      display.drawXbm(birdX, birdY, 14, 9, bird); // draw the bird with wings up

    for (int j = 0; j < 4; j++)
    {
      obstacleXposition[j] = obstacleXposition[j] - 0.03; // - 0.01 = Obstacle travel speed
      if (obstacleXposition[j] < -7)                      // obstacle passed successfully
      {
        score = score + 1;
        gapPosition[j] = random(8, 32);
        obstacleXposition[j] = 128;
      }
    }

    if (millis() - buttonPressedMillis > 185) // some millis after click
      birdDirection = 0;                      // bird downwards again

    if (birdDirection == 0)
      birdY = birdY + 0.01; // Sink down speed 0.01
    else
      birdY = birdY - 0.03; // Jump speed - 0.03

    // Quit game by pressing button 800ms ----
    if (millis() - buttonPressedMillis > 800 && buttonInput)
      gameOver();

    // Game over, collision with screen boundary detected ----
    if (birdY > 63 || birdY < 0)
    {
      gameOver();
    }

    // Game over, collision with obstacle detected ----
    for (int m = 0; m < 4; m++)
      if (obstacleXposition[m] <= birdX + 7 && birdX + 7 <= obstacleXposition[m] + 6) // Check in x direction
      {
        if (birdY < gapPosition[m] || birdY + 8 > gapPosition[m] + gapWidth) // Check in y direction
        {
          gameOver();
        }
      }
    display.drawRect(0, 0, 128, 64); // Frame around screen
  }
  display.display();
}
