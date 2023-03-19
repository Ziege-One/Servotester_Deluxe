/*
 *  Funktionsgenerator für Sinus, Dreieck und Rechteck Signale
 *  Einstellbare Frequenz 20 Hz bis 20 KHz
 *  Für Dreieck und Rechteck einstellbares Tastverhältnis 0 bis 100%
 *  Ausgangsspannung 3.3V Signale nur positiv!
 *  See: https://www.az-delivery.de/en/blogs/azdelivery-blog-fur-arduino-und-raspberry-pi/funktionsgenerator-mit-dem-esp32-teil1
 */

#include "Arduino.h"

// Bibliotheken zum direkten Zugriff auf Steuerregister des ESP32
#include "soc/rtc_cntl_reg.h"
#include "soc/sens_reg.h"
#include "soc/rtc.h"

// Bibliotheken zur Verwendung des Digital zu Analog Konverters und für den I2S-Bus
#include "driver/dac.h"
#include "driver/i2s.h"

#define SINFAKT 127.0 // gemessen für Schrittweite = 1 und kein Vorteiler (8.3MHz)

// Buffer zum Erstellen der Dreieckfunktion
uint32_t buf[128];

// Einstellwerte für Kurvenform, Frequenz und Tastverhältnis
char mode = 'R';        // S=Sinus, R=Rechteck, T=Dreieck
float frequency = 1000; // 20 bis 200000 Hz
uint8_t ratio = 50;     // Tastverhältnis 0 bis 100%

// Flag ist wahr, wenn die Initialisierung bereits erfolgte
bool initDone = false;

// Menu positions
int selectedItem = 0;     // The selected menu item
int mX[] = {22, 64, 106}; // x coordinates

// Konfiguration für den I2S Bus
i2s_config_t i2s_config = {
    .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX | I2S_MODE_DAC_BUILT_IN), // Betriebsart
    .sample_rate = 100000,                                                       // Abtastrate
    .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,                                // der DAC verwendet nur 8 Bit des MSB
    .channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT,                                // Kanalformat ESP32 unterstützt nur Stereo
    .communication_format = (i2s_comm_format_t)I2S_COMM_FORMAT_STAND_I2S,        // Standard Format für I2S (I2S_COMM_FORMAT_STAND_I2S or I2S_COMM_FORMAT_I2S_MSB)
    .intr_alloc_flags = 0,                                                       // Standard Interrupt
    .dma_buf_count = 2,                                                          // Anzahl der FIFO Buffer
    .dma_buf_len = 32,                                                           // Größe der FIFO Buffer
    .use_apll = 0                                                                // Taktquelle
};

//
// =======================================================================================================
// FILL TRIANGLE BUFFER
// =======================================================================================================
//

// Buffer für Dreieck Wellenform füllen
// Parameter up ist die Dauer für den Anstieg in Prozent
// Parameter sz gibt die Buffergröße für eine Periode an
// es werden die Werte für eine Periode in den Buffer geschrieben
void fillBuffer(uint8_t up, uint8_t sz)
{
  uint8_t down;      // Zeit für die fallende Flanke in %
  uint32_t sample;   // 32Bit Datenwort (I2S benötigt zwei Kanäle mit je 16 Bit
  float du, dd, val; // Hilfsvariablen
  down = 100 - up;
  // Anzahl der Schritte für Anstieg und Abfall berechnen
  uint16_t stup = round(1.0 * sz / 100 * up);
  uint16_t stdwn = round(1.0 * sz / 100 * down);
  uint16_t i;
  if ((stup + stdwn) < sz)
    stup++; // Ausgleich eventueller Rundungsfehler
  // Amplitudenänderung pro Schritt für Anstieg und Abfall
  du = 256.0 / stup;
  dd = 256.0 / stdwn;
  // füllen des Buffers
  val = 0; // Anstieg beginnt mit 0
  for (i = 0; i < stup; i++)
  {
    sample = val;
    sample = sample << 8; // Byte in /Byte verschieben
    buf[i] = sample;
    val = val + du; // Wert erhöhen
  }
  val = 255; // Abfallende Flanke beginnt mit Maximalwert
  // Rest wie bei der ansteigenden Flanke
  for (i = 0; i < stdwn; i++)
  {
    sample = val;
    sample = sample << 8;
    buf[i + stup] = sample;
    val = val - dd;
  }
}

//
// =======================================================================================================
// STOP ALL
// =======================================================================================================
//

void stopAll()
{
  ledcDetachPin(26);
  i2s_driver_uninstall((i2s_port_t)0);
  dac_output_disable(DAC_CHANNEL_2);
  dac_i2s_disable();
  pinMode(26, OUTPUT); // Required in order to make rectangle working after sinus mode
  initDone = false;
  Serial.println("Stopped all");
}

//
// =======================================================================================================
// START RECTANGLE
// =======================================================================================================
//

// Pin 26 als Ausgang zuweisen
void startRectangle()
{
  ledcAttachPin(26, 2);
  initDone = true;
  Serial.println("Rectangle Started");
}

//
// =======================================================================================================
// RECTANGLE SET FREQUENCY
// =======================================================================================================
//

// Frequenz für Rechteck setzen mit entsprechendem Tastverhältnis
void rectangleSetFrequency(double frequency, uint8_t ratio)
{
  ledcSetup(2, frequency, 7);        // Wir nutzen die LEDC Funktion mit 7 bit Auflösung
  ledcWrite(2, 127.0 * ratio / 100); // Berechnung der Schrittanzahl für Zustand = 1
}

//
// =======================================================================================================
// START TRIANGLE
// =======================================================================================================
//

// Dreiecksignal starten
void startTriangle()
{
  i2s_set_pin((i2s_port_t)0, NULL); // I2S wird mit dem DAC genutzt
  initDone = true;
  Serial.println("Triangle Started");
}

//
// =======================================================================================================
// TRIANGLE SET FREQUENCY
// =======================================================================================================
//

double triangleSetFrequency(double frequency, uint8_t ratio)
{
  int size = 64;
  // zuerst wird die geeignete Buffergröße ermittelt
  // damit die Ausgabe funktioniert, muss die I2S Abtastrate zwischen
  // 5200 und 650000 liegen
  if (frequency < 5000)
  {
    size = 64;
  }
  else if (frequency < 10000)
  {
    size = 32;
  }
  else if (frequency < 20000)
  {
    size = 16;
  }
  else
  {
    size = 8;
  }
  // Abtastrate muss in einer Periode beide Buffer ausgeben
  uint32_t rate = frequency * 2 * size;
  // Die Abtastrate darf nur innerhalb der Grenzwerte liegen
  if (rate < 5200)
    rate = 5200;
  if (rate > 650000)
    rate = 650000;
  // wirklichen Frequenzwert setzen
  frequency = rate / 2 / size;

  // I2S Treiber entfernen
  i2s_driver_uninstall((i2s_port_t)0);
  // Konfiguration anpassen
  i2s_config.sample_rate = rate;
  i2s_config.dma_buf_len = size;
  // und mit der neuen Konfiguration installieren
  i2s_driver_install((i2s_port_t)0, &i2s_config, 0, NULL);
  i2s_set_pin((i2s_port_t)0, NULL); // I2S wird mit dem DAC genutzt
  // Abtastrate einstellen
  i2s_set_sample_rates((i2s_port_t)0, rate);
  // Buffer füllen
  fillBuffer(ratio, size * 2);
  // und einmal ausgeben
  size_t i2s_Bytes_written;
  i2s_write((i2s_port_t)0, (const char *)&buf, size * 8, &i2s_Bytes_written, 100);
  return frequency;
}

//
// =======================================================================================================
// START SINUS
// =======================================================================================================
//

void startSinus()
{
  // Ausgang für Pin26 freigeben
  dac_output_enable(DAC_CHANNEL_2);
  // Sinusgenerator aktivieren
  SET_PERI_REG_MASK(SENS_SAR_DAC_CTRL1_REG, SENS_SW_TONE_EN);
  // Ausgabe auf Kanal 1 starten
  SET_PERI_REG_MASK(SENS_SAR_DAC_CTRL2_REG, SENS_DAC_CW_EN2_M);
  // Vorzeichenbit umkehren
  SET_PERI_REG_BITS(SENS_SAR_DAC_CTRL2_REG, SENS_DAC_INV2, 2, SENS_DAC_INV2_S);
  initDone = true;
  Serial.println("Sinus Started");
}

//
// =======================================================================================================
// SINUS SET FREQUENCY
// =======================================================================================================
//

double sinusSetFrequency(double frequency)
{
  // Formel f = s * SINFAKT / v
  // s sind die Schritte pro Taktimpuls
  // v ist der Vorteiler für den 8MHz Takt
  // Es gibt 8 Vorteiler von 1 bis 1/8 um die Kombination Vorteiler und
  // Schrittanzahl zu finden, testen wir alle acht Vorteiler Varianten
  // Die Kombination mit der geringsten Frequenzabweichung wird gewählt

  double f, delta, delta_min = 999999999.0;
  uint16_t divi = 0, step = 1, s;
  uint8_t clk_8m_div = 0; // 0 bis 7
  for (uint8_t div = 1; div < 9; div++)
  {
    s = round(frequency * div / SINFAKT);
    if ((s > 0) && ((div == 1) || (s < 1024)))
    {
      f = SINFAKT * s / div;
      /*
        Serial.print(f); Serial.print(" ");
        Serial.print(div); Serial.print(" ");
        Serial.println(s);
        */
      delta = abs(f - frequency);
      if (delta < delta_min)
      { // Abweichung geringer -> aktuelle Werte merken
        step = s;
        divi = div - 1;
        delta_min = delta;
      }
    }
  }
  // wirklichen Frequenzwert setzen
  frequency = SINFAKT * step / (divi + 1);
  // Vorteiler einstellen
  REG_SET_FIELD(RTC_CNTL_CLK_CONF_REG, RTC_CNTL_CK8M_DIV_SEL, divi);
  // Schritte pro Taktimpuls einstellen
  SET_PERI_REG_BITS(SENS_SAR_DAC_CTRL1_REG, SENS_SW_FSTEP, step, SENS_SW_FSTEP_S);
  return frequency;
}

//
// =======================================================================================================
// CONTROL GENERATOR
// =======================================================================================================
//

// Einstellungsänderungen durchführen
void controlGenerator()
{
  switch (mode)
  {
  case 'S':
  case 's':
    if (!initDone)
      startSinus();
    frequency = sinusSetFrequency(frequency);
    break;
  case 'T':
  case 't':
    if (!initDone)
      startTriangle();
    frequency = triangleSetFrequency(frequency, ratio);
    break;
  case 'R':
  case 'r':
    if (!initDone)
      startRectangle();
    rectangleSetFrequency(frequency, ratio);
    break;
  }
}

//
// =======================================================================================================
// DISPLAY
// =======================================================================================================
//

void drawSignalGeneratorDisplay()
{

  static unsigned long previousUpdate;

  if (millis() - previousUpdate >= 200)
  { // every 200 ms
    previousUpdate = millis();

    // clear old display contentmt
    display.clear();
    display.setTextAlignment(TEXT_ALIGN_CENTER);
    display.setFont(ArialMT_Plain_16);
    display.drawString(64, 0, signalGeneratorString[LANGUAGE]);

    // Menu
    display.setFont(ArialMT_Plain_10);
    display.drawRect((mX[selectedItem] - 20), 25, 40, 13); // Selected item

    display.drawString(mX[0], 25, "Mode");
    display.drawString(mX[0], 45, String(mode));

    display.drawString(mX[1], 25, "Freq.Hz");
    display.drawString(mX[1], 45, String(frequency, 0));

    display.drawString(mX[2], 25, "Ratio %");
    display.drawString(mX[2], 45, String(ratio));

    /*
        display.setTextAlignment(TEXT_ALIGN_LEFT);
        display.drawString(0, 0, String(pulseWidth) + "µs"); // Show pulsewidth

        // Popup window
        if (millis() - popupMillis < 1500)
        {
          display.setTextAlignment(TEXT_ALIGN_CENTER);
          display.setColor(BLACK);
          display.fillRect(25, 22, 78, 29); // Clear area behind window
          display.setColor(WHITE);
          display.drawRect(25, 22, 78, 29); // Draw window frame
          display.drawString(64, 25, "Sampling delay");
          display.drawString(64, 35, String(samplingDelay) + " µs"); // Show sampling delay
        }*/

    // refresh display content
    display.display();
  }
}

//
// =======================================================================================================
// SIGNAL GENERATOR SETUP
// =======================================================================================================
//

// Serielle Schnittstelle aktivieren und
// Defaulteinstellungen 1kHz Sinus setzen
void signalGeneratorSetup()
{
  controlGenerator();
  Serial.println("Use Pin 26 / Benutze Pin 26");
  Serial.print("Commands / Kommandos: M (Mode / Betriebsart): S (Sinus), T (Triangle), R (Rectangle), F (Frequency / Frequenz), R (Ratio / Tastverhältnis)");
}

//
// =======================================================================================================
// SIGNAL GENERATOR MENU
// =======================================================================================================
//

void signalGeneratorMenu()
{

  if (encoderState == 1)
  {
    switch (selectedItem)
    {
    case 0:
      if (mode == 'S')
        mode = 'R';
      else
        mode = 'S';
      stopAll();
      break;
    case 1:
      frequency /= 1.05;
      break;
    case 2:
      ratio -= 5;
      break;
    }
    controlGenerator();
  }

  if (encoderState == 2)
  {
    switch (selectedItem)
    {
    case 0:
      if (mode == 'S')
        mode = 'R';
      else
        mode = 'S';
      stopAll();
      break;
    case 1:
      frequency *= 1.05;
      break;
    case 2:
      ratio += 5;
      break;
    }
    controlGenerator();
  }

  if (buttonState == 2)
  {
    selectedItem++;
  }

  // Limitations
  if (selectedItem > 2)
    selectedItem = 0;
  if (selectedItem < 0)
    selectedItem = 2;

  if (ratio > 100)
    ratio = 100;
  if (ratio < 5)
    ratio = 5;
  if (frequency < 20)
    frequency = 20;
  if (frequency > 20000)
    frequency = 20000;
}

//
// =======================================================================================================
// SIGNAL GENERATOR LOOP
// =======================================================================================================
//

void signalGeneratorLoop(bool init)
{

  if (init) // Init = setup
  {
    signalGeneratorSetup();
  }

  signalGeneratorMenu();
  drawSignalGeneratorDisplay();

  // Serielle Schnittstelle abfragen
  if (Serial.available() > 0)
  {
    // Befehl von der Schnittstelle einlesen
    String inp = Serial.readStringUntil('\n');
    // und zur Kontrolle ausgeben
    Serial.println(inp);
    char cmd = inp[0]; // erstes Zeichen ist das Kommando
    if ((cmd == 'M') || (cmd == 'm'))
    {                        // war das Zeichen 'M' wird die Betriebsart eingestellt
      char newMode = inp[1]; // zweites Zeichen ist die Betriebsart
      if (newMode != mode)
      { // Nur wenn eine Änderung vorliegt, mus was getan werden
        stopAll();
        mode = newMode;
        controlGenerator();
      }
    }
    else
    {
      // bei den anderen Befehlen folgt ein Zahlenwert
      String dat = inp.substring(1);
      // je nach Befehl, werden die Daten geändert
      switch (cmd)
      {
      case 'F':
      case 'f':
        frequency = dat.toDouble();
        break; // Frequency / Frequenz
      case 'R':
      case 'r':
        ratio = dat.toInt();
        break; // Ratio / Tastverhältnis
      }
      // Grenzwerte werden überprüft
      if (ratio > 100)
        ratio = 100;
      if (frequency < 20)
        frequency = 20;
      if (frequency > 20000)
        frequency = 20000;
      controlGenerator();
    }
    // aktuelle Werte ausgeben
    String ba;
    switch (mode)
    {
    case 'S':
    case 's':
      ba = "Sine / Sinus";
      break;
    case 'T':
    case 't':
      ba = " Triangle / Dreieck";
      break;
    case 'R':
    case 'r':
      ba = "Rectangle / Rechteck";
      break;
    }
    Serial.println("**************** Adjusted Values / Eingestellte Werte ********************");
    Serial.print("Mode / Betriebsart     = ");
    Serial.println(ba);
    Serial.print("Frequncy / Frequenz    = ");
    Serial.print(frequency);
    Serial.println("Hz");
    Serial.print("Ratio / Tastverhältnis = ");
    Serial.print(ratio);
    Serial.println("%");
    Serial.println();
    Serial.print("Commands / Kommandos: M (Mode), F (Frequency), R (Ratio) : ");
  }
}