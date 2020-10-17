/*
   Servotester Deluxe
   Ziege-One (Der RC-Modelbauer)
   v1.0
 
 ESP32 + Encoder + OLED

 /////Pin Belegung////
 GPIO 13: Servo1
 GPIO xx: Servo2
 GPIO xx: Servo3
 GPIO xx: Servo4
 GPIO xx: Servo5
 GPIO 15: Ecoder Taster
 GPIO 16: Ecoder Richtung 1
 GPIO 17: Ecoder Richtung 2

 GPIO 21: SDA OLED
 GPIO 22: SDL OLED
 */
 
// ======== Vario2HoTT  =======================================

#include <WiFi.h>
#include <ESP32Servo.h>
#include "SSD1306Wire.h" 
#include <ESP32Encoder.h>

ESP32Encoder encoder;

long prev = 0;
int buttonState = 0;  // 0 = not pressed   --- 1 = long pressed --- 2 short pressed
int Button_PIN = 15;
int Duration_in_Millis = 1000; 
int encoder_last;
int servo_pos[5]; //Speicher 1
int servocount = 0;

SSD1306Wire display(0x3c, SDA, SCL);   // ADDRESS, SDA, SCL  -  SDA and SCL usually populate automatically based on your board's pins_arduino.h

// Optionally include custom images
#include "images.h"

Servo servo[5]; // create servo object to control a servo
// twelve servo objects can be created on most boards

// GPIO the servo is attached to
volatile unsigned char servopin[5]  ={13, 14, 27, 33, 32}; // Pin Servo Output

// Replace with your network credentials
const char* ssid     = "wlan";
const char* password = "123456789";

// Set web server port number to 80
WiFiServer server(80);

// Variable to store the HTTP request
String header;

// Decode HTTP GET value
String valueString = String(5);
int pos1 = 0;
int pos2 = 0;

// Current time
unsigned long currentTime = millis();
// Previous time
unsigned long previousTime = 0; 
// Define timeout time in milliseconds (example: 2000ms = 2s)
const long timeoutTime = 2000;

void setup() {
  // Setup Serial
  Serial.begin(115200);
  
  //Setup Servos
  servo[0].attach(servopin[0],1000,2000);  // attaches the servo on the servoPin to the servo object
  servo[1].attach(servopin[1],1000,2000);  // attaches the servo on the servoPin to the servo object
  servo[2].attach(servopin[2],1000,2000);  // attaches the servo on the servoPin to the servo object
  servo[3].attach(servopin[3],1000,2000);  // attaches the servo on the servoPin to the servo object
  servo[4].attach(servopin[4],1000,2000);  // attaches the servo on the servoPin to the servo object

  // Setup Encoder
  ESP32Encoder::useInternalWeakPullResistors=UP;
  encoder.attachHalfQuad(16, 17);
  pinMode(Button_PIN,INPUT_PULLUP);     // Button_PIN = Eingang
  encoder.setCount(90); // set starting count value after attaching

  // Setup OLED
  display.init(); 
  display.flipScreenVertically();
  display.setFont(ArialMT_Plain_10);
  display.drawXbm(0, 0, Logo_width, Logo_height, Logo_bits);
  display.display();

  delay(1000);
  
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
  delay(3000);
  
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
  delay(3000);
  }
}

void loop(){

  //Serial.println("Encoder count = "+String((int32_t)encoder.getCount()));
  // Button Abfrage
  buttonState = 0;
  if (!(digitalRead(Button_PIN))){  // Button gedrückt 0
    prev = millis();
    buttonState = 1;
    while((millis()-prev)<=Duration_in_Millis){
      if(digitalRead(Button_PIN)){  // Button wieder 1 innerhalb Zeit
        buttonState = 2;
        break;
      }
    }
    while(!(digitalRead(Button_PIN))){ // Warten bis Button nicht gedückt ist = 1
    }
  }

  if(!buttonState){
    // TODO nothing is pressed
  }else if(buttonState == 1){
    // TODO button is pressed long
    servocount++;
    if (servocount > 4) 
    {
      servocount = 0;
    }
  }else if(buttonState ==2){
    //TODO button is pressed short
    encoder.setCount(90);
  }

    if (encoder_last != encoder.getCount())
    {
      servo_pos[servocount]  = encoder.getCount();   
    }
    else
    {
      encoder.setCount(servo_pos[servocount]); 
    }
      
    encoder_last  = encoder.getCount();
  
    if (servo_pos[servocount] > 180) 
    {
       servo_pos[servocount] = 180;
    }
    else if (servo_pos[servocount] < 0) 
    {
       servo_pos[servocount] = 0;
    } 

    servo[servocount].write(servo_pos[servocount]);

    display.clear();
    display.setTextAlignment(TEXT_ALIGN_CENTER);
    display.setFont(ArialMT_Plain_24);
    display.drawString(64, 0, "Servo" + String(servocount+1));
    display.drawString(64, 25, String(servo_pos[servocount]));
    display.drawProgressBar(8, 50, 112, 10, ((servo_pos[servocount]*100)/180));
    display.display();

    valueString = String(servo_pos[servocount]);

  
  
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
            client.println("<input type=\"range\" min=\"0\" max=\"180\" class=\"slider\" id=\"servoSlider\" onchange=\"servo(this.value)\" value=\""+valueString+"\"/>");
            
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
              servo_pos[servocount] = 90;
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
