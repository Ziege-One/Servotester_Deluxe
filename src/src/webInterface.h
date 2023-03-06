
#include <Arduino.h>

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
                  SERVO_CENTER = (valueString.toInt());
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
                servo_pos[0] = SERVO_CENTER; // Mitte
              }
              if (header.indexOf("GET /mitte2/on") >= 0)
              {
                servo_pos[1] = SERVO_CENTER; // Mitte
              }
              if (header.indexOf("GET /mitte3/on") >= 0)
              {
                servo_pos[2] = SERVO_CENTER; // Mitte
              }
              if (header.indexOf("GET /mitte4/on") >= 0)
              {
                servo_pos[3] = SERVO_CENTER; // Mitte
              }
              if (header.indexOf("GET /mitte5/on") >= 0)
              {
                servo_pos[4] = SERVO_CENTER; // Mitte
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

                valueString = String(SERVO_CENTER, DEC);

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
