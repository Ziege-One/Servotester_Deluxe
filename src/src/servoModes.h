
#include <Arduino.h>

//
// =======================================================================================================
// SERVO MODES
// =======================================================================================================
//

void servoModes()
{
  // Servo frequencies and corresponding microseconds ranges -------
  // Std = 50Hz   1000 - 1500 - 2000µs = gemäss ursprünglichem Standard
  // NOR = 100Hz  1000 - 1500 - 2000µs = normal = für die meisten analogen Servos
  // SHR = 333Hz  1000 - 1500 - 2000µs = Sanwa High-Response = für alle Digitalservos
  // SSR = 400Hz   130 -  300 - 470µs  = Sanwa Super Response = nur für Sanwa Servos der SRG-Linie
  // SUR = 800Hz   130 -  300 - 470µs  = Sanwa Ultra Response
  // SXR = 1600Hz  130 -  300 - 470µs  = Sanwa Xtreme Response

  if (SERVO_MODE <= STD)
    SERVO_MODE = STD; // Min. limit

  if (SERVO_MODE == STD)
  {
    SERVO_Hz = 50;
    SERVO_MAX = 2000;
    SERVO_CENTER = 1500;
    SERVO_MIN = 1000;

    servoMode = "Std.";
  }

  if (SERVO_MODE == NOR)
  {
    SERVO_Hz = 100;
    SERVO_MAX = 2000;
    SERVO_CENTER = 1500;
    SERVO_MIN = 1000;

    servoMode = "NOR";
  }

  if (SERVO_MODE == SHR)
  {
    SERVO_Hz = 333;
    SERVO_MAX = 2000;
    SERVO_CENTER = 1500;
    SERVO_MIN = 1000;

    servoMode = "SHR";
  }

  if (SERVO_MODE == SSR)
  {
    SERVO_Hz = 400;
    SERVO_MAX = 470;
    SERVO_CENTER = 300;
    SERVO_MIN = 130;

    servoMode = "SSR";
  }

  if (SERVO_MODE == SUR)
  {
    SERVO_Hz = 800;
    SERVO_MAX = 470;
    SERVO_CENTER = 300;
    SERVO_MIN = 130;

    servoMode = "SUR";
  }

  if (SERVO_MODE == SXR)
  {
    SERVO_Hz = 1600;
    SERVO_MAX = 470;
    SERVO_CENTER = 300;
    SERVO_MIN = 130;

    servoMode = "SXR";
  }

  if (SERVO_MODE >= SXR)
    SERVO_MODE = SXR; // Max. limit

  // Auto calculate a useful servo step size
  SERVO_STEPS = (SERVO_MAX - SERVO_MIN) / 100;
}
