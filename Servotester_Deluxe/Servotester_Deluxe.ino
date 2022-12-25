/*
   Servotester Deluxe
   Ziege-One (Der RC-Modelbauer)
 
 ESP32 + Encoder + OLED

 /////Pin Belegung////
 GPIO 13: Servo1
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

// ======== Servotester Deluxe  =======================================

/* Boardversion
ESP32                                         2.0.5
 */

/* Installierte Bibliotheken
ESP32Servo                                    0.12.1
ESP32Encoder                                  0.10.1
Bolder Flight Systems SBUS                    8.1.4
IBusBM                                        1.1.4
ESP8266 and ESP OLED driver SSD1306 displays  4.3.0
 */

#include <WiFi.h>
#include <ESP32Servo.h>
#include <ESP32Encoder.h>
#include <EEPROM.h>
#include "sbus.h"
#include <IBusBM.h>

#include <SH1106Wire.h>  //1.3 Zoll
#include "SSD1306Wire.h"

const float Version = 0.6; // Software Version

//#define OLED1306 //0.96 Zoll

// EEprom
#define EEPROM_SIZE 28

#define adr_eprom_WIFI_ON 0                     // WIFI 1 = Ein 0 = Aus
#define adr_eprom_SERVO_STEPS 4                 // SERVO Steps für Encoder im Servotester Modus
#define adr_eprom_SERVO_MAX 8                   // SERVO µs Max Wert im Servotester Modus
#define adr_eprom_SERVO_MIN 12                  // SERVO µs Min Wert im Servotester Modus
#define adr_eprom_SERVO_Mitte 16                // SERVO µs Mitte Wert im Servotester Modus
#define adr_eprom_SERVO_Hz 20                   // SERVO µs Mitte Wert im Servotester Modus
#define adr_eprom_POWER_SCALE 24                // SERVO µs Mitte Wert im Servotester Modus

//Speicher der Einstellungen
int WIFI_ON;
int SERVO_STEPS;
int SERVO_MAX;
int SERVO_MIN;
int SERVO_Mitte;
int SERVO_Hz;
int POWER_SCALE;

//Encoder + Taster
ESP32Encoder encoder;

int button_PIN = 15;            // Hardware Pin Button
int encoder_Pin1 =16;           // Hardware Pin1 Encoder
int encoder_Pin2 =17;           // Hardware Pin2 Encoder
long prev = 0;                  // Zeitspeicher für Taster 
long prev2 = 0;                 // Zeitspeicher für Taster 
long previousDebouncTime = 0;   // Speicher Entprellzeit für Taster 
int buttonState = 0;            // 0 = Taster nicht betätigt; 1 = Taster langer Druck; 2 = Taster kurzer Druck; 3 = Taster Doppelklick
int encoderState = 0;           // 1 = Drehung nach Links (-); 2 = Drehung nach Rechts (+) 
int Duration_long = 600;        // Zeit für langen Druck
int Duration_double = 200;      // Zeit für Doppelklick
int bouncing = 50;              // Zeit für Taster Entprellung
int encoder_last;               // Speicher letzer Wert Encoder
int encoder_read;               // Speicher aktueller Wert Encoder

// Servo
volatile unsigned char servopin[5]  ={13, 14, 27, 33, 32}; // Pins Servoausgang
Servo servo[5];       // Servo Objekte erzeugen 
int servo_pos[5];     // Speicher für Servowerte
int servocount = 0;   // Zähler für Servowerte

// SBUS
bfs::SbusRx sbus_rx(&Serial2,servopin[0], servopin[0]+1,true); // Sbus auf Serial2

bfs::SbusData sbus_data;

// IBUS
IBusBM IBus;    // IBus object

// Externe Spannungsversorgung
int Power_PIN = 36;   // Hardware Pin Externe Spannung in
bool Power_EN;        // Akku vorhanden 
int  Power_Zel;       // Akkuzellen           
float Power_V;        // Akkuspannung in V
float Power_V_per;    // Akkuspannung in Prozent

//Menüstruktur
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

int Autopos[5];    //Speicher
bool Auto_Pause = 0; //Pause im Auto Modus

//-Menu 40 Multiwitch Futaba lesen

#define kanaele 9       //Anzahl der Kanäle
int value1[kanaele];    //Speicher

//-Menu 30 Impuls lesen
int Impuls_min = 1000;
int Impuls_max = 2000;

//-Menu
int Menu = Servotester_Auswahl; // Aktives Menu
bool SetupMenu = false;         // Zustand Setupmenu
int Einstellung =0 ;            // Aktives Einstellungsmenu
bool Edit = false;              // Einstellungen ausgewählt

//OLED
#ifdef OLED1306
SSD1306Wire display(0x3c, SDA, SCL);   // Oled Hardware an SDA 21 und SCL 22  
#else
SH1106Wire display(0x3c, SDA, SCL);   // Oled Hardware an SDA 21 und SCL 22  
#endif

// Startlogo
#include "images.h"

// Wlan Einstellungen AP Modus
const char* ssid     = "Servotester_Deluxe";
const char* password = "123456789";

// Webserver auf Port 80
WiFiServer server(80);

// Speicher HTTP request
String header;

// Für HTTP GET value
String valueString = String(5);
int pos1 = 0;
int pos2 = 0;

unsigned long currentTime = millis();   // Aktuelle Zeit
unsigned long currentTimeAuto = millis();   // Aktuelle Zeit für Auto Modus
unsigned long currentTimeSpan = millis();   // Aktuelle Zeit für Externe Spannung
unsigned long previousTime = 0;         // Previous time
unsigned long previousTimeAuto = 0;     // Previous time für Auto Modus
unsigned long previousTimeSpan = 0;     // Previous time für Externe Spannung
unsigned long TimeAuto = 50;     // Auto time
const long timeoutTime = 2000;          // Define timeout time in milliseconds (example: 2000ms = 2s)

//MAP als Float
float map_float(float x, float in_min, float in_max, float out_min, float out_max)
{
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

// ======== Setup  =======================================
void setup() {
  // Setup Serial
  Serial.begin(115200);
  
  //EEPROM
  EEPROM.begin(EEPROM_SIZE);
  EEprom_Load(); // Einstellung laden
  Serial.println(WIFI_ON);
  Serial.println(SERVO_STEPS);
  Serial.println(SERVO_MAX);
  Serial.println(SERVO_MIN);
  Serial.println(SERVO_Mitte);
  Serial.println(POWER_SCALE);

  if (SERVO_MIN < 500) {    //EEporm int Werte
      WIFI_ON = 1;  //Wifi ein
      SERVO_STEPS = 10;
      SERVO_MAX = 2000;
      SERVO_MIN = 1000;
      SERVO_Mitte = 1500;
      SERVO_Hz = 50;
      POWER_SCALE = 500;
  }

  // Setup Encoder
  ESP32Encoder::useInternalWeakPullResistors=UP;
  encoder.attachHalfQuad(encoder_Pin1, encoder_Pin2);
  pinMode(button_PIN,INPUT_PULLUP);     // Button_PIN = Eingang

  pinMode(Power_PIN,INPUT);            // 
  

  // Setup OLED
  display.init(); 
  display.flipScreenVertically();
  display.setFont(ArialMT_Plain_10);
  display.drawXbm(0, 0, Logo_width, Logo_height, Logo_bits);
  display.display();

  delay(1000);

  if (WIFI_ON == 1) {     //Wifi Ein
    // Print local IP address and start web server
    Serial.print("AP (Zugangspunkt) einstellen…");
    WiFi.softAP(ssid, password);

    IPAddress IP = WiFi.softAPIP();
    Serial.print("AP-IP-Adresse: ");
    Serial.println(IP);

    display.clear();
    display.setTextAlignment(TEXT_ALIGN_LEFT);
    display.setFont(ArialMT_Plain_16);
    display.drawString(0, 10, "WiFi on.");
    display.drawString(0, 26, "IP address: ");
    display.drawString(0, 42, IP.toString());
    display.display();
    delay(2000);

    display.clear();
    display.setTextAlignment(TEXT_ALIGN_LEFT);
    display.setFont(ArialMT_Plain_16);
    display.drawString(0, 0, "SSID: ");
    display.drawString(0, 16, ssid);
    display.drawString(0, 32, "Password: ");
    display.drawString(0, 48, password);
    display.display();
    delay(3000);
  
    server.begin();  // Start Webserver
    }
    else
    {
      Serial.println("");
      Serial.println("WiFi off.");

      display.clear();
      display.setTextAlignment(TEXT_ALIGN_LEFT);
      display.setFont(ArialMT_Plain_16);
      display.drawString(0, 10, "WiFi");
      display.drawString(0, 26, "off.");
      display.display();
      delay(2000);
    }
  
  display.clear();
  display.setTextAlignment(TEXT_ALIGN_CENTER);
  display.setFont(ArialMT_Plain_16);
  display.drawString(64, 10, "Version:");
  display.drawString(64, 26, String(Version));
  display.display();
  delay(1000);
  

  encoder.setCount(Menu);
  servo_pos[0] = 1500;
  servo_pos[1] = 1500;
  servo_pos[2] = 1500;
  servo_pos[3] = 1500;
  servo_pos[4] = 1500; 
}

// ======== Loop  =======================================
void loop(){

  ButtonRead();

  MenuUpdate();

  Extern_Span();

  if (WIFI_ON == 1) {     //Wifi Ein
  WiFiClient client = server.available();   // Listen for incoming clients

  if (client) {                             // If a new client connects,
    currentTime = millis();
    previousTime = currentTime;
    Serial.println("New Client.");          // print a message out in the serial port
    String currentLine = "";                // make a String to hold incoming data from the client
    while (client.connected() && currentTime - previousTime <= timeoutTime) { // loop while the client's connected
      currentTime = millis();
      if (client.available()) {             // if there's bytes to read from the client,
        char c = client.read();             // read a byte, then
        Serial.write(c);                    // print it out the serial monitor
        header += c;
        if (c == '\n') {                    // if the byte is a newline character
          // if the current line is blank, you got two newline characters in a row.
          // that's the end of the client HTTP request, so send a response:
          if (currentLine.length() == 0) {
            // HTTP-Header fangen immer mit einem Response-Code an (z.B. HTTP/1.1 200 OK)
            // gefolgt vom Content-Type damit der Client weiss was folgt, gefolgt von einer Leerzeile:
            client.println("HTTP/1.1 200 OK");
            client.println("Content-type:text/html");
            client.println("Connection: close");
            client.println();

            // Webseiten Eingaben abfragen

                        //GET /?value=180& HTTP/1.1
            if(header.indexOf("GET /?Pos0=")>=0) {
              pos1 = header.indexOf('=');
              pos2 = header.indexOf('&');
              valueString = header.substring(pos1+1, pos2);
              servo_pos[0] = (valueString.toInt());
            }
            if(header.indexOf("GET /?Pos1=")>=0) {
              pos1 = header.indexOf('=');
              pos2 = header.indexOf('&');
              valueString = header.substring(pos1+1, pos2);
              servo_pos[1] = (valueString.toInt());
            }
            if(header.indexOf("GET /?Pos2=")>=0) {
              pos1 = header.indexOf('=');
              pos2 = header.indexOf('&');
              valueString = header.substring(pos1+1, pos2);
              servo_pos[2] = (valueString.toInt());
            }
            if(header.indexOf("GET /?Pos3=")>=0) {
              pos1 = header.indexOf('=');
              pos2 = header.indexOf('&');
              valueString = header.substring(pos1+1, pos2);
              servo_pos[3] = (valueString.toInt());
            }
            if(header.indexOf("GET /?Pos4=")>=0) {
              pos1 = header.indexOf('=');
              pos2 = header.indexOf('&');
              valueString = header.substring(pos1+1, pos2);
              servo_pos[4] = (valueString.toInt());
            }
            if(header.indexOf("GET /?Set1=")>=0) {
              pos1 = header.indexOf('=');
              pos2 = header.indexOf('&');
              valueString = header.substring(pos1+1, pos2);
              SERVO_STEPS = (valueString.toInt());
            }
            if(header.indexOf("GET /?Set2=")>=0) {
              pos1 = header.indexOf('=');
              pos2 = header.indexOf('&');
              valueString = header.substring(pos1+1, pos2);
              SERVO_MAX = (valueString.toInt());
            }
            if(header.indexOf("GET /?Set3=")>=0) {
              pos1 = header.indexOf('=');
              pos2 = header.indexOf('&');
              valueString = header.substring(pos1+1, pos2);
              SERVO_MIN = (valueString.toInt());
            }
            if(header.indexOf("GET /?Set4=")>=0) {
              pos1 = header.indexOf('=');
              pos2 = header.indexOf('&');
              valueString = header.substring(pos1+1, pos2);
              if (valueString.toInt() > SERVO_MIN){        //Nur übernehmen wenn > MIN
                SERVO_Mitte = (valueString.toInt());
              }
            }
            if(header.indexOf("GET /?Set5=")>=0) {
              pos1 = header.indexOf('=');
              pos2 = header.indexOf('&');
              valueString = header.substring(pos1+1, pos2);
              SERVO_Hz = (valueString.toInt());
            }            
            if(header.indexOf("GET /?Speed=")>=0) {
              pos1 = header.indexOf('=');
              pos2 = header.indexOf('&');
              valueString = header.substring(pos1+1, pos2);
              TimeAuto = (valueString.toInt());
            }
            if (header.indexOf("GET /mitte1/on") >= 0) 
            {
              servo_pos[0] = SERVO_Mitte; //Mitte
            }  
            if (header.indexOf("GET /mitte2/on") >= 0) 
            {
              servo_pos[1] = SERVO_Mitte; //Mitte
            }
            if (header.indexOf("GET /mitte3/on") >= 0) 
            {
              servo_pos[2] = SERVO_Mitte; //Mitte
            }
            if (header.indexOf("GET /mitte4/on") >= 0) 
            {
              servo_pos[3] = SERVO_Mitte; //Mitte
            }
            if (header.indexOf("GET /mitte5/on") >= 0) 
            {
              servo_pos[4] = SERVO_Mitte; //Mitte
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
              EEprom_Save();
            }
            if (header.indexOf("GET /pause/on") >= 0) 
            {
              if(Auto_Pause){
                Auto_Pause=false;
              }
              else
              {
                Auto_Pause=true;
              }
            }
            
            //HTML Seite angezeigen:
            client.println("<!DOCTYPE html><html>");
            //client.println("<meta http-equiv='refresh' content='5'>");
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

            switch (Menu) {
              case Servotester_Menu:
                  client.println("<h2>Servotester</h2>");
                  //Servo1
                  valueString = String(servo_pos[0], DEC);

                  client.println("<p><h3>Servo 1 Mikrosekunden : <span id=\"textServo1SliderValue\">" + valueString + "</span>");
                  client.println("<a href=\"/mitte1/on\"><button class=\"button button1\">Mitte</button></a></p>");

                  client.println("<input type=\"range\" min=\""+String(SERVO_MIN, DEC)+"\" max=\""+String(SERVO_MAX, DEC)+"\" step=\"10\" class=\"slider\" id=\"Servo1Slider\" onchange=\"Servo1Speed(this.value)\" value=\"" + valueString + "\" /></p>");

                  client.println("<script> function Servo1Speed(pos) { ");
                  client.println("var sliderValue = document.getElementById(\"Servo1Slider\").value;");
                  client.println("document.getElementById(\"textServo1SliderValue\").innerHTML = sliderValue;");
                  client.println("var xhr = new XMLHttpRequest();");
                  client.println("xhr.open('GET', \"/?Pos0=\" + pos + \"&\", true);");
                  client.println("xhr.send(); } </script>");
                  //Servo2
                  valueString = String(servo_pos[1], DEC);

                  client.println("<p><h3>Servo 2 Mikrosekunden : <span id=\"textServo2SliderValue\">" + valueString + "</span>");
                  client.println("<a href=\"/mitte2/on\"><button class=\"button button1\">Mitte</button></a></p>");

                  client.println("<input type=\"range\" min=\""+String(SERVO_MIN, DEC)+"\" max=\""+String(SERVO_MAX, DEC)+"\" step=\"10\" class=\"slider\" id=\"Servo2Slider\" onchange=\"Servo2Speed(this.value)\" value=\"" + valueString + "\" /></p>");

                  client.println("<script> function Servo2Speed(pos) { ");
                  client.println("var sliderValue = document.getElementById(\"Servo2Slider\").value;");
                  client.println("document.getElementById(\"textServo2SliderValue\").innerHTML = sliderValue;");
                  client.println("var xhr = new XMLHttpRequest();");
                  client.println("xhr.open('GET', \"/?Pos1=\" + pos + \"&\", true);");
                  client.println("xhr.send(); } </script>");
                  //Servo3
                  valueString = String(servo_pos[2], DEC);

                  client.println("<p><h3>Servo 3 Mikrosekunden : <span id=\"textServo3SliderValue\">" + valueString + "</span>");
                  client.println("<a href=\"/mitte3/on\"><button class=\"button button1\">Mitte</button></a></p>");

                  client.println("<input type=\"range\" min=\""+String(SERVO_MIN, DEC)+"\" max=\""+String(SERVO_MAX, DEC)+"\" step=\"10\" class=\"slider\" id=\"Servo3Slider\" onchange=\"Servo3Speed(this.value)\" value=\"" + valueString + "\" /></p>");

                  client.println("<script> function Servo3Speed(pos) { ");
                  client.println("var sliderValue = document.getElementById(\"Servo3Slider\").value;");
                  client.println("document.getElementById(\"textServo3SliderValue\").innerHTML = sliderValue;");
                  client.println("var xhr = new XMLHttpRequest();");
                  client.println("xhr.open('GET', \"/?Pos2=\" + pos + \"&\", true);");
                  client.println("xhr.send(); } </script>");
                  //Servo4
                  valueString = String(servo_pos[3], DEC);

                  client.println("<p><h3>Servo 4 Mikrosekunden : <span id=\"textServo4SliderValue\">" + valueString + "</span>");
                  client.println("<a href=\"/mitte4/on\"><button class=\"button button1\">Mitte</button></a></p>");

                  client.println("<input type=\"range\" min=\""+String(SERVO_MIN, DEC)+"\" max=\""+String(SERVO_MAX, DEC)+"\" step=\"10\" class=\"slider\" id=\"Servo4Slider\" onchange=\"Servo4Speed(this.value)\" value=\"" + valueString + "\" /></p>");

                  client.println("<script> function Servo4Speed(pos) { ");
                  client.println("var sliderValue = document.getElementById(\"Servo4Slider\").value;");
                  client.println("document.getElementById(\"textServo4SliderValue\").innerHTML = sliderValue;");
                  client.println("var xhr = new XMLHttpRequest();");
                  client.println("xhr.open('GET', \"/?Pos3=\" + pos + \"&\", true);");
                  client.println("xhr.send(); } </script>");
                  //Servo5
                  valueString = String(servo_pos[4], DEC);

                  client.println("<p><h3>Servo 5 Mikrosekunden : <span id=\"textServo5SliderValue\">" + valueString + "</span>");
                  client.println("<a href=\"/mitte5/on\"><button class=\"button button1\">Mitte</button></a></p>");

                  client.println("<input type=\"range\" min=\""+String(SERVO_MIN, DEC)+"\" max=\""+String(SERVO_MAX, DEC)+"\" step=\"10\" class=\"slider\" id=\"Servo5Slider\" onchange=\"Servo5Speed(this.value)\" value=\"" + valueString + "\" /></p>");

                  client.println("<script> function Servo5Speed(pos) { ");
                  client.println("var sliderValue = document.getElementById(\"Servo5Slider\").value;");
                  client.println("document.getElementById(\"textServo5SliderValue\").innerHTML = sliderValue;");
                  client.println("var xhr = new XMLHttpRequest();");
                  client.println("xhr.open('GET', \"/?Pos4=\" + pos + \"&\", true);");
                  client.println("xhr.send(); } </script>");                                    
                  //Button erstellen und link zum aufrufen erstellen 
                                 
                  client.println("<p><a href=\"/back/on\"><button class=\"button button2\">Menu</button></a></p>");
                  
                  break;

              case Automatik_Modus_Menu:
                  client.println("<h2>Automatik Modus</h2>");

                  valueString = String(TimeAuto, DEC);

                  client.println("<p><h3>Servo Geschwindigkeit : <span id=\"textServoSpeedValue\">" + valueString + "</span>");

                  client.println("<input type=\"range\" min=\"5\" max=\"100\" step=\"5\" class=\"slider\" id=\"ServoSpeedAuto\" onchange=\"ServoSpeed(this.value)\" value=\"" + valueString + "\" /></p>");

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
          } else { // if you got a newline, then clear currentLine
            currentLine = "";
          }
        } else if (c != '\r') {  // if you got anything else but a carriage return character,
          currentLine += c;      // add it to the end of the currentLine
        }
      }
    }
    // header löschen
    header = "";
    // Schließen Sie die Verbindung
    client.stop();
    Serial.println("Client disconnected.");
    Serial.println("");
    }
}
}

// ======== Button Abfrage  =======================================
void ButtonRead(){  
  
  buttonState = 0;
  if (!(digitalRead(button_PIN))){  // Button gedrückt 0
    delay(bouncing);                // Taster entprellen
    prev = millis();
    buttonState = 1;
    while((millis()-prev)<=Duration_long){
      if(digitalRead(button_PIN)){  // Button losgelassen 1 innerhalb Zeit
        delay(bouncing);            // Taster entprellen
        buttonState = 2;
        prev2 = millis();
        while((millis()-prev2)<=Duration_double){     //Doppelkick abwarten
          if (!(digitalRead(button_PIN))){ // Button gedrückt 0 innerhalb Zeit Doppelklick
            delay(bouncing);               // Taster entprellen
            buttonState = 3;
            if(digitalRead(button_PIN)){   // Button losgelassen 1
              break;
            }
          }
        }    
      break;
      }
    }
    while(!(digitalRead(button_PIN))){ // Warten bis Button nicht gedückt ist = 1
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
    encoderState = 1;
    encoder_last = encoder_read;
    previousDebouncTime = millis();
    }
  }
  else if (encoder_last < encoder_read)
  {
    if (encoder_last < encoder_read - 1)
    {
    encoderState = 2;
    encoder_last = encoder_read;
    previousDebouncTime = millis();
    }
  }
  else
  {
    encoderState = 0;
    encoder.setCount(1500); // set starting count value after attaching
    encoder_last = 1500;
    //delay(100);
  }
}

// ======== MenuUpdate  =======================================
/* 
 * 1 = Servotester       Auswahl -> 10 Servotester
 * 2 = Automatik Modus   Auswahl -> 20 Automatik Modus
 * 3 = Impuls lesen      Auswahl -> 30 Impuls lesen
 * 4 = Multiswitch lesen Auswahl -> 40 Multiswitch lesen
 * 5 = SBUS lesen        Auswahl -> 50 SBUS lesen
 * 6 = Einstellung       Auswahl -> 60 Einstellung
 */
void MenuUpdate(){

switch (Menu) {
  case Servotester_Auswahl: 
    display.clear();
    display.setTextAlignment(TEXT_ALIGN_CENTER);
    display.setFont(ArialMT_Plain_24);
    display.drawString(64, 0, "  Menu >" );
    display.setFont(ArialMT_Plain_16);
    display.drawString(64, 25,"Servotester" );
    if (Power_EN)
    {
      display.setFont(ArialMT_Plain_10);  
      display.setTextAlignment(TEXT_ALIGN_RIGHT);
      display.drawString(30, 50, String(Power_V));
      display.setTextAlignment(TEXT_ALIGN_LEFT);
      display.drawString(30, 50, "V");
      display.setTextAlignment(TEXT_ALIGN_RIGHT);
      display.drawString(98, 50, String(Power_V_per));
      display.setTextAlignment(TEXT_ALIGN_LEFT);
      display.drawString(98, 50, "%");
    }
    
    display.display();
          
    if (encoderState == 1){
      Menu=1;
    }
    if (encoderState == 2){
      Menu++;
    }
    
    if(buttonState == 2){
      Menu = Servotester_Menu;
    }
    break;
  case Automatik_Modus_Auswahl: 
    display.clear();
    display.setTextAlignment(TEXT_ALIGN_CENTER);
    display.setFont(ArialMT_Plain_24);
    display.drawString(64, 0, "< Menu >" );
    display.setFont(ArialMT_Plain_16);
    display.drawString(64, 25,"Automatik Modus" );
    display.display();
          
    if (encoderState == 1){
      Menu--;
    }
    if (encoderState == 2){
      Menu++;
    }
    
    if(buttonState == 2){
      Menu = Automatik_Modus_Menu;
    }
    break;  
  case Impuls_lesen_Auswahl: 
    display.clear();
    display.setTextAlignment(TEXT_ALIGN_CENTER);
    display.setFont(ArialMT_Plain_24);
    display.drawString(64, 0, "< Menu >" );
    display.setFont(ArialMT_Plain_16);
    display.drawString(64, 25,"Impuls lesen" );
    display.display();
          
    if (encoderState == 1){
      Menu--;
    }
    if (encoderState == 2){
      Menu++;
    }
       
    if(buttonState == 2){
      Menu = Impuls_lesen_Menu;
    }
    break;  
    
  case Multiswitch_lesen_Auswahl: 
    display.clear();
    display.setTextAlignment(TEXT_ALIGN_CENTER);
    display.setFont(ArialMT_Plain_24);
    display.drawString(64, 0, "< Menu >" );
    display.setFont(ArialMT_Plain_16);
    display.drawString(64, 25,"Multiswitch" );
    display.drawString(64, 45,"lesen" );
    display.display();
          
    if (encoderState == 1){
      Menu--;
    }
    if (encoderState == 2){
      Menu++;
    }
       
    if(buttonState == 2){
      Menu = Multiswitch_lesen_Menu;
    }
    break;
    
  case SBUS_lesen_Auswahl: 
    display.clear();
    display.setTextAlignment(TEXT_ALIGN_CENTER);
    display.setFont(ArialMT_Plain_24);
    display.drawString(64, 0, "< Menu >" );
    display.setFont(ArialMT_Plain_16);
    display.drawString(64, 25,"SBUS lesen" );
    display.display();
          
    if (encoderState == 1){
      Menu--;
    }
    if (encoderState == 2){
      Menu++;
    }
       
    if(buttonState == 2){
      Menu = SBUS_lesen_Menu;
    }
    break;

  case IBUS_lesen_Auswahl: 
    display.clear();
    display.setTextAlignment(TEXT_ALIGN_CENTER);
    display.setFont(ArialMT_Plain_24);
    display.drawString(64, 0, "< Menu >" );
    display.setFont(ArialMT_Plain_16);
    display.drawString(64, 25,"IBUS lesen" );
    display.display();
          
    if (encoderState == 1){
      Menu--;
    }
    if (encoderState == 2){
      Menu++;
    }
       
    if(buttonState == 2){
      Menu = IBUS_lesen_Menu;
    }
    break;    
    
  case Einstellung_Auswahl: 
    display.clear();
    display.setTextAlignment(TEXT_ALIGN_CENTER);
    display.setFont(ArialMT_Plain_24);
    display.drawString(64, 0, "< Menu  " );
    display.setFont(ArialMT_Plain_16);
    display.drawString(64, 25,"Einstellung" );
    display.display();
          
    if (encoderState == 1){
      Menu--;
    }
    if (encoderState == 2){
      Menu=Einstellung_Auswahl;
    }
       
    if(buttonState == 2){
      Menu = Einstellung_Menu;
    }
    break;
 
  case Servotester_Menu:
    display.clear();
    display.setTextAlignment(TEXT_ALIGN_LEFT);
    display.setFont(ArialMT_Plain_10);
    display.drawString(0, 0, "HZ");
    display.drawString(0, 10, String(SERVO_Hz));
    display.setTextAlignment(TEXT_ALIGN_CENTER);
    display.setFont(ArialMT_Plain_24);
    display.drawString(64, 0, "Servo" + String(servocount+1));
    //display.drawString(64, 25, String(servo_pos[servocount]) + "°");
    //display.drawProgressBar(8, 50, 112, 10, ((servo_pos[servocount]*100)/180));
    display.drawString(64, 25, String(servo_pos[servocount]) + "µs");
    display.drawProgressBar(8, 50, 112, 10, (((servo_pos[servocount]-SERVO_MIN)*100)/(SERVO_MAX-SERVO_MIN)));
    display.display();
    if (!SetupMenu)
    {
      servo[0].attach(servopin[0],SERVO_MIN,SERVO_MAX);  // ServoPIN, MIN, MAX
      servo[1].attach(servopin[1],SERVO_MIN,SERVO_MAX);  // ServoPIN, MIN, MAX
      servo[2].attach(servopin[2],SERVO_MIN,SERVO_MAX);  // ServoPIN, MIN, MAX
      servo[3].attach(servopin[3],SERVO_MIN,SERVO_MAX);  // ServoPIN, MIN, MAX
      servo[4].attach(servopin[4],SERVO_MIN,SERVO_MAX);  // ServoPIN, MIN, MAX
      servo[0].setPeriodHertz(SERVO_Hz);
      SetupMenu = true;
    }
    
    servo[0].writeMicroseconds(servo_pos[0]);
    servo[1].writeMicroseconds(servo_pos[1]);
    servo[2].writeMicroseconds(servo_pos[2]);
    servo[3].writeMicroseconds(servo_pos[3]);
    servo[4].writeMicroseconds(servo_pos[4]);
     
    if (encoderState == 1){
      servo_pos[servocount] = servo_pos[servocount] - SERVO_STEPS ;
    }
    if (encoderState == 2){
      servo_pos[servocount] = servo_pos[servocount] + SERVO_STEPS ;
    }

    if (servo_pos[servocount] > SERVO_MAX)   //Servo MAX
    {
       servo_pos[servocount] = SERVO_MAX;
    }
    else if (servo_pos[servocount] < SERVO_MIN)  //Servo MIN
    {
       servo_pos[servocount] = SERVO_MIN;
    } 
    
    if(buttonState == 1){
      Menu = Servotester_Auswahl; 
      SetupMenu = false;
      servo[0].detach();
      servo[1].detach();
      servo[2].detach();
      servo[3].detach();
      servo[4].detach();
      servocount = 0;
    }

     if(buttonState == 2){
      servo_pos[servocount] = SERVO_Mitte;    //Servo Mitte 
    }

     if(buttonState == 3){
      servocount++;    //Servo +
    }    

    if (servocount > 4) 
    {
       servocount = 0;
    }
    break;
    
  case Automatik_Modus_Menu:
    display.clear();
    display.setTextAlignment(TEXT_ALIGN_CENTER);
    display.setFont(ArialMT_Plain_24);
    display.drawString(64, 0, "Servo" + String(servocount+1));
    display.drawString(64, 25, String(servo_pos[servocount]) + "µs");
    if (Auto_Pause){
      display.setFont(ArialMT_Plain_16);
      display.drawString(64, 48, "Pause");
    }
    else
    {
      display.drawProgressBar(8, 50, 112, 10, (((servo_pos[servocount]-SERVO_MIN)*100)/(SERVO_MAX-SERVO_MIN)));
    }
    display.display();
    if (!SetupMenu)
    {
      servo[0].attach(servopin[0],SERVO_MIN,SERVO_MAX);  // ServoPIN, MIN, MAX
      servo[1].attach(servopin[1],SERVO_MIN,SERVO_MAX);  // ServoPIN, MIN, MAX
      servo[2].attach(servopin[2],SERVO_MIN,SERVO_MAX);  // ServoPIN, MIN, MAX
      servo[3].attach(servopin[3],SERVO_MIN,SERVO_MAX);  // ServoPIN, MIN, MAX
      servo[4].attach(servopin[4],SERVO_MIN,SERVO_MAX);  // ServoPIN, MIN, MAX
      TimeAuto = 50;    //Zeit für SERVO Steps +-
      Auto_Pause = false; //Pause aus
      SetupMenu = true;
    }
    
    currentTimeAuto = millis();
    if (!Auto_Pause){
      if ((currentTimeAuto - previousTimeAuto) > TimeAuto){
        previousTimeAuto = currentTimeAuto;
        if (Autopos[servocount] > 1500){
          servo_pos[servocount] = servo_pos[servocount] + SERVO_STEPS;
        }
        else
        {
          servo_pos[servocount] = servo_pos[servocount] - SERVO_STEPS;
        }
      }
    }

    if (servo_pos[servocount] < SERVO_MIN){
      Autopos[servocount] = SERVO_MAX;
      servo_pos[servocount] = SERVO_MIN;
    }
    if (servo_pos[servocount] > SERVO_MAX){
      Autopos[servocount] = SERVO_MIN;
      servo_pos[servocount] = SERVO_MAX;
    }

    servo[servocount].writeMicroseconds(servo_pos[servocount]);
    
    if (encoderState == 1){
      if (Auto_Pause){
        servo_pos[servocount] = servo_pos[servocount] - SERVO_STEPS ;
      }
      else
      {
        TimeAuto--;
      }
    }
    if (encoderState == 2){
      if (Auto_Pause){
        servo_pos[servocount] = servo_pos[servocount] + SERVO_STEPS ;
      }
      else
      {
        TimeAuto++;
      }
    }

    if (TimeAuto > 100)   //TimeAuto MAX
    {
       TimeAuto = 100;
    }
    else if (TimeAuto < 5)  //TimeAuto MIN
    {
       TimeAuto = 5;
    } 

    if (servocount > 4) 
    {
       servocount = 4;
    }
    else if (servocount < 0) 
    {
       servocount = 0;
    } 
    
    if(buttonState == 1){
      Menu = Automatik_Modus_Auswahl; 
      SetupMenu = false;
      servo[0].detach();
      servo[1].detach();
      servo[2].detach();
      servo[3].detach();
      servo[4].detach();
      servocount = 0;
    }

     if(buttonState == 2){
      Auto_Pause = !Auto_Pause;
    }

     if(buttonState == 3){
      servocount++;    //Servo +
    }    

    if (servocount > 4) 
    {
       servocount = 0;
    }
    break;    
  
  case Impuls_lesen_Menu:   
    display.clear();
    display.setTextAlignment(TEXT_ALIGN_CENTER);
    display.setFont(ArialMT_Plain_24);
    display.drawString(64, 0, "Impuls" + String(servocount+1));
    display.drawString(64, 25, String(servo_pos[servocount]) + "µs");
    display.drawProgressBar(8, 50, 112, 10, (((servo_pos[servocount]-Impuls_min)*100)/(Impuls_max - Impuls_min)));
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
    servo_pos[servocount] = pulseIn(servopin[servocount], HIGH);

    if (servo_pos[servocount] < Impuls_min) {
      Impuls_min = servo_pos[servocount];
    }
    if (servo_pos[servocount] > Impuls_max) {
      Impuls_max = servo_pos[servocount];
    }
    
    if (encoderState == 1){
      servocount--;
      Impuls_min = 1000;
      Impuls_max = 2000;
    }
    if (encoderState == 2){
      servocount++;
      Impuls_min = 1000;
      Impuls_max = 2000;      
    }

    if (servocount > 4) 
    {
       servocount = 4;
    }
    else if (servocount < 0) 
    {
       servocount = 0;
    } 
    
    if(buttonState == 1){
      Menu = Impuls_lesen_Auswahl; 
      SetupMenu = false;
      servocount = 0;
    }

    if(buttonState == 2){
      Impuls_min = 1000;
      Impuls_max = 2000;
    }
    break;    

  case Multiswitch_lesen_Menu:   
    display.clear();
    display.setTextAlignment(TEXT_ALIGN_LEFT);
    display.setFont(ArialMT_Plain_10);
    display.drawString(20, 0, String(value1[8]));
    display.drawString(64, 0, String(value1[0]) + "  " + String(value1[1]));
    display.drawString(64, 15, String(value1[2]) + "  " + String(value1[3]));
    display.drawString(64, 30, String(value1[4]) + "  " + String(value1[5]));
    display.drawString(64, 45, String(value1[6]) + "  " + String(value1[7]));
    display.display();
    if (!SetupMenu)
    {
      pinMode(servopin[0], INPUT);
      SetupMenu = true;
    }
    
    value1[8] = 1500;
    
    while(value1[8] > 1000)  //Wait for the beginning of the frame
    {
         value1[8]=pulseIn(servopin[0], HIGH);
    }

    for(int x=0; x<=kanaele-1; x++)//Loop to store all the channel position
    {
      value1[x]=pulseIn(servopin[0], HIGH);
    }
   

    if (encoderState == 1){
      
    }
    if (encoderState == 2){
      
    }
    
    if(buttonState == 1){
      Menu = Multiswitch_lesen_Auswahl; 
      SetupMenu = false;
    }

     if(buttonState == 2){
      
    }
    break;

  case SBUS_lesen_Menu:   
    display.clear();
    display.setTextAlignment(TEXT_ALIGN_RIGHT);
    display.setFont(ArialMT_Plain_10);
    display.drawString( 32, 0,  String(sbus_data.ch[0]));
    display.drawString( 64, 0,  String(sbus_data.ch[1]));
    display.drawString( 96, 0,  String(sbus_data.ch[2]));
    display.drawString(128, 0,  String(sbus_data.ch[3]));

    display.drawString( 32, 15,  String(sbus_data.ch[4]));
    display.drawString( 64, 15,  String(sbus_data.ch[5]));
    display.drawString( 96, 15,  String(sbus_data.ch[6]));
    display.drawString(128, 15,  String(sbus_data.ch[7]));

    display.drawString( 32, 30,  String(sbus_data.ch[8]));
    display.drawString( 64, 30,  String(sbus_data.ch[9]));
    display.drawString( 96, 30,  String(sbus_data.ch[10]));
    display.drawString(128, 30,  String(sbus_data.ch[11]));

    display.drawString( 32, 45,  String(sbus_data.ch[12]));
    display.drawString( 64, 45,  String(sbus_data.ch[13]));
    display.drawString( 96, 45,  String(sbus_data.ch[14]));
    display.drawString(128, 45,  String(sbus_data.ch[15]));
    display.display();
    
    if (!SetupMenu)
    {
      sbus_rx.Begin();
      SetupMenu = true;
    }
    
  if (sbus_rx.Read()) {
      sbus_data = sbus_rx.data();
  }
   

    if (encoderState == 1){
      
    }
    if (encoderState == 2){
      
    }
    
    if(buttonState == 1){
      Menu = SBUS_lesen_Auswahl; 
      SetupMenu = false;
    }

     if(buttonState == 2){
      
    }
    break;

  case IBUS_lesen_Menu:   
    display.clear();
    display.setTextAlignment(TEXT_ALIGN_RIGHT);
    display.setFont(ArialMT_Plain_10);
    display.drawString( 32, 0,  String(IBus.readChannel(0)));
    display.drawString( 64, 0,  String(IBus.readChannel(1)));
    display.drawString( 96, 0,  String(IBus.readChannel(2)));
    display.drawString(128, 0,  String(IBus.readChannel(3)));

    display.drawString( 32, 15,  String(IBus.readChannel(4)));
    display.drawString( 64, 15,  String(IBus.readChannel(5)));
    display.drawString( 96, 15,  String(IBus.readChannel(6)));
    display.drawString(128, 15,  String(IBus.readChannel(7)));

    display.drawString( 32, 30,  String(IBus.readChannel(8)));
    display.drawString( 64, 30,  String(IBus.readChannel(9)));
    display.drawString( 96, 30,  String(IBus.readChannel(10)));
    display.drawString(128, 30,  String(IBus.readChannel(11)));

    display.drawString( 32, 45,  String(IBus.readChannel(12)));
    display.drawString( 64, 45,  String(IBus.readChannel(13)));
    //display.drawString( 96, 45,  String(IBus.readChannel(14)));
    //display.drawString(128, 45,  String(IBus.readChannel(15)));
    display.display();
    
    if (!SetupMenu)
    {
      IBus.begin(Serial2,IBUSBM_NOTIMER,servopin[0], servopin[1]);    // iBUS object connected to serial2 RX2 pin and use timer 1
      SetupMenu = true;
    }

    IBus.loop(); // call internal loop function to update the communication to the receiver 
  //if (sbus_rx.Read()) {
  //    sbus_data = sbus_rx.ch();
  //}
   

    if (encoderState == 1){
      
    }
    if (encoderState == 2){
      
    }
    
    if(buttonState == 1){
      Menu = IBUS_lesen_Auswahl; 
      SetupMenu = false;
    }

     if(buttonState == 2){
      
    }
    break;    

  case Einstellung_Menu:
    display.clear();
    display.setTextAlignment(TEXT_ALIGN_CENTER);
    display.setFont(ArialMT_Plain_24);
    display.drawString(64, 0, "< Einstellung  >" );
    display.setFont(ArialMT_Plain_16);
    switch (Einstellung) {
      case 0:
        display.drawString(64, 25,"Wifi" );
        if (WIFI_ON == 1) {
          display.drawString(64, 45,"Ein" );
        }
        else {
          display.drawString(64, 45,"Aus" );
        }
        break;
      case 1:
        display.drawString(64, 25,"Servo Steps µs" );
        display.drawString(64, 45,String(SERVO_STEPS));
        break;
      case 2:
        display.drawString(64, 25,"Servo MAX µs" );
        display.drawString(64, 45,String(SERVO_MAX));
        break;
      case 3:
        display.drawString(64, 25,"Servo MIN µs" );
        display.drawString(64, 45,String(SERVO_MIN));
        break;
      case 4:
        display.drawString(64, 25,"Servo Mitte µs" );
        display.drawString(64, 45,String(SERVO_Mitte));
        break;   
      case 5:
        display.drawString(64, 25,"Servo Hz" );
        display.drawString(64, 45,String(SERVO_Hz));
        break;  
      case 6:
        display.drawString(64, 25,"POWER Scale" );
        display.drawString(64, 45,String(POWER_SCALE));
        display.setFont(ArialMT_Plain_10);  
        display.setTextAlignment(TEXT_ALIGN_RIGHT);
        display.drawString(110, 50, String(Power_V));
        display.setTextAlignment(TEXT_ALIGN_LEFT);
        display.drawString(110, 50, "V");
        break;     
    }
    if (Edit) { 
      display.drawString(10, 45,"->" );
    }
    display.display();                       
    if (!SetupMenu)
    {
      
      SetupMenu = true;
    }
    
    if (encoderState == 1){
      if (!Edit) {    
      Einstellung--;
      }
      else
      {
      switch (Einstellung) {
      case 0:  
        WIFI_ON --;
        break;
      case 1:
        SERVO_STEPS --;
        break;
      case 2:
        SERVO_MAX --;
        break;
      case 3:
        SERVO_MIN --;
        break;
      case 4:
        SERVO_Mitte --;
        break;
      case 5:
        SERVO_Hz --;
        break;
      case 6:
        POWER_SCALE --;
        break;        
      }  
      } 
    }
    if (encoderState == 2){
      if (!Edit) {    
      Einstellung++;
      }
      else
      {
      switch (Einstellung) {
      case 0:  
        WIFI_ON ++;
        break;
      case 1:
        SERVO_STEPS ++;
        break;
      case 2:
        SERVO_MAX ++;
        break;
      case 3:
        SERVO_MIN ++;
        break;
      case 4:
        SERVO_Mitte ++;
        break;
      case 5:
        SERVO_Hz ++;
        break;
      case 6:
        POWER_SCALE ++;
        break;         
      }              
      } 
    }

    if (Einstellung > 6) 
    {
       Einstellung = 0;
    }
    else if (Einstellung < 0) 
    {
       Einstellung = 6;
    } 
    
    if (WIFI_ON < 0) {   //Wifi off nicht unter 0
      WIFI_ON = 0;
    }

   if (WIFI_ON > 1) {   //Wifi on nicht über 1
      WIFI_ON = 1;
    }

    if (SERVO_STEPS < 0) {   //Steps nicht unter 0
      SERVO_STEPS = 0;
    }

    if (SERVO_MAX < SERVO_Mitte) {   //Max nicht unter Mitte
      SERVO_MAX = SERVO_Mitte + 1;
    }

    if (SERVO_MAX > 2500) {   //Max nicht über 2500
      SERVO_MAX = 2500;
    }

    if (SERVO_MIN > SERVO_Mitte) {   //Min nicht über Mitte
      SERVO_MIN = SERVO_Mitte - 1;
    }

    if (SERVO_MIN < 500) {   //Min nicht unter 500
      SERVO_MIN = 500;
    }

    if (SERVO_Mitte > SERVO_MAX) {   //Mitte nicht über Max
      SERVO_Mitte = SERVO_MAX - 1;
    }

    if (SERVO_Mitte < SERVO_MIN) {   //Mitte nicht unter Min
      SERVO_Mitte = SERVO_MIN + 1;
    }

    if (SERVO_Hz == 51) {
      SERVO_Hz = 200;
    }
    
    if (SERVO_Hz == 201) {
      SERVO_Hz = 333;
    }
    
    if (SERVO_Hz == 334) {
      SERVO_Hz = 560;
      SERVO_MAX = 1000;
      SERVO_MIN = 500;
      SERVO_Mitte = 760;
    }
      
    if (SERVO_Hz == 561) {
      SERVO_Hz = 50;
      SERVO_MAX = 2000;
      SERVO_MIN = 1000;
      SERVO_Mitte = 1500;
    }
    
    if (SERVO_Hz == 49) {
      SERVO_Hz = 560;
      SERVO_MAX = 1000;
      SERVO_MIN = 500;
      SERVO_Mitte = 760;
    }
    
    if (SERVO_Hz == 559) {
      SERVO_Hz = 333;
      SERVO_MAX = 2000;
      SERVO_MIN = 1000;
      SERVO_Mitte = 1500;
    }      
    
    if (SERVO_Hz == 332) {
      SERVO_Hz = 200;
    }      
    
    if (SERVO_Hz == 199) {
      SERVO_Hz = 50;
    }      

    if (SERVO_Hz == 560) {
      SERVO_MAX = 1000;
      SERVO_MIN = 500;
      SERVO_Mitte = 760;
    }  
    
        
    if(buttonState == 1){
      Menu = Einstellung_Auswahl; 
      SetupMenu = false;
    }

    if(buttonState == 2){
      if (Edit) {
        Edit = false;
        //Speichern
        EEprom_Save();
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

// ======== Externe Spanunngsversorgung  ======================================= 
void Extern_Span() {

  currentTimeSpan = millis();
  if ((currentTimeSpan - previousTimeSpan) > 2000)
  {
    previousTimeSpan = currentTimeSpan;


    // ADC & DAC calibration https://hackaday.io/project/27511-microfluidics-control-system/log/69406-adc-dac-calibration  
    int val = analogRead(Power_PIN);
    double x = val;
    double y = -3e-12*pow(x, 3) - 7e-10*pow(x, 2) + 0.0003*x + 0.0169;
    int MeasuredValue = std::min(3300, std::max(0, int(round(y*UINT8_MAX))));

    Serial.print("MeasuredValue: ");
    Serial.println(MeasuredValue);
  
    // Wert in Volt umrechnen
    Power_V = MeasuredValue;
    Power_V = Power_V/100;

    float scale_value = POWER_SCALE;
    scale_value = scale_value/100;
  
    Power_V = Power_V  * scale_value;

    Serial.print("Spannung: ");
    Serial.println(Power_V,8);

    Power_EN = 1;
  
    if (Power_V > 21) //6s Lipo
    {
      Power_Zel = 6;
    }
    else if (Power_V > 16.8) //5s Lipo
    {
      Power_Zel = 5;
    }
    else if (Power_V > 12.6) //4s Lipo
    {
      Power_Zel = 4;
    }
    else if (Power_V > 8.4) //3s Lipo
    {
      Power_Zel = 3;
    }
    else if (Power_V > 4.2) //2s Lipo
    {
      Power_Zel = 2;
    }   
    else  //1s Lipo kein Akku angeschlossen
    {
      Power_Zel = 1;
      Power_EN = 0;
    }     
  
  Power_V_per = (Power_V/Power_Zel);  // Prozentanzeige
  Power_V_per = map_float(Power_V_per, 3.5, 4.2, 0, 100);
}
}  


// ======== EEprom  ======================================= 
void EEprom_Load() {
  WIFI_ON = EEPROM.readInt(adr_eprom_WIFI_ON);
  SERVO_STEPS = EEPROM.readInt(adr_eprom_SERVO_STEPS);
  SERVO_MAX = EEPROM.readInt(adr_eprom_SERVO_MAX);
  SERVO_MIN = EEPROM.readInt(adr_eprom_SERVO_MIN);
  SERVO_Mitte = EEPROM.readInt(adr_eprom_SERVO_Mitte);
  SERVO_Hz = EEPROM.readInt(adr_eprom_SERVO_Hz);
  POWER_SCALE = EEPROM.readInt(adr_eprom_POWER_SCALE);
  Serial.println("EEPROM gelesen.");
}

void EEprom_Save() {
  EEPROM.writeInt(adr_eprom_WIFI_ON, WIFI_ON);
  EEPROM.writeInt(adr_eprom_SERVO_STEPS, SERVO_STEPS);
  EEPROM.writeInt(adr_eprom_SERVO_MAX, SERVO_MAX);
  EEPROM.writeInt(adr_eprom_SERVO_MIN, SERVO_MIN);
  EEPROM.writeInt(adr_eprom_SERVO_Mitte, SERVO_Mitte);
  EEPROM.writeInt(adr_eprom_SERVO_Hz, SERVO_Hz);
  EEPROM.writeInt(adr_eprom_POWER_SCALE, POWER_SCALE);
  EEPROM.commit();
  Serial.println("EEPROM gespeichert.");
}
