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
   Drawback for now:
   - Unstable Multiswitch readings, see comment below. << ---------------------------- ! ! ! !

 ESP32 + Encoder + OLED

 /////Pin Belegung////
 GPIO 13: Servo1 & BUS Input
 GPIO 14: Servo2
 GPIO 27: Servo3
 GPIO 33: Servo4
 GPIO 32: Servo5
 GPIO 15: Ecoder Taster
 GPIO 16: Ecoder Richtung 1
 GPIO 17: Ecoder Richtung 2

 GPIO 21: SDA OLED
 GPIO 22: SDL OLED
 */

char codeVersion[] = "0.9.0"; // Software revision.

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
ESP32                                         2.0.5 <<----- NOTE! this is the only version, which is not causing unstable Multiswitch readings!
unfortunately it is not compatible with VS Code, so you have to use Arduino for now. The pre compiled .bin images were made with Arduino.
 */

/* Required Libraries / Benötigte Bibliotheken
ESP32Servo                                    0.12.1
ESP32Encoder                                  0.10.1
SBUS                                          internal, no library required anymore
IBusBM                                        1.1.5
ESP8266 and ESP OLED driver SSD1306 displays  4.3.0
ESP32AnalogRead                               0.2.1
 */

#include <ESP32Servo.h>      // https://github.com/madhephaestus/ESP32Servo/archive/refs/tags/0.12.1.tar.gz
#include <ESP32Encoder.h>    // https://github.com/madhephaestus/ESP32Encoder/archive/refs/tags/0.10.1.tar.gz
#include <IBusBM.h>          // https://github.com/bmellink/IBusBM/archive/refs/tags/v1.1.5.tar.gz
#include <SH1106Wire.h>      //1.3"
#include <SSD1306Wire.h>     //0.96" https://github.com/ThingPulse/esp8266-oled-ssd1306/archive/refs/tags/4.3.0.tar.gz
#include <ESP32AnalogRead.h> // https://github.com/madhephaestus/ESP32AnalogRead <<------- required for calibrated battery voltage measurement

// No need to install these, they come with the ESP32 board definition
#include <WiFi.h>
#include <EEPROM.h>
#include <Esp.h>     // for displaying memory information
#include "rom/rtc.h" // for displaying reset reason

// Project specific includes
#if not defined ALTERNATIVE_LOGO
#include "src/images.h" // Startlogo Der RC Modellbauer
#else
#include "src/images2.h" // Alternative Logo
#endif
#include "src/sbus.h"      // For SBUS interface
#include "src/languages.h" // Menu language ressources

// EEPROM
#define EEPROM_SIZE 40

#define adr_eprom_WIFI_ON 0           // WIFI 1 = Ein 0 = Aus
#define adr_eprom_SERVO_STEPS 4       // SERVO Steps für Encoder im Servotester Modus
#define adr_eprom_SERVO_MAX 8         // SERVO µs Max Wert im Servotester Modus
#define adr_eprom_SERVO_MIN 12        // SERVO µs Min Wert im Servotester Modus
#define adr_eprom_SERVO_Mitte 16      // SERVO µs Mitte Wert im Servotester Modus
#define adr_eprom_SERVO_Hz 20         // SERVO µs Mitte Wert im Servotester Modus
#define adr_eprom_POWER_SCALE 24      // SERVO µs Mitte Wert im Servotester Modus
#define adr_eprom_SBUS_INVERTED 28    // SBUS inverted
#define adr_eprom_ENCODER_INVERTED 32 // Encoder inverted
#define adr_eprom_LANGUAGE 36         // SBUS inverted

// Speicher der Einstellungen
int WIFI_ON;
int SERVO_STEPS;
int SERVO_MAX;
int SERVO_MIN;
int SERVO_Mitte;
int SERVO_Hz;
int POWER_SCALE;
int SBUS_INVERTED;
int ENCODER_INVERTED;
int LANGUAGE;

// Encoder + button
ESP32Encoder encoder;

#define BUTTON_PIN 15         // Hardware Pin Button
#define ENCODER_PIN_1 16      // Hardware Pin1 Encoder
#define ENCODER_PIN_2 17      // Hardware Pin2 Encoder
long prev = 0;                // Zeitspeicher für Taster
long prev2 = 0;               // Zeitspeicher für Taster
long previousDebouncTime = 0; // Speicher Entprellzeit für Taster
int buttonState = 0;          // 0 = Taster nicht betätigt; 1 = Taster langer Druck; 2 = Taster kurzer Druck; 3 = Taster Doppelklick
int encoderState = 0;         // 1 = Drehung nach Links (-); 2 = Drehung nach Rechts (+)
int Duration_long = 600;      // Zeit für langen Druck
int Duration_double = 200;    // Zeit für Doppelklick
int bouncing = 50;            // Zeit für Taster Entprellung
int encoder_last;             // Speicher letzer Wert Encoder
int encoder_read;             // Speicher aktueller Wert Encoder

// Servo
volatile unsigned char servopin[5] = {13, 14, 27, 33, 32}; // Pins Servoausgang
Servo servo[5];                                            // Servo Objekte erzeugen
int servo_pos[5];                                          // Speicher für Servowerte
int selectedServo = 0;                                     // Das momentan angesteuerte Servo

// Serial command pins for SBUS, IBUS -----
#define COMMAND_RX 13 // pin 13
#define COMMAND_TX -1 // -1 is just a dummy

// SBUS
bfs::SbusRx sBus(&Serial2);
std::array<int16_t, bfs::SbusRx::NUM_CH()> SBUSchannels;

// IBUS
IBusBM IBus; // IBus object

// Externe Spannungsversorgung
#define BATTERY_DETECT_PIN 36  // Hardware Pin Externe Spannung in
bool batteryDetected;          // Akku vorhanden
int numberOfBatteryCells;      // Akkuzellen
float batteryVoltage;          // Akkuspannung in V
float batteryChargePercentage; // Akkuspannung in Prozent

// Menüstruktur
/*
 * 1 = Servotester_Auswahl       Auswahl -> 10 Servotester_Menu
 * 2 = Automatik_Modus_Auswahl   Auswahl -> 20 Automatik_Modus_Menu
 * 3 = Impuls_lesen_Auswahl      Auswahl -> 30 Impuls_lesen_Menu
 * 4 = Multiswitch_lesen_Auswahl Auswahl -> 40 Multiswitch_lesen_Menu
 * 5 = SBUS_lesen_Auswahl        Auswahl -> 50 SBUS_lesen_Menu
 * 6 = Einstellung_Auswahl       Auswahl -> 60 Einstellung_Menu
 */
enum
{
  Servotester_Auswahl = 1,
  Automatik_Modus_Auswahl = 2,
  Impuls_lesen_Auswahl = 3,
  Multiswitch_lesen_Auswahl = 4,
  SBUS_lesen_Auswahl = 5,
  IBUS_lesen_Auswahl = 6,
  Einstellung_Auswahl = 7,
  //
  Servotester_Menu = 10,
  Automatik_Modus_Menu = 20,
  Impuls_lesen_Menu = 30,
  Multiswitch_lesen_Menu = 40,
  SBUS_lesen_Menu = 50,
  IBUS_lesen_Menu = 60,
  Einstellung_Menu = 70
};

//-Menu 20 Automatik Modus

int Autopos[5];      // Speicher
bool Auto_Pause = 0; // Pause im Auto Modus

//-Menu 40 Multiwitch Futaba lesen

#define kanaele 9    // Anzahl der Kanäle
int value1[kanaele]; // Speicher

//-Menu 30 Impuls lesen
int Impuls_min = 1000;
int Impuls_max = 2000;

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
// SUB FUNCTIONS
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
  return (value - 500) / 11.111 - 90.0;
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
  Serial.printf("Sevotester Deluxe für ESP32 software version %s\n", codeVersion);
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

  // Battery
  battery.attach(BATTERY_DETECT_PIN);

  // SBUS
  // sBus.begin(COMMAND_RX, COMMAND_TX, SBUS_INVERTED); // begin SBUS communication with compatible receivers TODO

  // Setup OLED
  display.init();
  display.flipScreenVertically();
  display.setFont(ArialMT_Plain_10);
  display.drawXbm(0, 0, Logo_width, Logo_height, Logo_bits);
  display.display();
  delay(1500);

  display.clear();
  display.setTextAlignment(TEXT_ALIGN_LEFT);
  display.setFont(ArialMT_Plain_10);
  display.drawString(0, 0, operationString[LANGUAGE]);
  display.drawString(0, 12, shortPressString[LANGUAGE]);
  display.drawString(0, 24, longPressString[LANGUAGE]);
  display.drawString(0, 36, doubleclickString[LANGUAGE]);
  display.drawString(0, 48, RotateKnobString[LANGUAGE]);
  display.display();
  delay(5000);

  if (WIFI_ON == 1)
  { // Wifi Ein
    // Print local IP address and start web server
    Serial.println(connectingAccessPointString[LANGUAGE]);
    WiFi.softAP(ssid, password);

    IPAddress IP = WiFi.softAPIP();
    Serial.print(apIpAddressString[LANGUAGE]);
    Serial.println(IP);

    display.clear();
    display.setTextAlignment(TEXT_ALIGN_LEFT);
    display.setFont(ArialMT_Plain_16);
    display.drawString(0, 10, WiFiOnString[LANGUAGE]);
    display.drawString(0, 26, ipAddressString[LANGUAGE]);
    display.drawString(0, 42, IP.toString());
    display.display();
    delay(2000);

    display.clear();
    display.setTextAlignment(TEXT_ALIGN_LEFT);
    display.setFont(ArialMT_Plain_16); // was 16 size
    display.drawString(0, 0, "SSID: ");
    display.drawString(0, 16, ssid);
    display.drawString(0, 32, passwordString[LANGUAGE]);
    display.drawString(0, 48, password);
    display.display();
    delay(2500);

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
    delay(2000);
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
}

//
// =======================================================================================================
// ENCODER & BUTTON
// =======================================================================================================
//
void ButtonRead()
{

  buttonState = 0;
  if (!(digitalRead(BUTTON_PIN)))
  {                  // Button gedrückt 0
    delay(bouncing); // Taster entprellen
    prev = millis();
    buttonState = 1;
    while ((millis() - prev) <= Duration_long)
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
  }

  encoder_read = encoder.getCount();

  if (previousDebouncTime + 20 > millis())
  {
    encoder_last = encoder_read;
  }

  if (encoder_last > encoder_read)
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
  else if (encoder_last < encoder_read)
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
    encoder.setCount(1500); // set starting count value after attaching
    encoder_last = 1500;
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

  // Servotester Auswahl *********************************************************
  switch (Menu)
  {
  case Servotester_Auswahl:
    display.clear();
    display.setTextAlignment(TEXT_ALIGN_CENTER);
    display.setFont(ArialMT_Plain_24);
    display.drawString(64, 0, "  Menu >");
    display.setFont(ArialMT_Plain_16);
    display.drawString(64, 25, servotesterString[LANGUAGE]);
    display.setFont(ArialMT_Plain_10);
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
      Menu = 1;
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
    display.drawString(64, 45, readCh1String[LANGUAGE]);
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
    display.drawString(64, 45, "CH1");
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
    display.drawString(64, 45, "CH1");
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
    display.setTextAlignment(TEXT_ALIGN_RIGHT);
    display.drawString(128, 10, "°");
    display.drawString(128, 20, String(us2degree(servo_pos[selectedServo])));
    display.setTextAlignment(TEXT_ALIGN_CENTER);
    display.setFont(ArialMT_Plain_24);
    display.drawString(64, 0, "Servo" + String(selectedServo + 1));
    // display.drawString(64, 25, String(servo_pos[selectedServo]) + "°");
    // display.drawProgressBar(8, 50, 112, 10, ((servo_pos[selectedServo]*100)/180));
    display.drawString(64, 25, String(servo_pos[selectedServo]) + "µs");
    display.drawProgressBar(8, 50, 112, 10, (((servo_pos[selectedServo] - SERVO_MIN) * 100) / (SERVO_MAX - SERVO_MIN)));
    display.display();
    if (!SetupMenu)
    {
      servo[0].attach(servopin[0], SERVO_MIN, SERVO_MAX); // ServoPIN, MIN, MAX
      servo[1].attach(servopin[1], SERVO_MIN, SERVO_MAX); // ServoPIN, MIN, MAX
      servo[2].attach(servopin[2], SERVO_MIN, SERVO_MAX); // ServoPIN, MIN, MAX
      servo[3].attach(servopin[3], SERVO_MIN, SERVO_MAX); // ServoPIN, MIN, MAX
      servo[4].attach(servopin[4], SERVO_MIN, SERVO_MAX); // ServoPIN, MIN, MAX
      servo[0].setPeriodHertz(SERVO_Hz);
      SetupMenu = true;
    }

    servo[0].writeMicroseconds(servo_pos[0]);
    servo[1].writeMicroseconds(servo_pos[1]);
    servo[2].writeMicroseconds(servo_pos[2]);
    servo[3].writeMicroseconds(servo_pos[3]);
    servo[4].writeMicroseconds(servo_pos[4]);

    if (encoderState == 1)
    {
      servo_pos[selectedServo] = servo_pos[selectedServo] - SERVO_STEPS;
    }
    if (encoderState == 2)
    {
      servo_pos[selectedServo] = servo_pos[selectedServo] + SERVO_STEPS;
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
      servo[0].detach();
      servo[1].detach();
      servo[2].detach();
      servo[3].detach();
      servo[4].detach();
      selectedServo = 0;
    }

    if (buttonState == 2)
    {
      servo_pos[selectedServo] = SERVO_Mitte; // Servo Mitte
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
      display.setTextAlignment(TEXT_ALIGN_RIGHT);
      display.drawString(128, 10, "°");
      display.drawString(128, 20, String(us2degree(servo_pos[selectedServo])));
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
      servo[0].attach(servopin[0], SERVO_MIN, SERVO_MAX); // ServoPIN, MIN, MAX
      servo[1].attach(servopin[1], SERVO_MIN, SERVO_MAX); // ServoPIN, MIN, MAX
      servo[2].attach(servopin[2], SERVO_MIN, SERVO_MAX); // ServoPIN, MIN, MAX
      servo[3].attach(servopin[3], SERVO_MIN, SERVO_MAX); // ServoPIN, MIN, MAX
      servo[4].attach(servopin[4], SERVO_MIN, SERVO_MAX); // ServoPIN, MIN, MAX
      TimeAuto = 50;                                      // Zeit für SERVO Steps +-
      Auto_Pause = false;                                 // Pause aus
      SetupMenu = true;
    }

    currentTimeAuto = millis();
    if (!Auto_Pause)
    {
      if ((currentTimeAuto - previousTimeAuto) > TimeAuto)
      {
        previousTimeAuto = currentTimeAuto;
        if (Autopos[selectedServo] > 1500)
        {
          servo_pos[selectedServo] = servo_pos[selectedServo] + SERVO_STEPS;
        }
        else
        {
          servo_pos[selectedServo] = servo_pos[selectedServo] - SERVO_STEPS;
        }
        servo[selectedServo].writeMicroseconds(servo_pos[selectedServo]);
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
      servo[0].detach();
      servo[1].detach();
      servo[2].detach();
      servo[3].detach();
      servo[4].detach();
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
    display.clear();
    display.setTextAlignment(TEXT_ALIGN_CENTER);
    display.setFont(ArialMT_Plain_24);
    display.drawString(64, 0, impulseString[LANGUAGE] + String(selectedServo + 1));
    display.drawString(64, 25, String(servo_pos[selectedServo]) + "µs");
    display.drawProgressBar(8, 50, 112, 10, (((servo_pos[selectedServo] - Impuls_min) * 100) / (Impuls_max - Impuls_min)));
    display.display();
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

    if (servo_pos[selectedServo] < Impuls_min)
    {
      Impuls_min = servo_pos[selectedServo];
    }
    if (servo_pos[selectedServo] > Impuls_max)
    {
      Impuls_max = servo_pos[selectedServo];
    }

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
      display.display();
    }

    if (!SetupMenu)
    {
      pinMode(servopin[0], INPUT);
      SetupMenu = true;
    }

    value1[8] = 1500;

    while (value1[8] > 1000) // Wait for the beginning of the frame (1000)
    {
      value1[8] = pulseIn(servopin[0], HIGH, 50000);
    }

    for (int x = 0; x <= kanaele - 1; x++) // Loop to store all the channel positions
    {
      value1[x] = pulseIn(servopin[0], HIGH, 50000);
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

    if (!SetupMenu)
    {
      sBus.begin(COMMAND_RX, COMMAND_TX, SBUS_INVERTED); // begin SBUS communication with compatible receivers
      SetupMenu = true;
    }

    if (encoderState == 1)
    {
    }
    if (encoderState == 2)
    {
    }

    if (buttonState == 1)
    {
      Menu = SBUS_lesen_Auswahl;
      SetupMenu = false;
    }

    if (buttonState == 2)
    {
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
      IBus.begin(Serial2, IBUSBM_NOTIMER, COMMAND_RX, COMMAND_RX); // iBUS object connected to serial2 RX2 pin and use timer 1
      SetupMenu = true;
    }

    IBus.loop(); // call internal loop function to update the communication to the receiver

    if (encoderState == 1)
    {
    }
    if (encoderState == 2)
    {
    }

    if (buttonState == 1)
    {
      Menu = IBUS_lesen_Auswahl;
      SetupMenu = false;
    }

    if (buttonState == 2)
    {
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
      display.drawString(64, 45, String(SERVO_Mitte));
      break;
    case 5:
      display.drawString(64, 25, servoHzString[LANGUAGE]);
      display.drawString(64, 45, String(SERVO_Hz));
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
    }
    if (Edit)
    {
      display.drawString(10, 45, "->");
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
          SERVO_Mitte--;
          break;
        case 5:
          SERVO_Hz--;
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
          SERVO_Mitte++;
          break;
        case 5:
          SERVO_Hz++;
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
        }
      }
    }

    // Menu range
    if (Einstellung > 9)
    {
      Einstellung = 0;
    }
    else if (Einstellung < 0)
    {
      Einstellung = 9;
    }

    // Limits
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

    if (SERVO_STEPS < 0)
    { // Steps nicht unter 0
      SERVO_STEPS = 0;
    }

    if (SERVO_MAX < SERVO_Mitte)
    { // Max nicht unter Mitte
      SERVO_MAX = SERVO_Mitte + 1;
    }

    if (SERVO_MAX > 2500)
    { // Max nicht über 2500
      SERVO_MAX = 2500;
    }

    if (SERVO_MIN > SERVO_Mitte)
    { // Min nicht über Mitte
      SERVO_MIN = SERVO_Mitte - 1;
    }

    if (SERVO_MIN < 500)
    { // Min nicht unter 500
      SERVO_MIN = 500;
    }

    if (SERVO_Mitte > SERVO_MAX)
    { // Mitte nicht über Max
      SERVO_Mitte = SERVO_MAX - 1;
    }

    if (SERVO_Mitte < SERVO_MIN)
    { // Mitte nicht unter Min
      SERVO_Mitte = SERVO_MIN + 1;
    }

    if (SERVO_Hz == 51)
    {
      SERVO_Hz = 200;
    }

    if (SERVO_Hz == 201)
    {
      SERVO_Hz = 333;
    }

    if (SERVO_Hz == 334)
    {
      SERVO_Hz = 560;
      SERVO_MAX = 1000;
      SERVO_MIN = 500;
      SERVO_Mitte = 760;
    }

    if (SERVO_Hz == 561)
    {
      SERVO_Hz = 50;
      SERVO_MAX = 2000;
      SERVO_MIN = 1000;
      SERVO_Mitte = 1500;
    }

    if (SERVO_Hz == 49)
    {
      SERVO_Hz = 560;
      SERVO_MAX = 1000;
      SERVO_MIN = 500;
      SERVO_Mitte = 760;
    }

    if (SERVO_Hz == 559)
    {
      SERVO_Hz = 333;
      SERVO_MAX = 2000;
      SERVO_MIN = 1000;
      SERVO_Mitte = 1500;
    }

    if (SERVO_Hz == 332)
    {
      SERVO_Hz = 200;
    }

    if (SERVO_Hz == 199)
    {
      SERVO_Hz = 50;
    }

    if (SERVO_Hz == 560)
    {
      SERVO_MAX = 1000;
      SERVO_MIN = 500;
      SERVO_Mitte = 760;
    }

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
  if (SERVO_MIN < 500)
  {
    WIFI_ON = 1; // Wifi on
    SERVO_STEPS = 10;
    SERVO_MAX = 2000;
    SERVO_MIN = 1000;
    SERVO_Mitte = 1500;
    SERVO_Hz = 50;
    POWER_SCALE = 948;
    SBUS_INVERTED = 1; // 1 = Standard signal!
    ENCODER_INVERTED = 0;
    LANGUAGE = 0;
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
  EEPROM.writeInt(adr_eprom_SERVO_Mitte, SERVO_Mitte);
  EEPROM.writeInt(adr_eprom_SERVO_Hz, SERVO_Hz);
  EEPROM.writeInt(adr_eprom_POWER_SCALE, POWER_SCALE);
  EEPROM.writeInt(adr_eprom_SBUS_INVERTED, SBUS_INVERTED);
  EEPROM.writeInt(adr_eprom_ENCODER_INVERTED, ENCODER_INVERTED);
  EEPROM.writeInt(adr_eprom_LANGUAGE, LANGUAGE);
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
  SERVO_Mitte = EEPROM.readInt(adr_eprom_SERVO_Mitte);
  SERVO_Hz = EEPROM.readInt(adr_eprom_SERVO_Hz);
  POWER_SCALE = EEPROM.readInt(adr_eprom_POWER_SCALE);
  SBUS_INVERTED = EEPROM.readInt(adr_eprom_SBUS_INVERTED);
  ENCODER_INVERTED = EEPROM.readInt(adr_eprom_ENCODER_INVERTED);
  LANGUAGE = EEPROM.readInt(adr_eprom_LANGUAGE);
  Serial.println(eepromReadString[LANGUAGE]);
  Serial.println(WIFI_ON);
  Serial.println(SERVO_STEPS);
  Serial.println(SERVO_MAX);
  Serial.println(SERVO_MIN);
  Serial.println(SERVO_Mitte);
  Serial.println(POWER_SCALE);
  Serial.println(SBUS_INVERTED);
  Serial.println(ENCODER_INVERTED);
  if (LANGUAGE < 0) // Make sure, language is in correct range, otherwise device will crash!
    LANGUAGE = 0;
  if (LANGUAGE > noOfLanguages)
    LANGUAGE = noOfLanguages;
  Serial.println(LANGUAGE);
}

//
// =======================================================================================================
// WEB INTERFACE
// =======================================================================================================
//

void webInterface()
{

  if (WIFI_ON == 1)
  {                                         // Wifi Ein
    WiFiClient client = server.available(); // Listen for incoming clients

    if (client)
    { // If a new client connects,
      currentTime = millis();
      previousTime = currentTime;
      Serial.println("New Client."); // print a message out in the serial port
      String currentLine = "";       // make a String to hold incoming data from the client
      while (client.connected() && currentTime - previousTime <= timeoutTime)
      { // loop while the client's connected
        currentTime = millis();
        if (client.available())
        {                         // if there's bytes to read from the client,
          char c = client.read(); // read a byte, then
          Serial.write(c);        // print it out the serial monitor
          header += c;
          if (c == '\n')
          { // if the byte is a newline character
            // if the current line is blank, you got two newline characters in a row.
            // that's the end of the client HTTP request, so send a response:
            if (currentLine.length() == 0)
            {
              // HTTP-Header fangen immer mit einem Response-Code an (z.B. HTTP/1.1 200 OK)
              // gefolgt vom Content-Type damit der Client weiss was folgt, gefolgt von einer Leerzeile:
              client.println("HTTP/1.1 200 OK");
              client.println("Content-type:text/html");
              client.println("Connection: close");
              client.println();

              // Webseiten Eingaben abfragen

              // GET /?value=180& HTTP/1.1
              if (header.indexOf("GET /?Pos0=") >= 0)
              {
                pos1 = header.indexOf('=');
                pos2 = header.indexOf('&');
                valueString = header.substring(pos1 + 1, pos2);
                servo_pos[0] = (valueString.toInt());
              }
              if (header.indexOf("GET /?Pos1=") >= 0)
              {
                pos1 = header.indexOf('=');
                pos2 = header.indexOf('&');
                valueString = header.substring(pos1 + 1, pos2);
                servo_pos[1] = (valueString.toInt());
              }
              if (header.indexOf("GET /?Pos2=") >= 0)
              {
                pos1 = header.indexOf('=');
                pos2 = header.indexOf('&');
                valueString = header.substring(pos1 + 1, pos2);
                servo_pos[2] = (valueString.toInt());
              }
              if (header.indexOf("GET /?Pos3=") >= 0)
              {
                pos1 = header.indexOf('=');
                pos2 = header.indexOf('&');
                valueString = header.substring(pos1 + 1, pos2);
                servo_pos[3] = (valueString.toInt());
              }
              if (header.indexOf("GET /?Pos4=") >= 0)
              {
                pos1 = header.indexOf('=');
                pos2 = header.indexOf('&');
                valueString = header.substring(pos1 + 1, pos2);
                servo_pos[4] = (valueString.toInt());
              }
              if (header.indexOf("GET /?Set1=") >= 0)
              {
                pos1 = header.indexOf('=');
                pos2 = header.indexOf('&');
                valueString = header.substring(pos1 + 1, pos2);
                SERVO_STEPS = (valueString.toInt());
              }
              if (header.indexOf("GET /?Set2=") >= 0)
              {
                pos1 = header.indexOf('=');
                pos2 = header.indexOf('&');
                valueString = header.substring(pos1 + 1, pos2);
                SERVO_MAX = (valueString.toInt());
              }
              if (header.indexOf("GET /?Set3=") >= 0)
              {
                pos1 = header.indexOf('=');
                pos2 = header.indexOf('&');
                valueString = header.substring(pos1 + 1, pos2);
                SERVO_MIN = (valueString.toInt());
              }
              if (header.indexOf("GET /?Set4=") >= 0)
              {
                pos1 = header.indexOf('=');
                pos2 = header.indexOf('&');
                valueString = header.substring(pos1 + 1, pos2);
                if (valueString.toInt() > SERVO_MIN)
                { // Nur übernehmen wenn > MIN
                  SERVO_Mitte = (valueString.toInt());
                }
              }
              if (header.indexOf("GET /?Set5=") >= 0)
              {
                pos1 = header.indexOf('=');
                pos2 = header.indexOf('&');
                valueString = header.substring(pos1 + 1, pos2);
                SERVO_Hz = (valueString.toInt());
              }
              if (header.indexOf("GET /?Speed=") >= 0)
              {
                pos1 = header.indexOf('=');
                pos2 = header.indexOf('&');
                valueString = header.substring(pos1 + 1, pos2);
                TimeAuto = (valueString.toInt());
              }
              if (header.indexOf("GET /mitte1/on") >= 0)
              {
                servo_pos[0] = SERVO_Mitte; // Mitte
              }
              if (header.indexOf("GET /mitte2/on") >= 0)
              {
                servo_pos[1] = SERVO_Mitte; // Mitte
              }
              if (header.indexOf("GET /mitte3/on") >= 0)
              {
                servo_pos[2] = SERVO_Mitte; // Mitte
              }
              if (header.indexOf("GET /mitte4/on") >= 0)
              {
                servo_pos[3] = SERVO_Mitte; // Mitte
              }
              if (header.indexOf("GET /mitte5/on") >= 0)
              {
                servo_pos[4] = SERVO_Mitte; // Mitte
              }
              if (header.indexOf("GET /back/on") >= 0)
              {
                Menu = Servotester_Auswahl;
              }
              if (header.indexOf("GET /10/on") >= 0)
              {
                Menu = Servotester_Menu;
              }
              if (header.indexOf("GET /20/on") >= 0)
              {
                Menu = Automatik_Modus_Menu;
              }
              if (header.indexOf("GET /30/on") >= 0)
              {
                Menu = Impuls_lesen_Menu;
              }
              if (header.indexOf("GET /40/on") >= 0)
              {
                Menu = Multiswitch_lesen_Menu;
              }
              if (header.indexOf("GET /50/on") >= 0)
              {
                Menu = SBUS_lesen_Menu;
              }
              if (header.indexOf("GET /60/on") >= 0)
              {
                Menu = Einstellung_Menu;
              }
              if (header.indexOf("GET /save/on") >= 0)
              {
                eepromWrite();
              }
              if (header.indexOf("GET /pause/on") >= 0)
              {
                if (Auto_Pause)
                {
                  Auto_Pause = false;
                }
                else
                {
                  Auto_Pause = true;
                }
              }

              // HTML Seite angezeigen:
              client.println("<!DOCTYPE html><html>");
              // client.println("<meta http-equiv='refresh' content='5'>");
              client.println("<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">");
              client.println("<link rel=\"icon\" href=\"data:,\">");
              // CSS zum Stylen der Ein/Aus-Schaltflächen
              // Fühlen Sie sich frei, die Attribute für Hintergrundfarbe und Schriftgröße nach Ihren Wünschen zu ändern
              client.println("<style>html { font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: center;}");
              client.println(".button { border: yes; color: white; padding: 10px 40px; width: 100%;");
              client.println("text-decoration: none; font-size: 20px; margin: 2px; cursor: pointer;}");
              client.println(".slider { -webkit-appearance: none; width: 100%; height: 25px; background: #d3d3d3; outline: none; opacity: 0.7; -webkit-transition: .2s; transition: opacity .2s; }");
              client.println(".button1 {background-color: #4CAF50;}");
              client.println(".button2 {background-color: #ff0000;}");
              client.println(".textbox {font-size: 25px; text-align: center;}");
              client.println("</style></head>");

              // Webseiten-Überschrift
              client.println("</head><body><h1>Servotester Deluxe</h1>");

              switch (Menu)
              {
              case Servotester_Menu:
                client.println("<h2>Servotester</h2>");
                // Servo1
                valueString = String(servo_pos[0], DEC);

                client.println("<p><h3>Servo 1 Mikrosekunden : <span id=\"textServo1SliderValue\">" + valueString + "</span>");
                client.println("<a href=\"/mitte1/on\"><button class=\"button button1\">Mitte</button></a></p>");

                client.println("<input type=\"range\" min=\"" + String(SERVO_MIN, DEC) + "\" max=\"" + String(SERVO_MAX, DEC) + "\" step=\"10\" class=\"slider\" id=\"Servo1Slider\" onchange=\"Servo1Speed(this.value)\" value=\"" + valueString + "\" /></p>");

                client.println("<script> function Servo1Speed(pos) { ");
                client.println("var sliderValue = document.getElementById(\"Servo1Slider\").value;");
                client.println("document.getElementById(\"textServo1SliderValue\").innerHTML = sliderValue;");
                client.println("var xhr = new XMLHttpRequest();");
                client.println("xhr.open('GET', \"/?Pos0=\" + pos + \"&\", true);");
                client.println("xhr.send(); } </script>");
                // Servo2
                valueString = String(servo_pos[1], DEC);

                client.println("<p><h3>Servo 2 Mikrosekunden : <span id=\"textServo2SliderValue\">" + valueString + "</span>");
                client.println("<a href=\"/mitte2/on\"><button class=\"button button1\">Mitte</button></a></p>");

                client.println("<input type=\"range\" min=\"" + String(SERVO_MIN, DEC) + "\" max=\"" + String(SERVO_MAX, DEC) + "\" step=\"10\" class=\"slider\" id=\"Servo2Slider\" onchange=\"Servo2Speed(this.value)\" value=\"" + valueString + "\" /></p>");

                client.println("<script> function Servo2Speed(pos) { ");
                client.println("var sliderValue = document.getElementById(\"Servo2Slider\").value;");
                client.println("document.getElementById(\"textServo2SliderValue\").innerHTML = sliderValue;");
                client.println("var xhr = new XMLHttpRequest();");
                client.println("xhr.open('GET', \"/?Pos1=\" + pos + \"&\", true);");
                client.println("xhr.send(); } </script>");
                // Servo3
                valueString = String(servo_pos[2], DEC);

                client.println("<p><h3>Servo 3 Mikrosekunden : <span id=\"textServo3SliderValue\">" + valueString + "</span>");
                client.println("<a href=\"/mitte3/on\"><button class=\"button button1\">Mitte</button></a></p>");

                client.println("<input type=\"range\" min=\"" + String(SERVO_MIN, DEC) + "\" max=\"" + String(SERVO_MAX, DEC) + "\" step=\"10\" class=\"slider\" id=\"Servo3Slider\" onchange=\"Servo3Speed(this.value)\" value=\"" + valueString + "\" /></p>");

                client.println("<script> function Servo3Speed(pos) { ");
                client.println("var sliderValue = document.getElementById(\"Servo3Slider\").value;");
                client.println("document.getElementById(\"textServo3SliderValue\").innerHTML = sliderValue;");
                client.println("var xhr = new XMLHttpRequest();");
                client.println("xhr.open('GET', \"/?Pos2=\" + pos + \"&\", true);");
                client.println("xhr.send(); } </script>");
                // Servo4
                valueString = String(servo_pos[3], DEC);

                client.println("<p><h3>Servo 4 Mikrosekunden : <span id=\"textServo4SliderValue\">" + valueString + "</span>");
                client.println("<a href=\"/mitte4/on\"><button class=\"button button1\">Mitte</button></a></p>");

                client.println("<input type=\"range\" min=\"" + String(SERVO_MIN, DEC) + "\" max=\"" + String(SERVO_MAX, DEC) + "\" step=\"10\" class=\"slider\" id=\"Servo4Slider\" onchange=\"Servo4Speed(this.value)\" value=\"" + valueString + "\" /></p>");

                client.println("<script> function Servo4Speed(pos) { ");
                client.println("var sliderValue = document.getElementById(\"Servo4Slider\").value;");
                client.println("document.getElementById(\"textServo4SliderValue\").innerHTML = sliderValue;");
                client.println("var xhr = new XMLHttpRequest();");
                client.println("xhr.open('GET', \"/?Pos3=\" + pos + \"&\", true);");
                client.println("xhr.send(); } </script>");
                // Servo5
                valueString = String(servo_pos[4], DEC);

                client.println("<p><h3>Servo 5 Mikrosekunden : <span id=\"textServo5SliderValue\">" + valueString + "</span>");
                client.println("<a href=\"/mitte5/on\"><button class=\"button button1\">Mitte</button></a></p>");

                client.println("<input type=\"range\" min=\"" + String(SERVO_MIN, DEC) + "\" max=\"" + String(SERVO_MAX, DEC) + "\" step=\"10\" class=\"slider\" id=\"Servo5Slider\" onchange=\"Servo5Speed(this.value)\" value=\"" + valueString + "\" /></p>");

                client.println("<script> function Servo5Speed(pos) { ");
                client.println("var sliderValue = document.getElementById(\"Servo5Slider\").value;");
                client.println("document.getElementById(\"textServo5SliderValue\").innerHTML = sliderValue;");
                client.println("var xhr = new XMLHttpRequest();");
                client.println("xhr.open('GET', \"/?Pos4=\" + pos + \"&\", true);");
                client.println("xhr.send(); } </script>");
                // Button erstellen und link zum aufrufen erstellen

                client.println("<p><a href=\"/back/on\"><button class=\"button button2\">Menu</button></a></p>");

                break;

              case Automatik_Modus_Menu:
                client.println("<h2>Automatik Modus</h2>");

                valueString = String(TimeAuto, DEC);

                client.println("<p><h3>Servo Geschwindigkeit : <span id=\"textServoSpeedValue\">" + valueString + "</span>");

                client.println("<input type=\"range\" min=\"0\" max=\"100\" step=\"5\" class=\"slider\" id=\"ServoSpeedAuto\" onchange=\"ServoSpeed(this.value)\" value=\"" + valueString + "\" /></p>");

                client.println("<script> function ServoSpeed(pos) { ");
                client.println("var sliderValue = document.getElementById(\"ServoSpeedAuto\").value;");
                client.println("document.getElementById(\"textServoSpeedValue\").innerHTML = sliderValue;");
                client.println("var xhr = new XMLHttpRequest();");
                client.println("xhr.open('GET', \"/?Speed=\" + pos + \"&\", true);");
                client.println("xhr.send(); } </script>");

                client.println("<p><a href=\"/pause/on\"><button class=\"button button1\">Pause</button></a></p>");
                client.println("<p><a href=\"/back/on\"><button class=\"button button2\">Menu</button></a></p>");
                break;

              case Einstellung_Menu:
                client.println("<h2>Einstellung</h2>");

                valueString = String(SERVO_STEPS, DEC);

                client.println("<p><h3>Servo Steps : <span id=\"textSetting1Value\">" + valueString + "</span>");
                client.println("<input type=\"text\" id=\"Setting1Input\" class=\"textbox\" oninput=\"Setting1change(this.value)\" value=\"" + valueString + "\" /></p>");

                client.println("<script> function Setting1change(pos) { ");
                client.println("var sliderValue = document.getElementById(\"Setting1Input\").value;");
                client.println("document.getElementById(\"textSetting1Value\").innerHTML = sliderValue;");
                client.println("var xhr = new XMLHttpRequest();");
                client.println("xhr.open('GET', \"/?Set1=\" + pos + \"&\", true);");
                client.println("xhr.send(); } </script>");

                valueString = String(SERVO_MAX, DEC);

                client.println("<p><h3>Servo MAX : <span id=\"textSetting2Value\">" + valueString + "</span>");
                client.println("<input type=\"text\" id=\"Setting2Input\" class=\"textbox\" oninput=\"Setting2change(this.value)\" value=\"" + valueString + "\" /></p>");

                client.println("<script> function Setting2change(pos) { ");
                client.println("var sliderValue = document.getElementById(\"Setting2Input\").value;");
                client.println("document.getElementById(\"textSetting2Value\").innerHTML = sliderValue;");
                client.println("var xhr = new XMLHttpRequest();");
                client.println("xhr.open('GET', \"/?Set2=\" + pos + \"&\", true);");
                client.println("xhr.send(); } </script>");

                valueString = String(SERVO_MIN, DEC);

                client.println("<p><h3>Servo MIN : <span id=\"textSetting3Value\">" + valueString + "</span>");
                client.println("<input type=\"text\" id=\"Setting3Input\" class=\"textbox\" oninput=\"Setting3change(this.value)\" value=\"" + valueString + "\" /></p>");

                client.println("<script> function Setting3change(pos) { ");
                client.println("var sliderValue = document.getElementById(\"Setting3Input\").value;");
                client.println("document.getElementById(\"textSetting3Value\").innerHTML = sliderValue;");
                client.println("var xhr = new XMLHttpRequest();");
                client.println("xhr.open('GET', \"/?Set3=\" + pos + \"&\", true);");
                client.println("xhr.send(); } </script>");

                valueString = String(SERVO_Mitte, DEC);

                client.println("<p><h3>Servo Mitte : <span id=\"textSetting4Value\">" + valueString + "</span>");
                client.println("<input type=\"text\" id=\"Setting4Input\" class=\"textbox\" oninput=\"Setting4change(this.value)\" value=\"" + valueString + "\" /></p>");

                client.println("<script> function Setting4change(pos) { ");
                client.println("var sliderValue = document.getElementById(\"Setting4Input\").value;");
                client.println("document.getElementById(\"textSetting4Value\").innerHTML = sliderValue;");
                client.println("var xhr = new XMLHttpRequest();");
                client.println("xhr.open('GET', \"/?Set4=\" + pos + \"&\", true);");
                client.println("xhr.send(); } </script>");

                valueString = String(SERVO_Hz, DEC);

                client.println("<p><h3>Servo Hz : <span id=\"textSetting5Value\">" + valueString + "</span>");
                client.println("<input type=\"text\" id=\"Setting5Input\" class=\"textbox\" oninput=\"Setting5change(this.value)\" value=\"" + valueString + "\" /></p>");

                client.println("<script> function Setting5change(pos) { ");
                client.println("var sliderValue = document.getElementById(\"Setting5Input\").value;");
                client.println("document.getElementById(\"textSetting4Value\").innerHTML = sliderValue;");
                client.println("var xhr = new XMLHttpRequest();");
                client.println("xhr.open('GET', \"/?Set5=\" + pos + \"&\", true);");
                client.println("xhr.send(); } </script>");

                client.println("<p><a href=\"/save/on\"><button class=\"button button1\">Speichern</button></a></p>");
                client.println("<p><a href=\"/back/on\"><button class=\"button button2\">Menu</button></a></p>");
                break;

              default:
                client.println("<h2>Menu</h2>");
                client.println("<p><a href=\"/10/on\"><button class=\"button button1\">Servotester</button></a></p>");
                client.println("<p><a href=\"/20/on\"><button class=\"button button1\">Automatik Modus</button></a></p>");
                client.println("<p><a href=\"/30/on\"><button class=\"button button1\">Impuls lesen</button></a></p>");
                client.println("<p><a href=\"/40/on\"><button class=\"button button1\">Multiswitch lesen</button></a></p>");
                client.println("<p><a href=\"/50/on\"><button class=\"button button1\">SBUS lesen</button></a></p>");
                client.println("<p><a href=\"/60/on\"><button class=\"button button1\">Einstellung</button></a></p>");
                break; // Wird nicht benötigt, wenn Statement(s) vorhanden sind
              }

              client.println("</body></html>");

              // Die HTTP-Antwort endet mit einer weiteren Leerzeile
              client.println();
              // Break out of the while loop
              break;
            }
            else
            { // if you got a newline, then clear currentLine
              currentLine = "";
            }
          }
          else if (c != '\r')
          {                   // if you got anything else but a carriage return character,
            currentLine += c; // add it to the end of the currentLine
          }
        }
      }
      // Header löschen
      header = "";
      // Schließen Sie die Verbindung
      client.stop();
      Serial.println("Client disconnected.");
      Serial.println("");
    }
  }
}

//
// =======================================================================================================
// LOOP TIME MEASUREMENT
// =======================================================================================================
//

unsigned long loopDuration()
{
  static unsigned long timerOld;
  unsigned long loopTime;
  unsigned long timer = millis();
  loopTime = timer - timerOld;
  timerOld = timer;
  return loopTime;
}

//
// =======================================================================================================
// MAIN LOOP, RUNNING ON CORE 1
// =======================================================================================================
//

void loop()
{

  ButtonRead();
  MenuUpdate();
  batteryVolts();
  webInterface();
  // Serial.print(loopDuration());
}
