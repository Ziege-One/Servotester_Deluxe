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

#include <WiFi.h>
#include <ESP32Servo.h>
#include "SSD1306Wire.h" 
#include <ESP32Encoder.h>
#include <EEPROM.h>
#include "sbus.h"

const float Version = 0.2; // Software Version

// EEprom
#define EEPROM_SIZE 20

#define adr_eprom_WIFI_ON 0                     // WIFI 1 = Ein 0 = Aus
#define adr_eprom_SERVO_STEPS 4                 // SERVO Steps für Encoder im Servotester Modus
#define adr_eprom_SERVO_MAX 8                   // SERVO µs Max Wert im Servotester Modus
#define adr_eprom_SERVO_MIN 12                  // SERVO µs Min Wert im Servotester Modus
#define adr_eprom_SERVO_Mitte 16                // SERVO µs Mitte Wert im Servotester Modus

int settings[5]; //Speicher der Einstellungen

//Encoder + Taster
ESP32Encoder encoder;

int button_PIN = 15;            // Hardware Pin Button
int encoder_Pin1 =16;           // Hardware Pin1 Encoder
int encoder_Pin2 =17;           // Hardware Pin2 Encoder
long prev = 0;                  // Zeitspeicher für Taster 
long previousDebouncTime = 0;   // Speicher Entprellzeit für Taster 
int buttonState = 0;            // 0 = Taster nicht betätigt; 1 = Taster langer Druck; 2 = Taster kurzer Druck
int encoderState = 0;           // 1 = Drehung nach Links (-); 2 = Drehung nach Rechts (+) 
int Duration_long = 1000;       // Zeit für langen Druck
int encoder_last;               // Speicher letzer Wert Encoder
int encoder_read;               // Speicher aktueller Wert Encoder

// Servo
volatile unsigned char servopin[5]  ={13, 14, 27, 33, 32}; // Pins Servoausgang
Servo servo[5];       // Servo Objekte erzeugen 
int servo_pos[5];     // Speicher für Servowerte
int servocount = 0;   // Zähler für Servowerte

// SBUS
bfs::SbusRx sbus_rx(&Serial2); // Sbus auf Serial2

std::array<int16_t, bfs::SbusRx::NUM_CH()> sbus_data;

//-Menu 30 Multiwitch Futaba lesen

#define kanaele 9       //Anzahl der Kanäle
int value1[kanaele];    //Speicher

//-Menu 20 Impuls lesen
int Impuls_min = 1000;
int Impuls_max = 2000;

//-Menu
int Menu = 1;             // Aktives Menu
bool SetupMenu = false;   // Zustand Setupmenu
int Einstellung =0 ;      // Aktives Einstellungsmenu
bool Edit = false;        // Einstellungen ausgewählt

SSD1306Wire display(0x3c, SDA, SCL);   // Oled Hardware an SDA 21 und SCL 22  

// Startlogo
#include "images.h"

// Wlan Einstellungen Client Modus
const char* ssid     = "wlan";
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
unsigned long previousTime = 0;         // Previous time
const long timeoutTime = 2000;          // Define timeout time in milliseconds (example: 2000ms = 2s)

// ======== Setup  =======================================
void setup() {
  // Setup Serial
  Serial.begin(115200);
  
  //EEPROM
  EEPROM.begin(EEPROM_SIZE);
  EEprom_Load(); // Einstellung laden
  Serial.println(settings[0]);
  Serial.println(settings[1]);
  Serial.println(settings[2]);
  Serial.println(settings[3]);
  Serial.println(settings[4]);

  if (settings[3] < 500) {    //EEporm int Werte
      settings[0] = 0;
      settings[1] = 10;
      settings[2] = 2000;
      settings[3] = 1000;
      settings[4] = 1500;
  }

  // Setup Encoder
  ESP32Encoder::useInternalWeakPullResistors=UP;
  encoder.attachHalfQuad(encoder_Pin1, encoder_Pin2);
  pinMode(button_PIN,INPUT_PULLUP);     // Button_PIN = Eingang
  

  // Setup OLED
  display.init(); 
  display.flipScreenVertically();
  display.setFont(ArialMT_Plain_10);
  display.drawXbm(0, 0, Logo_width, Logo_height, Logo_bits);
  display.display();

  delay(1000);

  if (settings[0] == 1) {     //Wifi Ein
    // Connect to Wi-Fi network with SSID and password
    Serial.print("Connecting to ");
    Serial.println(ssid);
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {   //10 Sekunden
      delay(500);
      Serial.print(".");
      if ((millis())>=10000)
      {
        break;
      }
    }

    if (WiFi.status() == WL_CONNECTED)
    {
    // Print local IP address and start web server
      Serial.println("");
      Serial.println("WiFi connected.");
      Serial.println("IP address: ");
      Serial.println(WiFi.localIP());

      display.clear();
      display.setTextAlignment(TEXT_ALIGN_LEFT);
      display.setFont(ArialMT_Plain_16);
      display.drawString(0, 10, "WiFi connected.");
      display.drawString(0, 26, "IP address: ");
      display.drawString(0, 42, WiFi.localIP().toString());
      display.display();
      delay(2000);
  
      server.begin();
    }
    else
    {
      Serial.println("");
      Serial.println("WiFi not connected.");

      display.clear();
      display.setTextAlignment(TEXT_ALIGN_LEFT);
      display.setFont(ArialMT_Plain_16);
      display.drawString(0, 10, "WiFi");
      display.drawString(0, 26, "not connected.");
      display.display();
      delay(2000);
    }
  }
  
  display.clear();
  display.setTextAlignment(TEXT_ALIGN_CENTER);
  display.setFont(ArialMT_Plain_16);
  display.drawString(64, 10, "Version:");
  display.drawString(64, 26, String(Version));
  display.display();
  delay(1000);
  

  encoder.setCount(Menu);
  servo_pos[servocount] = 1500; 
}

// ======== Loop  =======================================
void loop(){

  ButtonRead();

  MenuUpdate();

  if (settings[0] == 1) {     //Wifi Ein
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

            //HTML Seite angezeigen:
            client.println("<!DOCTYPE html><html>");
            client.println("<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">");
            client.println("<link rel=\"icon\" href=\"data:,\">");
            // Es folgen der CSS-Code um die Ein/Aus Buttons zu gestalten
            // Hier können Sie die Hintergrundfarge (background-color) und Schriftgröße (font-size) anpassen
            client.println("<style>body { text-align: center; font-family: \"Trebuchet MS\", Arial; margin-left:auto; margin-right:auto;}");
            client.println(".button { background-color: #333344; border: none; color: white; padding: 16px 40px;");
            client.println(".slider { width: 300px; }</style>");
            client.println("<script src=\"https://ajax.googleapis.com/ajax/libs/jquery/3.3.1/jquery.min.js\"></script>");
                     
            // Webseiten-Überschrift
            client.println("</head><body><h1>Servotester Deluxe</h1>");
            
            client.println("<p>Position: <span id=\"servoPos\"></span></p>");          
            client.println("<input type=\"range\" min=\""+String(settings[3], DEC)+"\" max=\""+String(settings[2], DEC)+"\" class=\"slider\" id=\"servoSlider\" onchange=\"servo(this.value)\" value=\""+valueString+"\"/>");
            
            client.println("<script>var slider = document.getElementById(\"servoSlider\");");
            client.println("var servoP = document.getElementById(\"servoPos\"); servoP.innerHTML = slider.value;");
            client.println("slider.oninput = function() { slider.value = this.value; servoP.innerHTML = this.value; }");
            client.println("$.ajaxSetup({timeout:1000}); function servo(pos) { ");
            client.println("$.get(\"/?value=\" + pos + \"&\"); {Connection: close};}</script>");

            //Button erstellen und link zum aufrufen erstellen 
            client.println("<p><a href=\"/5/on\"><button class=\"button\">Mitte</button></a></p>");
           
            client.println("</body></html>");     
            
            //GET /?value=180& HTTP/1.1
            if(header.indexOf("GET /?value=")>=0) {
              pos1 = header.indexOf('=');
              pos2 = header.indexOf('&');
              valueString = header.substring(pos1+1, pos2);
              
              //Rotate the servo
              //servo1.write(valueString.toInt());
              //encoder.setCount(valueString.toInt());
              servo_pos[servocount] = (valueString.toInt());
            }
            else if (header.indexOf("GET /5/on") >= 0) 
            {
              //servo1.write(90);
              //encoder.setCount(90);
              servo_pos[servocount] = settings[4]; //Mitte
            }               
            // The HTTP response ends with another blank line
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
    // Clear the header variable
    header = "";
    // Close the connection
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
    prev = millis();
    buttonState = 1;
    while((millis()-prev)<=Duration_long){
      if(digitalRead(button_PIN)){  // Button wieder 1 innerhalb Zeit
        buttonState = 2;
        break;
      }
    }
    while(!(digitalRead(button_PIN))){ // Warten bis Button nicht gedückt ist = 1
    }
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
 * 1 = Servotester       Auswahl -> 10 Servotesterz
 * 2 = Impuls lesen      Auswahl -> 20 Impuls lesen
 * 3 = Multiswitch lesen Auswahl -> 30 Multiswitch lesen
 * 4 = SBUS lesen        Auswahl -> 40 SBUS lesen
 * 5 = Einstellung       Auswahl -> 50 Einstellung
 */
void MenuUpdate(){

switch (Menu) {
  case 1: 
    display.clear();
    display.setTextAlignment(TEXT_ALIGN_CENTER);
    display.setFont(ArialMT_Plain_24);
    display.drawString(64, 0, "  Menu >" );
    display.setFont(ArialMT_Plain_16);
    display.drawString(64, 25,"Servotester" );
    display.display();
          
    if (encoderState == 1){
      Menu=1;
    }
    if (encoderState == 2){
      Menu++;
    }
    
    if(buttonState == 2){
      Menu = 10;
    }
    break;
  case 2: 
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
      Menu = 20;
    }
    break;  
  case 3: 
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
      Menu = 30;
    }
    break;
  case 4: 
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
      Menu = 40;
    }
    break;
  case 5: 
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
      Menu=5;
    }
       
    if(buttonState == 2){
      Menu = 50;
    }
    break;
 
  case 10:
    display.clear();
    display.setTextAlignment(TEXT_ALIGN_CENTER);
    display.setFont(ArialMT_Plain_24);
    display.drawString(64, 0, "Servo" + String(servocount+1));
    //display.drawString(64, 25, String(servo_pos[servocount]) + "°");
    //display.drawProgressBar(8, 50, 112, 10, ((servo_pos[servocount]*100)/180));
    display.drawString(64, 25, String(servo_pos[servocount]) + "µs");
    display.drawProgressBar(8, 50, 112, 10, (((servo_pos[servocount]-settings[3])*100)/(settings[2]-settings[3])));
    display.display();
    if (!SetupMenu)
    {
      servo[0].attach(servopin[0],settings[3],settings[2]);  // ServoPIN, MIN, MAX
      SetupMenu = true;
    }
    
    servo[servocount].writeMicroseconds(servo_pos[servocount]);
     
    if (encoderState == 1){
      servo_pos[servocount] = servo_pos[servocount] - settings[1] ;
    }
    if (encoderState == 2){
      servo_pos[servocount] = servo_pos[servocount] + settings[1] ;
    }

    if (servo_pos[servocount] > settings[2])   //Servo MAX
    {
       servo_pos[servocount] = settings[2];
    }
    else if (servo_pos[servocount] < settings[3])  //Servo MIN
    {
       servo_pos[servocount] = settings[3];
    } 
    
    if(buttonState == 1){
      Menu = 1; 
      SetupMenu = false;
      servo[0].detach();
    }

     if(buttonState == 2){
      servo_pos[servocount] = settings[4];    //Servo Mitte 
    }
    break;
  case 20:   // Impuls in
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
      Menu = 2; 
      SetupMenu = false;
    }

    if(buttonState == 2){
      Impuls_min = 1000;
      Impuls_max = 2000;
    }
    break;    

  case 30:   // Futaba Multiswitch in
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
      Menu = 3; 
      SetupMenu = false;
    }

     if(buttonState == 2){
      
    }
    break;

    case 40:   // Sbus
    display.clear();
    display.setTextAlignment(TEXT_ALIGN_RIGHT);
    display.setFont(ArialMT_Plain_10);
    display.drawString( 32, 0,  String(sbus_data[0]));
    display.drawString( 64, 0,  String(sbus_data[1]));
    display.drawString( 96, 0,  String(sbus_data[2]));
    display.drawString(128, 0,  String(sbus_data[3]));

    display.drawString( 32, 15,  String(sbus_data[4]));
    display.drawString( 64, 15,  String(sbus_data[5]));
    display.drawString( 96, 15,  String(sbus_data[6]));
    display.drawString(128, 15,  String(sbus_data[7]));

    display.drawString( 32, 30,  String(sbus_data[8]));
    display.drawString( 64, 30,  String(sbus_data[9]));
    display.drawString( 96, 30,  String(sbus_data[10]));
    display.drawString(128, 30,  String(sbus_data[11]));

    display.drawString( 32, 45,  String(sbus_data[12]));
    display.drawString( 64, 45,  String(sbus_data[13]));
    display.drawString( 96, 45,  String(sbus_data[14]));
    display.drawString(128, 45,  String(sbus_data[15]));
    display.display();
    
    if (!SetupMenu)
    {
      sbus_rx.Begin(servopin[0], servopin[0]+1);
      SetupMenu = true;
    }
    
  if (sbus_rx.Read()) {
      sbus_data = sbus_rx.ch();
  }
   

    if (encoderState == 1){
      
    }
    if (encoderState == 2){
      
    }
    
    if(buttonState == 1){
      Menu = 4; 
      SetupMenu = false;
    }

     if(buttonState == 2){
      
    }
    break;

  case 50:   // Einstellung
    display.clear();
    display.setTextAlignment(TEXT_ALIGN_CENTER);
    display.setFont(ArialMT_Plain_24);
    display.drawString(64, 0, "< Einstellung  >" );
    display.setFont(ArialMT_Plain_16);
    switch (Einstellung) {
      case 0:
        display.drawString(64, 25,"Wifi" );
        if (settings[Einstellung] == 1) {
          display.drawString(64, 45,"Ein" );
        }
        else {
          display.drawString(64, 45,"Aus" );
        }
        break;
      case 1:
        display.drawString(64, 25,"Servo Steps µs" );
        display.drawString(64, 45,String(settings[Einstellung]));
        break;
      case 2:
        display.drawString(64, 25,"Servo MAX µs" );
        display.drawString(64, 45,String(settings[Einstellung]));
        break;
      case 3:
        display.drawString(64, 25,"Servo MIN µs" );
        display.drawString(64, 45,String(settings[Einstellung]));
        break;
      case 4:
        display.drawString(64, 25,"Servo Mitte µs" );
        display.drawString(64, 45,String(settings[Einstellung]));
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
      settings[Einstellung]--;
      } 
    }
    if (encoderState == 2){
      if (!Edit) {    
      Einstellung++;
      }
      else
      {
      settings[Einstellung]++;
      } 
    }

    if (Einstellung > 4) 
    {
       Einstellung = 0;
    }
    else if (Einstellung < 0) 
    {
       Einstellung = 4;
    } 
    
    if (settings[0] < 0) {   //Wifi off nicht unter 0
      settings[0] = 0;
    }

   if (settings[0] > 1) {   //Wifi on nicht über 1
      settings[0] = 1;
    }

    if (settings[1] < 0) {   //Steps nicht unter 0
      settings[1] = 0;
    }

    if (settings[2] < settings[4]) {   //Max nicht unter Mitte
      settings[2] = settings[4] + 1;
    }

    if (settings[2] > 2500) {   //Max nicht über 2500
      settings[2] = 2500;
    }

    if (settings[3] > settings[4]) {   //Min nicht über Mitte
      settings[3] = settings[4] - 1;
    }

    if (settings[3] < 500) {   //Min nicht unter 500
      settings[3] = 500;
    }

    if (settings[4] > settings[2]) {   //Mitte nicht über Max
      settings[4] = settings[2] - 1;
    }

    if (settings[4] < settings[3]) {   //Mitte nicht unter Min
      settings[4] = settings[3] + 1;
    }
        
    if(buttonState == 1){
      Menu = 4; 
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
    Menu = 1; 
    break; // Wird nicht benötigt, wenn Statement(s) vorhanden sind
}

}

// ======== EEprom  ======================================= 
void EEprom_Load() {
  settings[0] = EEPROM.readInt(adr_eprom_WIFI_ON);
  settings[1] = EEPROM.readInt(adr_eprom_SERVO_STEPS);
  settings[2] = EEPROM.readInt(adr_eprom_SERVO_MAX);
  settings[3] = EEPROM.readInt(adr_eprom_SERVO_MIN);
  settings[4] = EEPROM.readInt(adr_eprom_SERVO_Mitte);
  Serial.println("EEPROM gelesen.");
}

void EEprom_Save() {
  EEPROM.writeInt(adr_eprom_WIFI_ON, settings[0]);
  EEPROM.writeInt(adr_eprom_SERVO_STEPS, settings[1]);
  EEPROM.writeInt(adr_eprom_SERVO_MAX, settings[2]);
  EEPROM.writeInt(adr_eprom_SERVO_MIN, settings[3]);
  EEPROM.writeInt(adr_eprom_SERVO_Mitte, settings[4]);
  EEPROM.commit();
  Serial.println("EEPROM gespeichert.");
}
