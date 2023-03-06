
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
  // SSR = 384Hz   130 -  300 - 470µs  = Sanwa Super Response = nur für Sanwa Servos der SRG-Linie
  // SSL = 560Hz   500 -  750 - 1000µs
  // SUR = 760Hz   130 -  300 - 470µs
  // SXR = 1520Hz  130 -  300 - 470µs

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
    SERVO_Hz = 384;
    SERVO_MAX = 470;
    SERVO_CENTER = 300;
    SERVO_MIN = 130;

    servoMode = "SSR";
  }

  if (SERVO_MODE == SSL)
  {
    SERVO_Hz = 560;
    SERVO_MAX = 1000;
    SERVO_CENTER = 750;
    SERVO_MIN = 500;

    servoMode = "SSL";
  }

  if (SERVO_MODE == SUR)
  {
    SERVO_Hz = 760;
    SERVO_MAX = 470;
    SERVO_CENTER = 300;
    SERVO_MIN = 130;

    servoMode = "SUR";
  }

  if (SERVO_MODE == SXR)
  {
    SERVO_Hz = 1520;
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
