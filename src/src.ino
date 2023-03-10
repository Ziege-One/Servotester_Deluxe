/*
   Servotester Deluxe
   Ziege-One (Der RC-Modelbauer)
   https://www.youtube.com/watch?v=YNLPCft2qjg&list=PLS6SFYu711FpxCNO_j4_ig4ey0NwVQPcW

   Modified by TheDIYGuy999
   https://www.youtube.com/channel/UCqWO3PNCSjHmYiACDMLr23w

   It is recommended to use VS Code instead of Arduino IDE, because board and library management is way easier this way!
   Requirements:
   - git is installed (allows to download libraries and boards automatically): https://git-scm.com/downloads
   - PlatformIO plugin is installed in VS Code
   - Espressif32 platform is up to date in Platformio > Platforms > Updates

   If you are using Arduino IDE: Select the following board: "ESP32 Dev Module"

 ESP32 + Encoder + OLED

 /////Pin Belegung////
 GPIO 4: active piezo buzzer

 GPIO 13: Servo1
 GPIO 14: Servo2
 GPIO 27: Servo3
 GPIO 33: Servo4
 GPIO 32: Servo5 & BUS Input
 GPIO 15: Ecoder Taster
 GPIO 16: Ecoder Richtung 1
 GPIO 17: Ecoder Richtung 2

 GPIO 21: SDA OLED
 GPIO 22: SDL OLED
 */

char codeVersion[] = "0.14-beta.1"; // Software revision.

//
// =======================================================================================================
// ! ! I M P O R T A N T ! !   ALL USER SETTINGS ARE DONE IN THE FOLLOWING TABS, WHICH ARE DISPLAYED ABOVE
// (ADJUST THEM BEFORE CODE UPLOAD), DO NOT CHANGE ANYTHING IN THIS TAB
// =======================================================================================================
//

// All the required user settings are done in the following .h files:
#include "0_generalSettings.h" // <<------- general settings

//
// =======================================================================================================
// LIRBARIES & HEADER FILES, REQUIRED ESP32 BOARD DEFINITION
// =======================================================================================================
//

// Libraries (you have to install all of them in the "Arduino sketchbook"/libraries folder)
// !! Do NOT install the libraries in the sketch folder.
// No manual library download is required in Visual Studio Code IDE (see platformio.ini)

/* Boardversion
ESP32                                         2.0.5 or 2.0.6 <<--- (make sure your Espressif32 piatform is up to date in Platformio > Platforms > Updates)
 */

/* Required Libraries / Benötigte Bibliotheken
ESP32Encoder                                  0.10.1
IBusBM                                        1.1.5
ESP8266 and ESP OLED driver SSD1306 displays  4.3.0
ESP32AnalogRead                               0.2.1
Array                                         1.0.0
 */

#include <ESP32AnalogRead.h> // https://github.com/madhephaestus/ESP32AnalogRead <<------- required for calibrated battery voltage measurement
#include <ESP32Encoder.h>    // https://github.com/madhephaestus/ESP32Encoder/archive/refs/tags/0.10.1.tar.gz
#include <IBusBM.h>          // https://github.com/bmellink/IBusBM/archive/refs/tags/v1.1.5.tar.gz
#include <SH1106Wire.h>      //1.3"
#include <SSD1306Wire.h>     //0.96" https://github.com/ThingPulse/esp8266-oled-ssd1306/archive/refs/tags/4.3.0.tar.gz
#include <Array.h>           //https://github.com/TheDIYGuy999/arduino-array

// No need to install these, they come with the ESP32 board definition
#include <WiFi.h>
#include <EEPROM.h>       // for non volatile storage
#include <Esp.h>          // for displaying memory information
#include "rom/rtc.h"      // for displaying reset reason
#include "driver/mcpwm.h" // for servo PWM output
#include <string>         // std::string, std::stof
using namespace std;

// Project specific includes
#if not defined ALTERNATIVE_LOGO
#include "src/images.h" // Startlogo Der RC Modellbauer
#else
#include "src/images2.h" // Alternative Logo
#endif
#include "src/sbus.h"      // For SBUS interface
#include "src/languages.h" // Menu language ressources

// EEPROM
#define EEPROM_SIZE 48

#define adr_eprom_WIFI_ON 0           // WIFI 1 = Ein 0 = Aus
#define adr_eprom_SERVO_STEPS 4       // SERVO Steps für Encoder im Servotester Modus
#define adr_eprom_SERVO_MAX 8         // SERVO µs Max Wert im Servotester Modus
#define adr_eprom_SERVO_MIN 12        // SERVO µs Min Wert im Servotester Modus
#define adr_eprom_SERVO_CENTER 16     // SERVO µs Mitte Wert im Servotester Modus
#define adr_eprom_SERVO_Hz 20         // SERVO µs Mitte Wert im Servotester Modus
#define adr_eprom_POWER_SCALE 24      // SERVO µs Mitte Wert im Servotester Modus
#define adr_eprom_SBUS_INVERTED 28    // SBUS inverted
#define adr_eprom_ENCODER_INVERTED 32 // Encoder inverted
#define adr_eprom_LANGUAGE 36         // Gewählte Sprache
#define adr_eprom_PONG_BALL_RATE 40   // Pong Ball Geschwindigkeit
#define adr_eprom_SERVO_MODE 44       // Servo operation mode

// Speicher der Einstellungen
int WIFI_ON;
int SERVO_STEPS;
int SERVO_MAX;
int SERVO_MIN;
int SERVO_CENTER;
int SERVO_Hz;
int POWER_SCALE;
int SBUS_INVERTED;
int ENCODER_INVERTED;
int LANGUAGE;
int PONG_BALL_RATE;
int SERVO_MODE;

// Encoder + button
ESP32Encoder encoder;

#define BUTTON_PIN 15         // Hardware Pin Button
#define ENCODER_PIN_1 16      // Hardware Pin1 Encoder
#define ENCODER_PIN_2 17      // Hardware Pin2 Encoder
long prev1 = 0;               // Zeitspeicher für Taster
long prev2 = 0;               // Zeitspeicher für Taster
long previousDebouncTime = 0; // Speicher Entprellzeit für Taster
int buttonState = 0;          // 0 = Taster nicht betätigt; 1 = Taster langer Druck; 2 = Taster kurzer Druck; 3 = Taster Doppelklick
int encoderState = 0;         // 1 = Drehung nach Links (-); 2 = Drehung nach Rechts (+)
int Duration_long = 600;      // Zeit für langen Druck
int Duration_double = 200;    // Zeit für Doppelklick
int bouncing = 50;            // Zeit für Taster Entprellung
int encoder_last;             // Speicher letzer Wert Encoder
int encoder_read;             // Speicher aktueller Wert Encoder
int encoderSpeed;             // Speicher aktuelle Encoder Geschwindigkeit für Beschleunigung
bool disableButtonRead;       // In gewissen Situationen soll der Encoder Button nicht von der blockierenden Funktion gelesen werden

// 3 pin connectors, used as input and output
#define SERVO_CONNECTOR_1 13
#define SERVO_CONNECTOR_2 14
#define SERVO_CONNECTOR_3 27
#define SERVO_CONNECTOR_4 33
#define SERVO_CONNECTOR_5 32

// Servo
volatile unsigned char servopin[5] = {SERVO_CONNECTOR_1, SERVO_CONNECTOR_2, SERVO_CONNECTOR_3, SERVO_CONNECTOR_4, SERVO_CONNECTOR_5}; // Pins Servoausgang 1 - 5
// Servo servo[5];                                                                                                                       // Servo Objekte erzeugen
int servo_pos[5];      // Speicher für Servowerte
int selectedServo = 0; // Das momentan angesteuerte Servo
String servoMode = ""; // Servo operation mode text

// Servo operation modes
enum
{
  STD, // Std = 50Hz   1000 - 1500 - 2000µs = gemäss ursprünglichem Standard
  NOR, // NOR = 100Hz  1000 - 1500 - 2000µs = normal = für die meisten analogen Servos
  SHR, // SHR = 333Hz  1000 - 1500 - 2000µs = Sanwa High-Response = für alle Digitalservos
  SSR, // SSR = 400Hz   130 -  300 - 470µs  = Sanwa Super Response = nur für Sanwa Servos der SRG-Linie
  SUR, // SUR = 800Hz   130 -  300 - 470µs  = Sanwa Ultra Response
  SXR  // SXR = 1600Hz  130 -  300 - 470µs  = Sanwa Xtreme Response
};

// Sound
#define BUZZER_PIN 4 // Active 3V buzzer
int beepDuration;    // how long the beep will be

// Oscilloscope pin
#define OSCILLOSCOPE_PIN 32 // ADC channel 2 only!

// Serial command pins for SBUS, IBUS -----
#define COMMAND_RX 32 // pin 13
#define COMMAND_TX -1 // -1 is just a dummy

// SBUS
bfs::SbusRx sBus(&Serial2);
std::array<int16_t, bfs::SbusRx::NUM_CH()> SBUSchannels;

// IBUS
IBusBM IBus; // IBus object

// Externe Spannungsversorgung
#define BATTERY_DETECT_PIN 36  // Hardware Pin Externe Spannung in. ADC channel 2 only!
bool batteryDetected;          // Akku vorhanden
int numberOfBatteryCells;      // Akkuzellen
float batteryVoltage;          // Akkuspannung in V
float batteryChargePercentage; // Akkuspannung in Prozent

// Menüstruktur
/*
 * 1 = Servotester_Auswahl       Auswahl -> 51 Servotester_Menu
 * 2 = Automatik_Modus_Auswahl   Auswahl -> 52 Automatik_Modus_Menu
 * 3 = Impuls_lesen_Auswahl      Auswahl -> 53 Impuls_lesen_Menu
 * 4 = Multiswitch_lesen_Auswahl Auswahl -> 54 Multiswitch_lesen_Menu
 * 5 = SBUS_lesen_Auswahl        Auswahl -> 55 SBUS_lesen_Menu
 * 6 = Einstellung_Auswahl       Auswahl -> 56 Einstellung_Menu
 * etc.
 */
enum
{
  Servotester_Auswahl = 1,
  Automatik_Modus_Auswahl = 2,
  Impuls_lesen_Auswahl = 3,
  Multiswitch_lesen_Auswahl = 4,
  SBUS_lesen_Auswahl = 5,
  IBUS_lesen_Auswahl = 6,
  Oscilloscope_Auswahl = 7,
  Rechner_Auswahl = 8,
  Pong_Auswahl = 9,
  Flappy_Birds_Auswahl = 10,
  Einstellung_Auswahl = 11,
  //
  Servotester_Menu = 51,
  Automatik_Modus_Menu = 52,
  Impuls_lesen_Menu = 53,
  Multiswitch_lesen_Menu = 54,
  SBUS_lesen_Menu = 55,
  IBUS_lesen_Menu = 56,
  Oscilloscope_Menu = 57,
  Rechner_Menu = 58,
  Flappy_Birds_Menu = 59,
  Pong_Menu = 60,
  Einstellung_Menu = 61
};

//-Menu 52 Automatik Modus
int Autopos[5];      // Speicher
bool Auto_Pause = 0; // Pause im Auto Modus

//-Menu 53 Impuls lesen
int Impuls_min = 1000;
int Impuls_max = 2000;

//-Menu 54 Multiwitch Futaba lesen
#define kanaele 9    // Anzahl der Multiswitch Kanäle
int value1[kanaele]; // Speicher Multiswitch Werte

//-Menu
int Menu = Servotester_Auswahl; // Aktives Menu
bool SetupMenu = false;         // Zustand Setupmenu
int Einstellung = 0;            // Aktives Einstellungsmenu
bool Edit = false;              // Einstellungen ausgewählt

// Battery voltage
ESP32AnalogRead battery;

// OLED
#ifdef OLED1306
SSD1306Wire display(0x3c, SDA, SCL); // Oled Hardware an SDA 21 und SCL 22
#else
SH1106Wire display(0x3c, SDA, SCL); // Oled Hardware an SDA 21 und SCL 22
#endif

// Webserver auf Port 80
WiFiServer server(80);

// Speicher HTTP request
String header;

// Für HTTP GET value
String valueString = String(5);
int pos1 = 0;
int pos2 = 0;

// These are used to print the reset reason on startup
const char *RESET_REASONS[] = {"POWERON_RESET", "NO_REASON", "SW_RESET", "OWDT_RESET", "DEEPSLEEP_RESET", "SDIO_RESET", "TG0WDT_SYS_RESET", "TG1WDT_SYS_RESET", "RTCWDT_SYS_RESET", "INTRUSION_RESET", "TGWDT_CPU_RESET", "SW_CPU_RESET", "RTCWDT_CPU_RESET", "EXT_CPU_RESET", "RTCWDT_BROWN_OUT_RESET", "RTCWDT_RTC_RESET"};

unsigned long currentTime = millis();     // Aktuelle Zeit
unsigned long currentTimeAuto = millis(); // Aktuelle Zeit für Auto Modus
unsigned long currentTimeSpan = millis(); // Aktuelle Zeit für Externe Spannung
unsigned long previousTime = 0;           // Previous time
unsigned long previousTimeAuto = 0;       // Previous time für Auto Modus
unsigned long previousTimeSpan = 0;       // Previous time für Externe Spannung
int TimeAuto = 50;                        // Auto time
const long timeoutTime = 2000;            // Define timeout time in milliseconds (example: 2000ms = 2s)

//
// =======================================================================================================
// SUB FUNCTIONS & ADDITIONAL HEADERS
// =======================================================================================================
//

// Map as Float
float map_float(float x, float in_min, float in_max, float out_min, float out_max)
{
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

// Convert µs to degrees (°)
float us2degree(uint16_t value)
{
  return map_float(float(value), float(SERVO_MIN), float(SERVO_MAX), -45.0, 45.0);
}

// buzzer control
void beep()
{
  static unsigned long buzzerTriggerMillis;
  if (beepDuration > 0 && !digitalRead(BUZZER_PIN))
  {
    digitalWrite(BUZZER_PIN, HIGH);
    buzzerTriggerMillis = millis();
  }

  if (millis() - buzzerTriggerMillis >= beepDuration)
  {
    digitalWrite(BUZZER_PIN, LOW);
    beepDuration = 0;
  }
}

// Loop time measurement
unsigned long loopDuration()
{
  static unsigned long timerOld;
  unsigned long loopTime;
  unsigned long timer = millis();
  loopTime = timer - timerOld;
  timerOld = timer;
  return loopTime;
}

// Additional headers
#include "src/pong.h"         // A little pong game :-)
#include "src/flappyBirds.h"  // A little flappy birds game :-)
#include "src/calculator.h"   // A handy calculator
#include "src/webInterface.h" // Configuration website
#include "src/servoModes.h"   // Servo operation profiles
#include "src/oscilloscope.h" // A handy oscilloscope

//
// =======================================================================================================
// mcpwm unit SETUP for servos (1x during startup)
// =======================================================================================================
//
// See: https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/peripherals/mcpwm.html#configure

void setupMcpwm()
{

  // Unit 0 ---------------------------------------------------------------------
  // 1. set our servo 1 - 4 output pins
  mcpwm_gpio_init(MCPWM_UNIT_0, MCPWM0A, SERVO_CONNECTOR_1); // Set steering as PWM0A
  mcpwm_gpio_init(MCPWM_UNIT_0, MCPWM0B, SERVO_CONNECTOR_2); // Set shifting as PWM0B
  mcpwm_gpio_init(MCPWM_UNIT_0, MCPWM1A, SERVO_CONNECTOR_3); // Set coupling as PWM1A
  mcpwm_gpio_init(MCPWM_UNIT_0, MCPWM1B, SERVO_CONNECTOR_4); // Set winch  or beacon as PWM1B

  // 2. configure MCPWM parameters
  mcpwm_config_t pwm_config;
  pwm_config.frequency = SERVO_Hz; // frequency
  pwm_config.cmpr_a = 0;           // duty cycle of PWMxa = 0
  pwm_config.cmpr_b = 0;           // duty cycle of PWMxb = 0
  pwm_config.counter_mode = MCPWM_UP_COUNTER;
  pwm_config.duty_mode = MCPWM_DUTY_MODE_0; // 0 = not inverted, 1 = inverted

  // 3. configure channels with settings above
  mcpwm_init(MCPWM_UNIT_0, MCPWM_TIMER_0, &pwm_config); // Configure PWM0A & PWM0B
  mcpwm_init(MCPWM_UNIT_0, MCPWM_TIMER_1, &pwm_config); // Configure PWM1A & PWM1B

  // Unit 1 ---------------------------------------------------------------------
  // 1. set our servo 5 output pin
  mcpwm_gpio_init(MCPWM_UNIT_1, MCPWM0A, SERVO_CONNECTOR_5); // Set ESC as PWM0A

  // 2. configure MCPWM parameters
  mcpwm_config_t pwm_config1;
  pwm_config1.frequency = SERVO_Hz; // frequency
  pwm_config1.cmpr_a = 0;           // duty cycle of PWMxa = 0
  pwm_config1.cmpr_b = 0;           // duty cycle of PWMxb = 0
  pwm_config1.counter_mode = MCPWM_UP_COUNTER;
  pwm_config1.duty_mode = MCPWM_DUTY_MODE_0; // 0 = not inverted, 1 = inverted

  // 3. configure channels with settings above
  mcpwm_init(MCPWM_UNIT_1, MCPWM_TIMER_0, &pwm_config1); // Configure PWM0A & PWM0B
}

//
// =======================================================================================================
// MAIN ARDUINO SETUP (1x during startup)
// =======================================================================================================
//
void setup()
{
  // Serial setup
  Serial.begin(115200); // USB serial (for DEBUG)

  // Print some system and software info to serial monitor
  delay(1000); // Give serial port/connection some time to get ready
  Serial.printf("\n**************************************************************************************************\n");
  Serial.printf("Sevotester Deluxe for ESP32 software version %s\n", codeVersion);
  Serial.printf("Original version: https://github.com/Ziege-One/Servotester_Deluxe\n");
  Serial.printf("Modified version: https://github.com/TheDIYGuy999/Servotester_Deluxe\n");
  Serial.printf("XTAL Frequency: %i MHz, CPU Clock: %i MHz, APB Bus Clock: %i Hz\n", getXtalFrequencyMhz(), getCpuFrequencyMhz(), getApbFrequency());
  Serial.printf("Internal RAM size: %i Byte, Free: %i Byte\n", ESP.getHeapSize(), ESP.getFreeHeap());
  Serial.printf("WiFi MAC address: %s\n", WiFi.macAddress().c_str());
  for (uint8_t coreNum = 0; coreNum < 2; coreNum++)
  {
    uint8_t resetReason = rtc_get_reset_reason(coreNum);
    if (resetReason <= (sizeof(RESET_REASONS) / sizeof(RESET_REASONS[0])))
    {
      Serial.printf("Core %i reset reason: %i: %s\n", coreNum, rtc_get_reset_reason(coreNum), RESET_REASONS[resetReason - 1]);
    }
  }

  // EEPROM
  EEPROM.begin(EEPROM_SIZE);

  eepromRead(); // Read EEPROM

  eepromInit(); // Initialize EEPROM (store defaults, if new or erased EEPROM is detected)

  // Setup Encoder
  ESP32Encoder::useInternalWeakPullResistors = UP;
  encoder.attachHalfQuad(ENCODER_PIN_1, ENCODER_PIN_2);
  encoder.setFilter(1023);
  pinMode(BUTTON_PIN, INPUT_PULLUP); // BUTTON_PIN = Eingang

  // Speaker setup
  pinMode(BUZZER_PIN, OUTPUT);

  // Battery
  battery.attach(BATTERY_DETECT_PIN);

  // Setup OLED
  display.init();
  display.flipScreenVertically();

  // Show logo
  display.setFont(ArialMT_Plain_10);
  display.drawXbm(0, 0, Logo_width, Logo_height, Logo_bits);
  display.display();
  delay(1500);

  // Show manual
  display.clear();
  display.setTextAlignment(TEXT_ALIGN_LEFT);
  display.setFont(ArialMT_Plain_10);
  display.drawString(0, 0, operationString[LANGUAGE]);
  display.drawString(0, 12, shortPressString[LANGUAGE]);
  display.drawString(0, 24, longPressString[LANGUAGE]);
  display.drawString(0, 36, doubleclickString[LANGUAGE]);
  display.drawString(0, 48, RotateKnobString[LANGUAGE]);
  display.display();
  delay(4000);

  if (WIFI_ON == 1)
  { // Wifi Ein
    // Print local IP address and start web server
    Serial.println(connectingAccessPointString[LANGUAGE]);
    WiFi.softAP(ssid, password);

    IPAddress IP = WiFi.softAPIP();
    Serial.print(apIpAddressString[LANGUAGE]);
    Serial.println(IP);

    // Show IP address
    display.clear();
    display.setTextAlignment(TEXT_ALIGN_LEFT);
    display.setFont(ArialMT_Plain_16);
    display.drawString(0, 10, WiFiOnString[LANGUAGE]);
    display.drawString(0, 26, ipAddressString[LANGUAGE]);
    display.drawString(0, 42, IP.toString());
    display.display();
    delay(1500);

    // Show SSID & Password
    display.clear();
    display.setTextAlignment(TEXT_ALIGN_LEFT);
    display.setFont(ArialMT_Plain_16);
    display.drawString(0, 0, "SSID: ");
    display.drawString(0, 16, ssid);
    display.drawString(0, 32, passwordString[LANGUAGE]);
    display.drawString(0, 48, password);
    display.display();
    delay(2000);

    Serial.printf("\nWiFi Tx Power Level: %u", WiFi.getTxPower());
    WiFi.setTxPower(cpType); // WiFi and ESP-Now power according to "0_generalSettings.h"
    Serial.printf("\nWiFi Tx Power Level changed to: %u\n\n", WiFi.getTxPower());

    server.begin(); // Start Webserver
  }

  // WiFi off
  else
  {
    Serial.println("");
    Serial.println(WiFiOffString[LANGUAGE]);

    display.clear();
    display.setTextAlignment(TEXT_ALIGN_LEFT);
    display.setFont(ArialMT_Plain_16);
    display.drawString(0, 10, "WiFi");
    display.drawString(0, 26, offString[LANGUAGE]);
    display.display();
    delay(1500);
  }

  display.clear();
  display.setTextAlignment(TEXT_ALIGN_CENTER);
  display.setFont(ArialMT_Plain_16);
  display.drawString(64, 10, "Software Version:");
  display.drawString(64, 26, String(codeVersion));
  display.display();
  delay(1000);

  encoder.setCount(Menu);
  servo_pos[0] = 1500;
  servo_pos[1] = 1500;
  servo_pos[2] = 1500;
  servo_pos[3] = 1500;
  servo_pos[4] = 1500;

  beepDuration = 10; // Short beep = device ready
}

//
// =======================================================================================================
// ENCODER & BUTTON
// =======================================================================================================
//
void ButtonRead()
{
  if (!disableButtonRead)
  {
    buttonState = 0;
    if (!(digitalRead(BUTTON_PIN)))
    {                  // Button gedrückt 0
      delay(bouncing); // Taster entprellen
      prev1 = millis();
      buttonState = 1;
      while ((millis() - prev1) <= Duration_long)
      {
        if (digitalRead(BUTTON_PIN))
        {                  // Button losgelassen 1 innerhalb Zeit
          delay(bouncing); // Taster entprellen
          buttonState = 2;
          prev2 = millis();
          while ((millis() - prev2) <= Duration_double)
          { // Doppelkick abwarten
            if (!(digitalRead(BUTTON_PIN)))
            {                  // Button gedrückt 0 innerhalb Zeit Doppelklick
              delay(bouncing); // Taster entprellen
              buttonState = 3;
              if (digitalRead(BUTTON_PIN))
              { // Button losgelassen 1
                break;
              }
            }
          }
          break;
        }
      }
      while (!(digitalRead(BUTTON_PIN)))
      { // Warten bis Button nicht gedückt ist = 1
      }
      Serial.print("Buttonstate: ");
      Serial.println(buttonState);

      if (buttonState == 1)
        beepDuration = 20;
      if (buttonState == 2)
        beepDuration = 10;
      if (buttonState == 3)
        beepDuration = 30;
    }
  }

  // Encoder -------------------------------------------------------------------------------------------------
  encoder_read = encoder.getCount(); // Read encoder --------------

  if (previousDebouncTime + 10 > millis()) // Debouncing 10ms -------------
  {
    encoder_last = encoder_read;
  }

  static unsigned long encoderSpeedMillis;
  static int lastEncoderSpeed;

  if (millis() - encoderSpeedMillis > 100) // Encoder speed detection -----------------
  {
    encoderSpeedMillis = millis();
    encoderSpeed = abs(encoder_read - lastEncoderSpeed);
    encoderSpeed = constrain(encoderSpeed, 1, 4);
    // Serial.println(encoderSpeed); // For encoder speed debuggging

    lastEncoderSpeed = encoder_read;
  }

  if (encoder_last > encoder_read) // Left turn detected --------------
  {
    if (encoder_last > encoder_read + 1)
    {
      if (ENCODER_INVERTED)
      {
        encoderState = 2; // right
      }
      else
      {
        encoderState = 1; // left
      }
      encoder_last = encoder_read;
      previousDebouncTime = millis();
    }
  }
  else if (encoder_last < encoder_read) // Right turn detected --------------
  {
    if (encoder_last < encoder_read - 1)
    {
      if (ENCODER_INVERTED)
      {
        encoderState = 1; // left
      }
      else
      {
        encoderState = 2; // right
      }
      encoder_last = encoder_read;
      previousDebouncTime = millis();
    }
  }
  else
  {
    encoderState = 0;
  }
}

//
// =======================================================================================================
// MENU
// =======================================================================================================
//
/*
 * 1 = Servotester       Auswahl -> 10 Servotester
 * 2 = Automatik Modus   Auswahl -> 20 Automatik Modus
 * 3 = Impuls lesen      Auswahl -> 30 Impuls lesen
 * 4 = Multiswitch lesen Auswahl -> 40 Multiswitch lesen
 * 5 = SBUS lesen        Auswahl -> 50 SBUS lesen
 * 6 = Einstellung       Auswahl -> 60 Einstellung
 */
void MenuUpdate()
{

  switch (Menu)
  {
    // Servotester Auswahl *********************************************************
  case Servotester_Auswahl:
    servoModes(); // Refresh servo operation mode
    display.clear();
    display.setTextAlignment(TEXT_ALIGN_CENTER);
    display.setFont(ArialMT_Plain_24);
    display.drawString(64, 0, "  Menu >");
    display.setFont(ArialMT_Plain_16);
    display.drawString(64, 25, servotesterString[LANGUAGE]);
    display.setFont(ArialMT_Plain_10);
    display.setTextAlignment(TEXT_ALIGN_LEFT);
    display.drawString(0, 0, "Hz");
    display.drawString(0, 10, String(SERVO_Hz));
    display.setTextAlignment(TEXT_ALIGN_RIGHT);
    display.drawString(128, 0, servoMode);
    if (batteryDetected)
    {
      display.setTextAlignment(TEXT_ALIGN_RIGHT);
      display.drawString(30, 50, String(batteryVoltage));
      display.setTextAlignment(TEXT_ALIGN_LEFT);
      display.drawString(30, 50, "V");

      display.setTextAlignment(TEXT_ALIGN_RIGHT);
      display.drawString(64, 50, String(numberOfBatteryCells));
      display.setTextAlignment(TEXT_ALIGN_LEFT);
      display.drawString(64, 50, "S");

      display.setTextAlignment(TEXT_ALIGN_RIGHT);
      display.drawString(115, 50, String(batteryChargePercentage, 0));
      display.setTextAlignment(TEXT_ALIGN_LEFT);
      display.drawString(115, 50, "%");
    }
    else
    {
      display.setTextAlignment(TEXT_ALIGN_CENTER);
      display.drawString(64, 50, noBatteryString[LANGUAGE]); // No battery
      display.setTextAlignment(TEXT_ALIGN_LEFT);
    }

    display.display();

    if (encoderState == 1)
    {
      Menu = Servotester_Auswahl;
    }
    if (encoderState == 2)
    {
      Menu++;
    }

    if (buttonState == 2)
    {
      Menu = Servotester_Menu;
    }
    break;

    // Automatikmodus Auswahl *********************************************************
  case Automatik_Modus_Auswahl:
    servoModes(); // Refresh servo operation mode
    display.clear();
    display.setTextAlignment(TEXT_ALIGN_CENTER);
    display.setFont(ArialMT_Plain_24);
    display.drawString(64, 0, "< Menu >");
    display.setFont(ArialMT_Plain_16);
    display.drawString(64, 25, automaticModeString[LANGUAGE]);
    display.drawString(64, 45, oscillateServoString[LANGUAGE]);
    display.display();

    if (encoderState == 1)
    {
      Menu--;
    }
    if (encoderState == 2)
    {
      Menu++;
    }

    if (buttonState == 2)
    {
      Menu = Automatik_Modus_Menu;
    }
    break;

  // Impuls lesen Auswahl *********************************************************
  case Impuls_lesen_Auswahl:
    display.clear();
    display.setTextAlignment(TEXT_ALIGN_CENTER);
    display.setFont(ArialMT_Plain_24);
    display.drawString(64, 0, "< Menu >");
    display.setFont(ArialMT_Plain_16);
    display.drawString(64, 25, PwmImpulseString[LANGUAGE]);
    display.drawString(64, 45, readCh1Ch5String[LANGUAGE]);
    display.display();

    if (encoderState == 1)
    {
      Menu--;
    }
    if (encoderState == 2)
    {
      Menu++;
    }

    if (buttonState == 2)
    {
      Menu = Impuls_lesen_Menu;
    }
    break;

  // Multiswitch lesen Auswahl *********************************************************
  case Multiswitch_lesen_Auswahl:
    display.clear();
    display.setTextAlignment(TEXT_ALIGN_CENTER);
    display.setFont(ArialMT_Plain_24);
    display.drawString(64, 0, "< Menu >");
    display.setFont(ArialMT_Plain_16);
    display.drawString(64, 25, "PPM Multiswitch");
    display.drawString(64, 45, readCh5String[LANGUAGE]);
    display.display();

    if (encoderState == 1)
    {
      Menu--;
    }
    if (encoderState == 2)
    {
      Menu++;
    }

    if (buttonState == 2)
    {
      Menu = Multiswitch_lesen_Menu;
    }
    break;

  // SBUS lesen Auswahl *********************************************************
  case SBUS_lesen_Auswahl:
    display.clear();
    display.setTextAlignment(TEXT_ALIGN_CENTER);
    display.setFont(ArialMT_Plain_24);
    display.drawString(64, 0, "< Menu >");
    display.setFont(ArialMT_Plain_16);
    display.drawString(64, 25, readSbusString[LANGUAGE]);
    display.drawString(64, 45, "CH5");
    display.display();

    if (encoderState == 1)
    {
      Menu--;
    }
    if (encoderState == 2)
    {
      Menu++;
    }

    if (buttonState == 2)
    {
      Menu = SBUS_lesen_Menu;
    }
    break;

  // IBUS lesen Auswahl *********************************************************
  case IBUS_lesen_Auswahl:
    display.clear();
    display.setTextAlignment(TEXT_ALIGN_CENTER);
    display.setFont(ArialMT_Plain_24);
    display.drawString(64, 0, "< Menu >");
    display.setFont(ArialMT_Plain_16);
    display.drawString(64, 25, readIbusString[LANGUAGE]);
    display.drawString(64, 45, "CH5");
    display.display();

    if (encoderState == 1)
    {
      Menu--;
    }
    if (encoderState == 2)
    {
      Menu++;
    }

    if (buttonState == 2)
    {
      Menu = IBUS_lesen_Menu;
    }
    break;

    // Oszilloskop Auswahl *********************************************************
  case Oscilloscope_Auswahl:
    display.clear();
    display.setTextAlignment(TEXT_ALIGN_CENTER);
    display.setFont(ArialMT_Plain_24);
    display.drawString(64, 0, "< Menu >");
    display.setFont(ArialMT_Plain_16);
    display.drawString(64, 25, readOscilloscopeString[LANGUAGE]);
    display.setFont(ArialMT_Plain_10);
    display.drawString(64, 45, readOscilloscopeString2[LANGUAGE]);
    display.display();

    if (encoderState == 1)
    {
      Menu--;
    }
    if (encoderState == 2)
    {
      Menu++;
    }

    if (buttonState == 2)
    {
      Menu = Oscilloscope_Menu;
    }
    break;

    // Rechner Auswahl *********************************************************
  case Rechner_Auswahl:
    display.clear();
    display.setTextAlignment(TEXT_ALIGN_CENTER);
    display.setFont(ArialMT_Plain_24);
    display.drawString(64, 0, "< Menu >");
    display.setFont(ArialMT_Plain_16);
    display.drawString(64, 25, calculatorString[LANGUAGE]);
    display.setFont(ArialMT_Plain_10);
    display.drawString(64, 45, "No Warranty Edition");
    display.display();

    if (encoderState == 1)
    {
      Menu--;
    }
    if (encoderState == 2)
    {
      Menu++;
    }

    if (buttonState == 2)
    {
      Menu = Rechner_Menu;
    }
    break;

    // Pong Auswahl *********************************************************
  case Pong_Auswahl:
    display.clear();
    display.setTextAlignment(TEXT_ALIGN_CENTER);
    display.setFont(ArialMT_Plain_24);
    display.drawString(64, 0, "< Menu >");
    display.setFont(ArialMT_Plain_16);
    display.drawString(64, 25, "P  O  N  G");
    display.setFont(ArialMT_Plain_10);
    display.drawString(64, 45, "TheDIYGuy999 Edition");
    display.display();

    if (encoderState == 1)
    {
      Menu--;
    }
    if (encoderState == 2)
    {
      Menu++;
    }

    if (buttonState == 2)
    {
      Menu = Pong_Menu;
    }
    break;

    // Flappy Birds Auswahl *********************************************************
  case Flappy_Birds_Auswahl:
    display.clear();
    display.setTextAlignment(TEXT_ALIGN_CENTER);
    display.setFont(ArialMT_Plain_24);
    display.drawString(64, 0, "< Menu >");
    display.setFont(ArialMT_Plain_16);
    display.drawString(64, 25, "Flappy Birds");
    display.setFont(ArialMT_Plain_10);
    display.drawString(64, 45, "TheDIYGuy999 Edition");
    display.display();

    disableButtonRead = false; // Re enable regular button read function

    if (encoderState == 1)
    {
      Menu--;
    }
    if (encoderState == 2)
    {
      Menu++;
    }

    if (buttonState == 2)
    {
      Menu = Flappy_Birds_Menu;
    }
    break;

  // Einstellung Auswahl *********************************************************
  case Einstellung_Auswahl:
    display.clear();
    display.setTextAlignment(TEXT_ALIGN_CENTER);
    display.setFont(ArialMT_Plain_24);
    display.drawString(64, 0, "< Menu  ");
    display.setFont(ArialMT_Plain_16);
    display.drawString(64, 25, settingsString[LANGUAGE]);
    display.display();

    if (encoderState == 1)
    {
      Menu--;
    }
    if (encoderState == 2)
    {
      Menu = Einstellung_Auswahl;
    }

    if (buttonState == 2)
    {
      Menu = Einstellung_Menu;
      Einstellung = 5; // Pre select Servo frequency setting
    }
    break;

  // Untermenus ================================================================

  // Servotester *********************************************************
  case Servotester_Menu:
    display.clear();
    display.setTextAlignment(TEXT_ALIGN_LEFT);
    display.setFont(ArialMT_Plain_10);
    display.drawString(0, 10, "Hz");
    display.drawString(0, 20, String(SERVO_Hz));
    display.drawString(0, 35, servoMode);
    display.setTextAlignment(TEXT_ALIGN_RIGHT);
    display.drawString(128, 10, "°");
    display.drawString(128, 20, String(us2degree(servo_pos[selectedServo])));
    display.setTextAlignment(TEXT_ALIGN_CENTER);
    display.setFont(ArialMT_Plain_24);
    display.drawString(64, 0, "Servo" + String(selectedServo + 1));
    display.drawString(64, 25, String(servo_pos[selectedServo]) + "µs");
    display.drawProgressBar(8, 50, 112, 10, (((servo_pos[selectedServo] - SERVO_MIN) * 100) / (SERVO_MAX - SERVO_MIN)));
    display.display();
    if (!SetupMenu)
    {
      servo_pos[0] = SERVO_CENTER;
      servo_pos[1] = SERVO_CENTER;
      servo_pos[2] = SERVO_CENTER;
      servo_pos[3] = SERVO_CENTER;
      servo_pos[4] = SERVO_CENTER;
      setupMcpwm();
      SetupMenu = true;
    }
    mcpwm_set_duty_in_us(MCPWM_UNIT_0, MCPWM_TIMER_0, MCPWM_OPR_A, servo_pos[0]);
    mcpwm_set_duty_in_us(MCPWM_UNIT_0, MCPWM_TIMER_0, MCPWM_OPR_B, servo_pos[1]);
    mcpwm_set_duty_in_us(MCPWM_UNIT_0, MCPWM_TIMER_1, MCPWM_OPR_A, servo_pos[2]);
    mcpwm_set_duty_in_us(MCPWM_UNIT_0, MCPWM_TIMER_1, MCPWM_OPR_B, servo_pos[3]);
    mcpwm_set_duty_in_us(MCPWM_UNIT_1, MCPWM_TIMER_0, MCPWM_OPR_A, servo_pos[4]);

    if (encoderState == 1) // Left turn
    {
      servo_pos[selectedServo] = servo_pos[selectedServo] - (SERVO_STEPS * encoderSpeed); // Variable encoder speed
    }
    if (encoderState == 2) // Right turn
    {
      servo_pos[selectedServo] = servo_pos[selectedServo] + (SERVO_STEPS * encoderSpeed);
    }

    if (servo_pos[selectedServo] > SERVO_MAX) // Servo MAX
    {
      servo_pos[selectedServo] = SERVO_MAX;
    }
    else if (servo_pos[selectedServo] < SERVO_MIN) // Servo MIN
    {
      servo_pos[selectedServo] = SERVO_MIN;
    }

    if (buttonState == 1)
    {
      Menu = Servotester_Auswahl;
      SetupMenu = false;
      selectedServo = 0;
    }

    if (buttonState == 2)
    {
      servo_pos[selectedServo] = SERVO_CENTER; // Servo Mitte
    }

    if (buttonState == 3)
    {
      selectedServo++; // Servo +
    }

    if (selectedServo > 4)
    {
      selectedServo = 0;
    }
    break;

  // Automatik Modus *********************************************************
  case Automatik_Modus_Menu:
    static unsigned long autoMenuMillis;
    int autoChange;
    if (millis() - autoMenuMillis > 20)
    { // Every 20ms (slow screen refresh down, servo movement is too slow otherwise!)
      autoMenuMillis = millis();
      display.clear();
      display.setTextAlignment(TEXT_ALIGN_LEFT);
      display.setFont(ArialMT_Plain_10);
      display.drawString(0, 0, delayString[LANGUAGE]);
      display.drawString(0, 10, String(TimeAuto));
      display.drawString(0, 20, "ms");
      display.drawString(0, 35, servoMode);
      display.setTextAlignment(TEXT_ALIGN_RIGHT);
      display.drawString(128, 10, "°");
      display.drawString(128, 20, String(us2degree(servo_pos[selectedServo])));
      display.drawString(128, 35, String(SERVO_Hz));
      display.setTextAlignment(TEXT_ALIGN_CENTER);
      display.setFont(ArialMT_Plain_24);
      display.drawString(64, 0, "Servo" + String(selectedServo + 1));
      display.drawString(64, 25, String(servo_pos[selectedServo]) + "µs");
      if (Auto_Pause)
      {
        display.setFont(ArialMT_Plain_16);
        display.drawString(64, 48, "Pause");
      }
      else
      {
        display.drawProgressBar(8, 50, 112, 10, (((servo_pos[selectedServo] - SERVO_MIN) * 100) / (SERVO_MAX - SERVO_MIN)));
      }
      display.display();
    }

    if (!SetupMenu)
    {
      servo_pos[0] = SERVO_CENTER;
      servo_pos[1] = SERVO_CENTER;
      servo_pos[2] = SERVO_CENTER;
      servo_pos[3] = SERVO_CENTER;
      servo_pos[4] = SERVO_CENTER;
      setupMcpwm();
      TimeAuto = 50;      // Zeit für SERVO Steps +-
      Auto_Pause = false; // Pause aus
      SetupMenu = true;
    }

    currentTimeAuto = millis();
    if (!Auto_Pause)
    {
      if ((currentTimeAuto - previousTimeAuto) > TimeAuto)
      {
        previousTimeAuto = currentTimeAuto;
        if (Autopos[selectedServo] > ((SERVO_MIN + SERVO_MAX) / 2))
        {
          servo_pos[selectedServo] = servo_pos[selectedServo] + SERVO_STEPS;
        }
        else
        {
          servo_pos[selectedServo] = servo_pos[selectedServo] - SERVO_STEPS;
        }
        mcpwm_set_duty_in_us(MCPWM_UNIT_0, MCPWM_TIMER_0, MCPWM_OPR_A, servo_pos[0]);
        mcpwm_set_duty_in_us(MCPWM_UNIT_0, MCPWM_TIMER_0, MCPWM_OPR_B, servo_pos[1]);
        mcpwm_set_duty_in_us(MCPWM_UNIT_0, MCPWM_TIMER_1, MCPWM_OPR_A, servo_pos[2]);
        mcpwm_set_duty_in_us(MCPWM_UNIT_0, MCPWM_TIMER_1, MCPWM_OPR_B, servo_pos[3]);
        mcpwm_set_duty_in_us(MCPWM_UNIT_1, MCPWM_TIMER_0, MCPWM_OPR_A, servo_pos[4]);
      }
    }

    if (servo_pos[selectedServo] < SERVO_MIN)
    {
      Autopos[selectedServo] = SERVO_MAX;
      servo_pos[selectedServo] = SERVO_MIN;
    }
    if (servo_pos[selectedServo] > SERVO_MAX)
    {
      Autopos[selectedServo] = SERVO_MIN;
      servo_pos[selectedServo] = SERVO_MAX;
    }
    autoChange = map(TimeAuto, 0, 100, 1, 10); // Calculate adjustment step
    if (encoderState == 1)
    {
      if (Auto_Pause)
      {
        servo_pos[selectedServo] = servo_pos[selectedServo] - SERVO_STEPS;
      }
      else
      {
        TimeAuto -= autoChange;
      }
    }
    if (encoderState == 2)
    {
      if (Auto_Pause)
      {
        servo_pos[selectedServo] = servo_pos[selectedServo] + SERVO_STEPS;
      }
      else
      {
        TimeAuto += autoChange;
      }
    }

    if (TimeAuto > 100) // TimeAuto MAX
    {
      TimeAuto = 100;
    }
    else if (TimeAuto < 0) // TimeAuto MIN
    {
      TimeAuto = 0;
    }

    if (selectedServo > 4)
    {
      selectedServo = 4;
    }
    else if (selectedServo < 0)
    {
      selectedServo = 0;
    }

    if (buttonState == 1) // Long press = back
    {
      Menu = Automatik_Modus_Auswahl;
      SetupMenu = false;
      selectedServo = 0;
    }

    if (buttonState == 2) // Short press = pause
    {
      Auto_Pause = !Auto_Pause;
    }

    if (buttonState == 3) // Doubleclick = Change channel
    {
      selectedServo++; // Servo +
    }

    if (selectedServo > 4)
    {
      selectedServo = 0;
    }
    break;

  // PWM Impuls lesen *********************************************************
  case Impuls_lesen_Menu:

    if (!SetupMenu)
    {
      pinMode(servopin[0], INPUT);
      pinMode(servopin[1], INPUT);
      pinMode(servopin[2], INPUT);
      pinMode(servopin[3], INPUT);
      pinMode(servopin[4], INPUT);
      SetupMenu = true;
    }

    servo_pos[selectedServo] = pulseIn(servopin[selectedServo], HIGH, 50000);

    if (servo_pos[selectedServo] < Impuls_min && servo_pos[selectedServo] > 100)
    {
      Impuls_min = servo_pos[selectedServo];
    }
    if (servo_pos[selectedServo] > Impuls_max)
    {
      Impuls_max = servo_pos[selectedServo];
    }

    display.clear();
    display.setTextAlignment(TEXT_ALIGN_CENTER);
    display.setFont(ArialMT_Plain_24);
    display.drawString(64, 0, impulseString[LANGUAGE] + String(selectedServo + 1));
    display.drawString(64, 25, String(servo_pos[selectedServo]) + "µs");
    if (servo_pos[selectedServo] < 1000)
      servo_pos[selectedServo] = 1000; // Make sure, that progressbar always is within range
    display.drawProgressBar(8, 50, 112, 10, (((servo_pos[selectedServo] - Impuls_min) * 100) / (Impuls_max - Impuls_min)));
    display.display();

    if (encoderState == 1)
    {
      selectedServo--;
      Impuls_min = 1000;
      Impuls_max = 2000;
    }
    if (encoderState == 2)
    {
      selectedServo++;
      Impuls_min = 1000;
      Impuls_max = 2000;
    }

    if (selectedServo > 4)
    {
      selectedServo = 4;
    }
    else if (selectedServo < 0)
    {
      selectedServo = 0;
    }

    if (buttonState == 1)
    {
      Menu = Impuls_lesen_Auswahl;
      SetupMenu = false;
      selectedServo = 0;
    }

    if (buttonState == 2)
    {
      Impuls_min = 1000;
      Impuls_max = 2000;
    }
    break;

  // Multiswitch lesen *********************************************************
  https: // www.modelltruck.net/showthread.php?54795-Futaba-Robbe-Multiswitch-Decoder-mit-Arduino
  case Multiswitch_lesen_Menu:
    static unsigned long multiswitchMenuMillis;
    if (millis() - multiswitchMenuMillis > 20)
    { // Every 20ms (slow screen refresh down, signal is unstable otherwise!) TODO, still unstable
      multiswitchMenuMillis = millis();
      display.clear();
      display.setTextAlignment(TEXT_ALIGN_LEFT);
      display.setFont(ArialMT_Plain_10);
      display.drawString(20, 0, String(value1[8]));
      display.drawString(64, 0, String(value1[0]) + "  " + String(value1[1]));
      display.drawString(64, 15, String(value1[2]) + "  " + String(value1[3]));
      display.drawString(64, 30, String(value1[4]) + "  " + String(value1[5]));
      display.drawString(64, 45, String(value1[6]) + "  " + String(value1[7]));
      display.drawString(5, 30, "Multiswitch");
      display.display();
    }

    if (!SetupMenu)
    {
      pinMode(servopin[4], INPUT);
      SetupMenu = true;
    }

    value1[8] = 1500;

    while (value1[8] > 1000) // Wait for the beginning of the frame (1000)
    {
      value1[8] = pulseIn(servopin[4], HIGH, 50000);
    }

    for (int x = 0; x <= kanaele - 1; x++) // Loop to store all the channel positions
    {
      value1[x] = pulseIn(servopin[4], HIGH, 50000);
    }

    if (buttonState == 1)
    {
      Menu = Multiswitch_lesen_Auswahl;
      SetupMenu = false;
    }

    break;

  // SBUS lesen *********************************************************
  case SBUS_lesen_Menu:
    display.clear();
    display.setTextAlignment(TEXT_ALIGN_RIGHT);
    display.setFont(ArialMT_Plain_10);

    if (!SetupMenu)
    {
      pinMode(servopin[0], INPUT);
      pinMode(servopin[1], INPUT);
      pinMode(servopin[2], INPUT);
      pinMode(servopin[3], INPUT);
      pinMode(servopin[4], INPUT);
      sBus.begin(COMMAND_RX, COMMAND_TX, SBUS_INVERTED); // begin SBUS communication with compatible receivers
      SetupMenu = true;
    }

    sBus.read();
    SBUSchannels = sBus.ch();

    display.drawString(32, 0, String(map(SBUSchannels[0], 172, 1811, 1000, 2000)));
    display.drawString(64, 0, String(map(SBUSchannels[1], 172, 1811, 1000, 2000)));
    display.drawString(96, 0, String(map(SBUSchannels[2], 172, 1811, 1000, 2000)));
    display.drawString(128, 0, String(map(SBUSchannels[3], 172, 1811, 1000, 2000)));

    display.drawString(32, 15, String(map(SBUSchannels[4], 172, 1811, 1000, 2000)));
    display.drawString(64, 15, String(map(SBUSchannels[5], 172, 1811, 1000, 2000)));
    display.drawString(96, 15, String(map(SBUSchannels[6], 172, 1811, 1000, 2000)));
    display.drawString(128, 15, String(map(SBUSchannels[7], 172, 1811, 1000, 2000)));

    display.drawString(32, 30, String(map(SBUSchannels[8], 172, 1811, 1000, 2000)));
    display.drawString(64, 30, String(map(SBUSchannels[9], 172, 1811, 1000, 2000)));
    display.drawString(96, 30, String(map(SBUSchannels[10], 172, 1811, 1000, 2000)));
    display.drawString(128, 30, String(map(SBUSchannels[11], 172, 1811, 1000, 2000)));

    display.drawString(32, 45, String(map(SBUSchannels[12], 172, 1811, 1000, 2000)));
    display.drawString(64, 45, String(map(SBUSchannels[13], 172, 1811, 1000, 2000)));
    display.drawString(96, 45, String(map(SBUSchannels[14], 172, 1811, 1000, 2000)));
    display.drawString(128, 45, String(map(SBUSchannels[15], 172, 1811, 1000, 2000)));
    display.display();

    if (buttonState == 1)
    {
      Menu = SBUS_lesen_Auswahl;
      SetupMenu = false;
    }
    break;

  // IBUS lesen *********************************************************
  case IBUS_lesen_Menu:
    display.clear();
    display.setTextAlignment(TEXT_ALIGN_RIGHT);
    display.setFont(ArialMT_Plain_10);
    display.drawString(32, 0, String(IBus.readChannel(0)));
    display.drawString(64, 0, String(IBus.readChannel(1)));
    display.drawString(96, 0, String(IBus.readChannel(2)));
    display.drawString(128, 0, String(IBus.readChannel(3)));

    display.drawString(32, 15, String(IBus.readChannel(4)));
    display.drawString(64, 15, String(IBus.readChannel(5)));
    display.drawString(96, 15, String(IBus.readChannel(6)));
    display.drawString(128, 15, String(IBus.readChannel(7)));

    display.drawString(32, 30, String(IBus.readChannel(8)));
    display.drawString(64, 30, String(IBus.readChannel(9)));
    display.drawString(96, 30, String(IBus.readChannel(10)));
    display.drawString(128, 30, String(IBus.readChannel(11)));

    display.drawString(32, 45, String(IBus.readChannel(12)));
    display.drawString(64, 45, String(IBus.readChannel(13)));
    // display.drawString( 96, 45,  String(IBus.readChannel(14)));
    // display.drawString(128, 45,  String(IBus.readChannel(15)));
    display.display();

    if (!SetupMenu)
    {
      pinMode(servopin[0], INPUT);
      pinMode(servopin[1], INPUT);
      pinMode(servopin[2], INPUT);
      pinMode(servopin[3], INPUT);
      pinMode(servopin[4], INPUT);
      IBus.begin(Serial2, IBUSBM_NOTIMER, COMMAND_RX, COMMAND_TX); // iBUS object connected to serial2 RX2 pin and use timer 1
      SetupMenu = true;
    }

    IBus.loop(); // call internal loop function to update the communication to the receiver

    if (buttonState == 1)
    {
      Menu = IBUS_lesen_Auswahl;
      SetupMenu = false;
    }
    break;

    // Oszilloskop *********************************************************
  case Oscilloscope_Menu:

    if (!SetupMenu) // This stuff is only executed once
    {
      pinMode(servopin[0], INPUT);
      pinMode(servopin[1], INPUT);
      pinMode(servopin[2], INPUT);
      pinMode(servopin[3], INPUT);
      pinMode(servopin[4], INPUT);
      pinMode(OSCILLOSCOPE_PIN, INPUT);
      SetupMenu = true;
    }

    oscilloscopeLoop(); // Loop oscilloscope code

    if (buttonState == 1) // Back
    {
      Menu = Oscilloscope_Auswahl;
      SetupMenu = false;
    }
    break;

    // Rechner *********************************************************
  case Rechner_Menu:

    static bool calculatorLeft;
    static bool calculatorRight;
    static bool calculatorSelect;
    calculator(calculatorLeft, calculatorRight, calculatorSelect); // Run calculator

    if (!SetupMenu) // This stuff is only executed once
    {

      SetupMenu = true;
    }

    if (encoderState == 1) // Left
    {
      calculatorLeft = true;
      calculatorRight = false;
    }
    else if (encoderState == 2) // Right
    {
      calculatorRight = true;
      calculatorLeft = false;
    }
    else // Cursor stop
    {
      calculatorLeft = false;
      calculatorRight = false;
    }

    if (buttonState == 1) // Back
    {
      Menu = Rechner_Auswahl;
      SetupMenu = false;
    }

    if (buttonState == 2) // Select
    {
      calculatorSelect = true;
    }
    else
    {
      calculatorSelect = false;
    }
    break;

    // Pong spielen *********************************************************
  case Pong_Menu:

    static bool paddleUp;
    static bool paddleDown;
    static bool pongReset;
    pong(paddleUp, paddleDown, pongReset, encoderSpeed); // Run pong game

    if (!SetupMenu) // This stuff is only executed once
    {

      SetupMenu = true;
    }

    if (encoderState == 1) // Paddle up
    {
      paddleUp = true;
      paddleDown = false;
    }
    else if (encoderState == 2) // Paddle down
    {
      paddleDown = true;
      paddleUp = false;
    }
    else // Paddle stop
    {
      paddleUp = false;
      paddleDown = false;
    }

    if (buttonState == 1) // Back
    {
      Menu = Pong_Auswahl;
      SetupMenu = false;
    }

    if (buttonState == 2) // Reset
    {
      pongReset = true;
    }
    else
    {
      pongReset = false;
    }
    break;

    // Flappy Birds spielen *********************************************************
  case Flappy_Birds_Menu:

    static bool flappyClick;
    static unsigned long lastClick = millis();

    disableButtonRead = true; // Disable button read function for Flappy Bird!

    flappyBirds(flappyClick); // Run flappy birds game

    if (!digitalRead(BUTTON_PIN)) // Button pressed
    {
      flappyClick = true;
    }
    else // Button released
    {
      flappyClick = false;
      lastClick = millis();
    }

    if (millis() - lastClick > Duration_long) // Back, if pressed long
    {
      Menu = Flappy_Birds_Auswahl;
      SetupMenu = false;
    }
    break;

  // Einstellung *********************************************************
  case Einstellung_Menu:
    display.clear();
    display.setTextAlignment(TEXT_ALIGN_CENTER);
    display.setFont(ArialMT_Plain_24);
    display.drawString(64, 0, settingsString[LANGUAGE]);
    display.setFont(ArialMT_Plain_16);
    switch (Einstellung)
    {
    case 0:
      display.drawString(64, 25, "Wifi");
      if (WIFI_ON == 1)
      {
        display.drawString(64, 45, onString[LANGUAGE]);
      }
      else
      {
        display.drawString(64, 45, offString[LANGUAGE]);
      }
      break;
    case 1:
      display.drawString(64, 25, servoStepsString[LANGUAGE]);
      display.drawString(64, 45, String(SERVO_STEPS));
      break;
    case 2:
      display.drawString(64, 25, servoMaxString[LANGUAGE]);
      display.drawString(64, 45, String(SERVO_MAX));
      break;
    case 3:
      display.drawString(64, 25, servoMinString[LANGUAGE]);
      display.drawString(64, 45, String(SERVO_MIN));
      break;
    case 4:
      display.drawString(64, 25, servoCenterString[LANGUAGE]);
      display.drawString(64, 45, String(SERVO_CENTER));
      break;
    case 5:
      display.drawString(64, 25, servoHzString[LANGUAGE]);
      display.drawString(64, 45, String(SERVO_Hz));
      display.setTextAlignment(TEXT_ALIGN_LEFT);
      display.setFont(ArialMT_Plain_10);
      display.drawString(0, 27, "µs");
      display.drawString(0, 37, String(SERVO_MIN));
      display.setTextAlignment(TEXT_ALIGN_RIGHT);
      display.drawString(128, 27, "µs");
      display.drawString(128, 37, String(SERVO_MAX));
      display.drawString(128, 50, servoMode);

      break;
    case 6:
      display.drawString(64, 25, PowerScaleString[LANGUAGE]);
      display.drawString(64, 45, String(POWER_SCALE));
      display.setFont(ArialMT_Plain_10);
      display.setTextAlignment(TEXT_ALIGN_RIGHT);
      display.drawString(110, 50, String(batteryVoltage));
      display.setTextAlignment(TEXT_ALIGN_LEFT);
      display.drawString(110, 50, "V");
      break;
    case 7:
      display.drawString(64, 25, "SBUS");
      if (SBUS_INVERTED == 1)
      {
        display.drawString(64, 45, standardString[LANGUAGE]);
      }
      else
      {
        display.drawString(64, 45, inversedString[LANGUAGE]);
      }
      break;
    case 8:
      display.drawString(64, 25, encoderDirectionString[LANGUAGE]);
      if (ENCODER_INVERTED == 0)
      {
        display.drawString(64, 45, standardString[LANGUAGE]);
      }
      else
      {
        display.drawString(64, 45, inversedString[LANGUAGE]);
      }
      break;
    case 9:
      display.drawString(64, 25, languageString[LANGUAGE]);
      display.drawString(64, 45, languagesString[LANGUAGE]);
      break;
    case 10:
      display.drawString(64, 25, pongBallRateString[LANGUAGE]);
      display.drawString(64, 45, String(PONG_BALL_RATE));
      break;
    }
    if (Edit)
    {
      display.drawString(10, 50, "->");
    }
    display.display();

    if (!SetupMenu)
    {

      SetupMenu = true;
    }

    if (encoderState == 1) // Encoder left turn
    {
      if (!Edit)
      {
        Einstellung--;
      }
      else
      {
        switch (Einstellung)
        {
        case 0:
          WIFI_ON--;
          break;
        case 1:
          SERVO_STEPS--;
          break;
        case 2:
          SERVO_MAX--;
          break;
        case 3:
          SERVO_MIN--;
          break;
        case 4:
          SERVO_CENTER--;
          break;
        case 5:
          SERVO_MODE--;
          break;
        case 6:
          POWER_SCALE--;
          break;
        case 7:
          SBUS_INVERTED--;
          break;
        case 8:
          ENCODER_INVERTED--;
          break;
        case 9:
          LANGUAGE--;
          break;
        case 10:
          PONG_BALL_RATE--;
          break;
        }
      }
    }
    if (encoderState == 2) // Encoder right turn
    {
      if (!Edit)
      {
        Einstellung++;
      }
      else
      {
        switch (Einstellung)
        {
        case 0:
          WIFI_ON++;
          break;
        case 1:
          SERVO_STEPS++;
          break;
        case 2:
          SERVO_MAX++;
          break;
        case 3:
          SERVO_MIN++;
          break;
        case 4:
          SERVO_CENTER++;
          break;
        case 5:
          SERVO_MODE++;
          break;
        case 6:
          POWER_SCALE++;
          break;
        case 7:
          SBUS_INVERTED++;
          break;
        case 8:
          ENCODER_INVERTED++;
          break;
        case 9:
          LANGUAGE++;
          break;
        case 10:
          PONG_BALL_RATE++;
          break;
        }
      }
    }

    // Menu range
    if (Einstellung > 10)
    {
      Einstellung = 0;
    }
    else if (Einstellung < 0)
    {
      Einstellung = 10;
    }

    // Limits
    if (PONG_BALL_RATE < 1)
    { // Pong ball rate nicht unter 1
      PONG_BALL_RATE = 1;
    }

    if (PONG_BALL_RATE > 4)
    { // Pong ball rate nicht über 4
      PONG_BALL_RATE = 4;
    }

    if (LANGUAGE < 0)
    { // Language nicht unter 0
      LANGUAGE = 0;
    }

    if (LANGUAGE > noOfLanguages)
    { // Language nicht über 1
      LANGUAGE = noOfLanguages;
    }

    if (ENCODER_INVERTED < 0)
    { // Encoder inverted nicht unter 0
      ENCODER_INVERTED = 0;
    }

    if (ENCODER_INVERTED > 1)
    { // Encoder inverted nicht über 1
      ENCODER_INVERTED = 1;
    }

    if (SBUS_INVERTED < 0)
    { // SBUS inverted nicht unter 0
      SBUS_INVERTED = 0;
    }

    if (SBUS_INVERTED > 1)
    { // SBUS inverted nicht über 1
      SBUS_INVERTED = 1;
    }

    if (WIFI_ON < 0)
    { // Wifi off nicht unter 0
      WIFI_ON = 0;
    }

    if (WIFI_ON > 1)
    { // Wifi on nicht über 1
      WIFI_ON = 1;
    }

    // Servos
    if (SERVO_STEPS < 1)
    { // Steps nicht unter 0
      SERVO_STEPS = 1;
    }

    if (SERVO_STEPS > 50)
    { // Steps nicht über 50
      SERVO_STEPS = 50;
    }

    if (SERVO_MAX < SERVO_CENTER)
    { // Max nicht unter Mitte
      SERVO_MAX = SERVO_CENTER + 1;
    }

    if (SERVO_MAX > 2500)
    { // Max nicht über 2500
      SERVO_MAX = 2500;
    }

    if (SERVO_MIN > SERVO_CENTER)
    { // Min nicht über Mitte
      SERVO_MIN = SERVO_CENTER - 1;
    }

    if (SERVO_MIN < 50)
    { // Min nicht unter 50
      SERVO_MIN = 50;
    }

    if (SERVO_CENTER > SERVO_MAX)
    { // Mitte nicht über Max
      SERVO_CENTER = SERVO_MAX - 1;
    }

    if (SERVO_CENTER < SERVO_MIN)
    { // Mitte nicht unter Min
      SERVO_CENTER = SERVO_MIN + 1;
    }

    servoModes(); // Refresh servo operation mode

    // Buttons -----------
    if (buttonState == 1)
    {
      Menu = Einstellung_Auswahl;
      SetupMenu = false;
    }

    if (buttonState == 2)
    {
      if (Edit)
      {
        Edit = false;
        // Speichern
        eepromWrite();
      }
      else
      {
        Edit = true;
      }
    }
    break;

  default:
    // Tue etwas, im Defaultfall
    // Dieser Fall ist optional
    Menu = Servotester_Auswahl;
    break; // Wird nicht benötigt, wenn Statement(s) vorhanden sind
  }
}

// =======================================================================================================
// BATTERY MONITOR
// =======================================================================================================
//
void batteryVolts()
{

  currentTimeSpan = millis();
  if ((currentTimeSpan - previousTimeSpan) > 2000)
  {
    previousTimeSpan = currentTimeSpan;

    batteryVoltage = battery.readVoltage() * 0.825; // We want the same POWER_SCALE as before with analogRead()

    float scale_value = POWER_SCALE / 100.0;
    batteryVoltage = batteryVoltage * scale_value;

    Serial.print(eepromVoltageString[LANGUAGE]);
    Serial.println(batteryVoltage, 3);

    batteryDetected = 1;

    if (batteryVoltage > 21.25) // 6s Lipo
    {
      numberOfBatteryCells = 6;
    }
    else if (batteryVoltage > 17.0) // 5s Lipo
    {
      numberOfBatteryCells = 5;
    }
    else if (batteryVoltage > 12.75) // 4s Lipo
    {
      numberOfBatteryCells = 4;
    }
    else if (batteryVoltage > 8.5) // 3s Lipo
    {
      numberOfBatteryCells = 3;
    }
    else if (batteryVoltage > 6.0) // 2s Lipo
    {
      numberOfBatteryCells = 2;
    }
    else // 1s Lipo kein Akku angeschlossen
    {
      numberOfBatteryCells = 1;
      batteryDetected = 0;
    }

    batteryChargePercentage = (batteryVoltage / numberOfBatteryCells); // Prozentanzeige
    batteryChargePercentage = map_float(batteryChargePercentage, 3.5, 4.2, 0, 100);
    if (batteryChargePercentage < 0)
    {
      batteryChargePercentage = 0;
    }
    if (batteryChargePercentage > 100)
    {
      batteryChargePercentage = 100;
    }
  }
}

//
// =======================================================================================================
// EEPROM
// =======================================================================================================
//

// Init new board with the default values you want ------
void eepromInit()
{
  Serial.println(SERVO_MIN);
  if (SERVO_MIN < 50)
  {
    WIFI_ON = 1; // Wifi on
    SERVO_STEPS = 10;
    SERVO_MAX = 2000;
    SERVO_MIN = 1000;
    SERVO_CENTER = 1500;
    SERVO_Hz = 50;
    POWER_SCALE = 948;
    SBUS_INVERTED = 1; // 1 = Standard signal!
    ENCODER_INVERTED = 0;
    LANGUAGE = 0;
    PONG_BALL_RATE = 1;
    SERVO_MODE = STD;
    Serial.println(eepromInitString[LANGUAGE]);
    eepromWrite();
  }
}

// Write new values to EEPROM ------
void eepromWrite()
{
  EEPROM.writeInt(adr_eprom_WIFI_ON, WIFI_ON);
  EEPROM.writeInt(adr_eprom_SERVO_STEPS, SERVO_STEPS);
  EEPROM.writeInt(adr_eprom_SERVO_MAX, SERVO_MAX);
  EEPROM.writeInt(adr_eprom_SERVO_MIN, SERVO_MIN);
  EEPROM.writeInt(adr_eprom_SERVO_CENTER, SERVO_CENTER);
  EEPROM.writeInt(adr_eprom_SERVO_Hz, SERVO_Hz);
  EEPROM.writeInt(adr_eprom_POWER_SCALE, POWER_SCALE);
  EEPROM.writeInt(adr_eprom_SBUS_INVERTED, SBUS_INVERTED);
  EEPROM.writeInt(adr_eprom_ENCODER_INVERTED, ENCODER_INVERTED);
  EEPROM.writeInt(adr_eprom_LANGUAGE, LANGUAGE);
  EEPROM.writeInt(adr_eprom_PONG_BALL_RATE, PONG_BALL_RATE);
  EEPROM.writeInt(adr_eprom_SERVO_MODE, SERVO_MODE);

  EEPROM.commit();
  Serial.println(eepromWrittenString[LANGUAGE]);
}

// Read values from EEPROM ------
void eepromRead()
{
  WIFI_ON = EEPROM.readInt(adr_eprom_WIFI_ON);
  SERVO_STEPS = EEPROM.readInt(adr_eprom_SERVO_STEPS);
  SERVO_MAX = EEPROM.readInt(adr_eprom_SERVO_MAX);
  SERVO_MIN = EEPROM.readInt(adr_eprom_SERVO_MIN);
  SERVO_CENTER = EEPROM.readInt(adr_eprom_SERVO_CENTER);
  SERVO_Hz = EEPROM.readInt(adr_eprom_SERVO_Hz);
  POWER_SCALE = EEPROM.readInt(adr_eprom_POWER_SCALE);
  SBUS_INVERTED = EEPROM.readInt(adr_eprom_SBUS_INVERTED);
  ENCODER_INVERTED = EEPROM.readInt(adr_eprom_ENCODER_INVERTED);
  LANGUAGE = EEPROM.readInt(adr_eprom_LANGUAGE);
  PONG_BALL_RATE = EEPROM.readInt(adr_eprom_PONG_BALL_RATE);
  SERVO_MODE = EEPROM.readInt(adr_eprom_SERVO_MODE);
  Serial.println(eepromReadString[LANGUAGE]);
  Serial.println(WIFI_ON);
  Serial.println(SERVO_STEPS);
  Serial.println(SERVO_MAX);
  Serial.println(SERVO_MIN);
  Serial.println(SERVO_CENTER);
  Serial.println(POWER_SCALE);
  Serial.println(SBUS_INVERTED);
  Serial.println(ENCODER_INVERTED);
  if (LANGUAGE < 0) // Make sure, language is in correct range, otherwise device will crash!
    LANGUAGE = 0;
  if (LANGUAGE > noOfLanguages)
    LANGUAGE = noOfLanguages;
  Serial.println(LANGUAGE);
  Serial.println(PONG_BALL_RATE);
  Serial.println(SERVO_MODE);
}

//
// =======================================================================================================
// MAIN LOOP, RUNNING ON CORE 1
// =======================================================================================================
//

void loop()
{

  ButtonRead();
  beep();
  MenuUpdate();
  batteryVolts();
  webInterface();
  // Serial.print(loopDuration());
}
