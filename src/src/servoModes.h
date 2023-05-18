
#include <Arduino.h>

//
// =======================================================================================================
// SERVO MODES
// =======================================================================================================
//

// Servo frequencies and corresponding microseconds ranges -------
  // Std = 50Hz   1000 - 1500 - 2000µs = gemäss ursprünglichem Standard
  // NOR = 100Hz  1000 - 1500 - 2000µs = normal = für die meisten analogen Servos
  // SHR = 333Hz  1000 - 1500 - 2000µs = Sanwa High Response = für alle Digitalservos
  // SSR = 400Hz   130 -  300 - 470µs  = Sanwa Super Response = nur für Sanwa Servos der SRG-Linie
  // SUR = 800Hz   130 -  300 - 470µs  = Sanwa Ultra Response
  // SXR = 1600Hz  130 -  300 - 470µs  = Sanwa Xtreme Response

// Default µs values see eepromInit()

void servoModes()
{
  
  if (SERVO_MODE <= STD)
    SERVO_MODE = STD; // Min. limit

  // Modes with normal pulse lengths ---------------
  if (SERVO_MODE == STD)
  {
    SERVO_Hz = 50;
    SERVO_MAX = SERVO_MAX_STD;
    SERVO_CENTER = SERVO_CENTER_STD;
    SERVO_MIN = SERVO_MIN_STD;

    servoMode = "Std.";
  }

  if (SERVO_MODE == NOR)
  {
    SERVO_Hz = 100;
    SERVO_MAX = SERVO_MAX_STD;
    SERVO_CENTER = SERVO_CENTER_STD;
    SERVO_MIN = SERVO_MIN_STD;

    servoMode = "NOR";
  }

  if (SERVO_MODE == SHR)
  {
    SERVO_Hz = 333;
    SERVO_MAX = SERVO_MAX_STD;
    SERVO_CENTER = SERVO_CENTER_STD;
    SERVO_MIN = SERVO_MIN_STD;

    servoMode = "SHR";
  }

  // Modes with Sanwa pulse lengths ---------------
  if (SERVO_MODE == SSR)
  {
    SERVO_Hz = 400;
    SERVO_MAX = SERVO_MAX_SANWA;
    SERVO_CENTER = SERVO_CENTER_SANWA;
    SERVO_MIN = SERVO_MIN_SANWA;

    servoMode = "SSR";
  }

  if (SERVO_MODE == SUR)
  {
    SERVO_Hz = 800;
    SERVO_MAX = SERVO_MAX_SANWA;
    SERVO_CENTER = SERVO_CENTER_SANWA;
    SERVO_MIN = SERVO_MIN_SANWA;

    servoMode = "SUR";
  }

  if (SERVO_MODE == SXR)
  {
    SERVO_Hz = 1600;
    SERVO_MAX = SERVO_MAX_SANWA;
    SERVO_CENTER = SERVO_CENTER_SANWA;
    SERVO_MIN = SERVO_MIN_SANWA;

    servoMode = "SXR";
  }

  if (SERVO_MODE >= SXR)
    SERVO_MODE = SXR; // Max. limit

  // Auto calculate a useful servo step size
  SERVO_STEPS = (SERVO_MAX - SERVO_MIN) / 100;
}
